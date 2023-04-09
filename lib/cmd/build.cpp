#include "cppship/cmd/build.h"
#include "cppship/exception.h"
#include "cppship/util/cmd.h"
#include "cppship/util/fs.h"
#include "cppship/util/repo.h"

#include <exception>
#include <fstream>
#include <map>
#include <range/v3/range/conversion.hpp>
#include <stdexcept>
#include <string>
#include <system_error>

#include <boost/process/system.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/transform.hpp>
#include <toml.hpp>
#include <toml/exception.hpp>
#include <toml/from.hpp>
#include <toml/get.hpp>
#include <toml/into.hpp>
#include <toml/parser.hpp>
#include <toml/value.hpp>

using namespace cppship;

namespace {

constexpr std::string_view kConanBin = "conan";

constexpr std::string_view kLockFile = "cppship.lock";
constexpr std::string_view kManifestFile = "cppship.toml";

}

int cmd::run_build(const BuildOptions& options)
{
    (void)options;

    const auto manifest = load_manifest(get_project_root() / kManifestFile);
    const auto deps = resolve_dependencies(manifest);

    fmt::print("deps resolved\n");
    for (const auto& include : deps.includes) {
        fmt::print("include {}\n", include);
    }
    for (const auto& lib : deps.includes) {
        fmt::print("lib {}\n", lib);
    }

    return 0;
}

cmd::Manifest cmd::load_manifest(const fs::path& manifest_file)
{
    const auto manifest_tomb = toml::parse(manifest_file);
    Manifest manifest { .root = get_project_root() };

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