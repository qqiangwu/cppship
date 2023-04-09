#include "cppship/cmd/build.h"
#include "cppship/exception.h"
#include "cppship/util/cmd.h"
#include "cppship/util/fs.h"
#include "cppship/util/repo.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/algorithm/string/join.hpp>
#include <boost/process/system.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <gsl/util>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/transform.hpp>
#include <toml.hpp>
#include <toml/value.hpp>

using namespace cppship;

namespace {

constexpr std::string_view kConanBin = "conan";
constexpr std::string_view kNinjaBin = "ninja";

constexpr std::string_view kLockFile = "cppship.lock";
constexpr std::string_view kManifestFile = "cppship.toml";

}

int cmd::run_build(const BuildOptions& options)
{
    const auto manifest = load_manifest(get_project_root() / kManifestFile);
    const auto deps = resolve_dependencies(manifest);

    const auto build_dir = manifest.root / "build" / "debug";
    const auto ninji_build_file = build_dir / "build.ninja";
    if (!fs::exists(build_dir)) {
        fs::create_directories(build_dir);
    }

    ScopedCurrentDir guard(build_dir);
    write_file(ninji_build_file, detail::generate_build_ninji(manifest, deps));
    detail::build_ninji(options);

    return 0;
}

cmd::Manifest cmd::load_manifest(const fs::path& manifest_file)
{
    const auto manifest_tomb = toml::parse(manifest_file);
    const auto package_info = toml::find_or<std::map<std::string, toml::value>>(manifest_tomb, "package", {});

    // TODO(wuqq): do verify
    Manifest manifest { .project = package_info.at("name").as_string(), .root = get_project_root() };

    for (const auto& dep : toml::find_or<std::map<std::string, std::string>>(manifest_tomb, "dependencies", {})) {
        manifest.dependencies.emplace(dep.first, Manifest::Dependency { dep.second });
    }

    return manifest;
}

bool cmd::detail::lock_file_expired(const Manifest& manifest, const fs::path& lockfile)
{
    if (!fs::exists(lockfile)) {
        return true;
    }

    const auto last_resolve_deps_time = fs::last_write_time(lockfile);
    const auto last_manifest_changes = fs::last_write_time(manifest.root / kManifestFile);
    return last_resolve_deps_time < last_manifest_changes;
}

void cmd::detail::lock_dependencies(const fs::path& lockfile, const ResolvedDeps& deps)
{
    write_file(lockfile, toml::format(toml::value(deps)));
}

cmd::ResolvedDeps cmd::detail::load_cached_dependencies(const fs::path& lockfile)
{
    try {
        const auto value = toml::parse(lockfile);
        return toml::get<ResolvedDeps>(value);
    } catch (toml::exception&) {
        throw Error { fmt::format("invalid cppship lockfile {}", lockfile) };
    } catch (std::exception&) {
        throw Error { fmt::format("load cppship lockfile {} failed", lockfile) };
    }
}

void cmd::detail::install_conan_generator(const Manifest& manifest)
{
    const auto cfg_dir = manifest.root / ".cppship" / "generator";
    const auto cfg_lock_file = cfg_dir / "cppship.lock";

    if (fs::exists(cfg_lock_file)) {
        return;
    }

    if (!fs::exists(cfg_dir)) {
        fs::create_directories(cfg_dir);
    }

    constexpr std::string_view kGeneratorContent = R"(
from conans.model.conan_generator import Generator
from conans import ConanFile


class CppshipGenerator(ConanFile):
    name = "cppship_generator"
    version = "0.1.0"
    url = "https://github.com/qqiangwu/cppship"
    description = "Conan build generator for cppship build system"
    topics = ("conan", "generator", "cppship")
    homepage = "https://github.com/qqiangwu/cppship"
    license = "MIT"


class cppship(Generator):
    @property
    def filename(self):
        return "conan_cppship.json"

    @property
    def content(self):
        import json
        deps = self.deps_build_info
        return json.dumps({
            'defines': deps.defines,
            'include_paths': deps.include_paths,
            'lib_paths': deps.lib_paths,
            'libs': deps.libs
        })
    )";
    const auto conanfile = cfg_dir / "conanfile.py";
    write_file(conanfile, kGeneratorContent);

    const int result = boost::process::system(fmt::format("conan export {} cppship/generator", cfg_dir));
    if (result != 0) {
        throw Error { "install conan generator failed" };
    }

    write_file(cfg_lock_file, "");
}

std::string cmd::detail::generate_conan_requires(const Manifest& manifest)
{
    std::vector<std::string> lines;
    lines.reserve(manifest.dependencies.size());

    for (const auto& [name, dep] : manifest.dependencies) {
        lines.push_back(fmt::format("{}/{}", name, dep.version));
    }

    return boost::algorithm::join(lines, "\n");
}

void cmd::detail::generate_conan_file(const Manifest& manifest)
{
    const auto deps_dir = manifest.root / "build" / "conan";
    if (!fs::exists(deps_dir)) {
        fs::create_directories(deps_dir);
    }

    const auto conanfile = deps_dir / "conanfile.txt";
    constexpr std::string_view kConanfileTemplate = R"(
[generators]
cppship

[requires]
{}

[build_requires]
cppship_generator/0.1.0@cppship/generator
    )";

    const std::string content = fmt::format(kConanfileTemplate, generate_conan_requires(manifest));
    write_file(conanfile, content);
}

namespace {

std::vector<std::string> collect_dep_attr(const boost::property_tree::ptree& tree, const std::string& attr)
{
    return tree.get_child(attr) | ranges::views::values
        | ranges::views::transform([](const auto& item) { return item.data(); })
        | ranges::to<std::vector<std::string>>();
}

}

cmd::ResolvedDeps cmd::detail::install_conan_packages(const Manifest& manifest)
{
    const auto deps_dir = manifest.root / "build" / "conan";
    ScopedCurrentDir guard(deps_dir);

    const int result = boost::process::system(fmt::format("conan install . --build=missing --profile=debug"));
    if (result != 0) {
        throw Error { "install conan packages failed" };
    }

    const auto conan_deps_file = deps_dir / "conan_cppship.json";

    using namespace boost::property_tree;
    std::ifstream ifs(conan_deps_file);
    ptree tree;
    read_json(ifs, tree);

    return ResolvedDeps { .includes = collect_dep_attr(tree, "include_paths"),
        .defines = collect_dep_attr(tree, "defines"),
        .lib_dirs = collect_dep_attr(tree, "lib_paths"),
        .libs = collect_dep_attr(tree, "libs") };
}

cmd::ResolvedDeps cmd::resolve_dependencies(const Manifest& manifest)
{
    const auto lockfile = manifest.root / kLockFile;
    if (!detail::lock_file_expired(manifest, lockfile)) {
        return detail::load_cached_dependencies(lockfile);
    }

    require_cmd(kConanBin);

    detail::install_conan_generator(manifest);
    detail::generate_conan_file(manifest);
    auto deps = detail::install_conan_packages(manifest);

    detail::lock_dependencies(lockfile, deps);
    return deps;
}

namespace {

inline std::string format_entries(const std::string& prefix, const std::vector<std::string>& entries)
{
    using namespace ranges;

    return boost::join(entries | views::transform([&prefix](const auto& entry) { return prefix + entry; }), " ");
}

inline std::string format_includes(const std::vector<std::string>& entries) { return format_entries("-I", entries); }

inline std::string format_defines(const std::vector<std::string>& entries) { return format_entries("-D", entries); }

inline std::string format_libdirs(const std::vector<std::string>& entries) { return format_entries("-L", entries); }

inline std::string format_libs(const std::vector<std::string>& entries) { return format_entries("-l", entries); }

std::vector<std::string> compile_directory(const fs::path& source_dir, const cmd::ResolvedDeps& deps, std::ostream& out)
{
    std::vector<std::string> object_files;

    for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".cpp") {
            continue;
        }

        const auto& source_file = entry.path();
        const auto output_file = fmt::format("{}.o", source_file.lexically_relative(source_dir.parent_path()).string());

        out << fmt::format("build {}: compile {}\n", output_file, source_file.string())
            << fmt::format("  INCLUDES = {}\n", format_includes(deps.includes))
            << fmt::format("  DEFINES = {}\n", format_defines(deps.defines)) << std::endl;

        object_files.push_back(output_file);
    }

    return object_files;
}

}

std::string cmd::detail::generate_build_ninji(const Manifest& manifest, const ResolvedDeps& deps)
{
    std::ostringstream oss;

    oss << "rule compile\n"
        << "  command = g++ -std=c++20 -MD -MF $out.d $in -o $out -c $OPTIONS $DEFINES $INCLUDES\n"
        << "  description = compile $in to $out\n"
        << "  depfile = $out.d\n"
        << "  deps = gcc\n"
        << '\n'
        << "rule link_static\n"
        << "  command = rm -f $out && ar qc $out $in && ranlib $out\n"
        << "  description = link static $out\n"
        << '\n'
        << "rule binary\n"
        << "  command = g++ -std=c++20 $in -o $out $OPTIONS $LIBDIRS $LIBRARIES\n"
        << "  description = $PACKAGE_NAME\n"
        << std::endl;

    std::vector<std::string> implicit_deps;
    std::vector<std::string> default_targets;

    ResolvedDeps project_deps = deps;

    const auto build_dir = fs::current_path();
    if (const auto libdir = manifest.root / kLibPath; fs::exists(libdir)) {
        project_deps.includes.push_back(libdir.string());
        project_deps.includes.push_back(manifest.root / kIncludePath);

        const auto object_files = compile_directory(libdir, project_deps, oss);
        const auto out = fmt::format("{}/{}/lib{}.a", build_dir.string(), kLibPath, manifest.project);

        oss << fmt::format("build {}: link_static {}\n", out, boost::join(object_files, " ")) << std::endl;

        implicit_deps.push_back(out);
        default_targets.push_back(out);

        project_deps.lib_dirs.push_back(fmt::format("{}/{}", build_dir.string(), kLibPath));
        project_deps.libs.push_back(manifest.project);
    }

    if (const auto srcdir = manifest.root / kSrcPath; fs::exists(srcdir)) {
        project_deps.includes.push_back(srcdir.string());

        const auto object_files = compile_directory(srcdir, project_deps, oss);
        const auto out = build_dir / kSrcPath / manifest.project;

        oss << fmt::format("build {}: binary {}", out.string(), boost::join(object_files, " "));
        if (!implicit_deps.empty()) {
            oss << " | " << boost::join(implicit_deps, " ");
        }
        oss << std::endl;

        oss << fmt::format("  LIBRARIES = {}\n", format_libs(project_deps.libs))
            << fmt::format("  LIBDIRS = {}\n", format_libdirs(project_deps.lib_dirs))
            << fmt::format("  PACKAGE_NAME = {}\n", manifest.project) << std::endl;

        default_targets.push_back(out);
    }

    if (!default_targets.empty()) {
        oss << "default " << boost::join(default_targets, " ") << std::endl;
    }

    return oss.str();
}

void cmd::detail::build_ninji(const BuildOptions& options)
{
    require_cmd(kNinjaBin);

    const auto concurrency = [threads = options.max_concurrency] {
        if (threads <= 0) {
            return std::thread::hardware_concurrency();
        }

        return std::min(gsl::narrow_cast<unsigned>(threads), std::thread::hardware_concurrency());
    }();

    const int result = boost::process::system(fmt::format("{} -j {}", kNinjaBin, concurrency));
    if (result != 0) {
        throw Error { "run ninja failed" };
    }
}