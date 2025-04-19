#include "cppship/cmd/build.h"

#include <fstream>
#include <sstream>
#include <string>

#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <fmt/format.h>
#include <gsl/util>
#include <range/v3/algorithm.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>
#include <toml.hpp>

#include "cppship/cmake/generator.h"
#include "cppship/cmake/group.h"
#include "cppship/cmake/package_configurer.h"
#include "cppship/core/compiler.h"
#include "cppship/core/dependency.h"
#include "cppship/core/resolver.h"
#include "cppship/exception.h"
#include "cppship/util/cmd.h"
#include "cppship/util/fs.h"
#include "cppship/util/git.h"
#include "cppship/util/io.h"
#include "cppship/util/log.h"

using namespace cppship;
using namespace boost::process;
using namespace fmt::literals;

namespace rng = ranges::views;

int cmd::run_build(const BuildOptions& options)
{
    BuildContext ctx(options.profile);
    ScopedCurrentDir guard(ctx.root);
    conan_detect_profile(ctx);
    conan_setup(ctx);
    conan_install(ctx);
    cmake_setup(ctx);

    if (options.dry_run) {
        return 0;
    }

    return cmake_build(ctx, options);
}

void cmd::conan_detect_profile(const BuildContext& ctx)
{
    if (!fs::exists(ctx.profile_dir)) {
        fs::create_directories(ctx.profile_dir);
    }

    if (!ctx.is_expired(ctx.conan_profile_path)) {
        debug("profile is up to date");
        return;
    }

    status("dependency", "detect profile {}", ctx.profile);
    std::string conan_default_profile_path = [] {
        try {
            return check_output("conan profile path default");
        } catch (const RunCmdFailed&) {
            const int res = run_cmd("conan profile detect");
            if (res != 0) {
                throw Error { "detect conan profile failed" };
            }

            return check_output("conan profile path default");
        }
    }();

    boost::trim(conan_default_profile_path);

    std::ifstream ifs(conan_default_profile_path);
    if (!ifs) {
        throw Error { fmt::format("cannot get default profile from {}", conan_default_profile_path) };
    }

    std::ofstream ofs(ctx.conan_profile_path);
    std::string line;
    bool compiler_detected = false;
    while (std::getline(ifs, line)) {
        if (line.starts_with("build_type")) {
            line = fmt::format("build_type={}", ctx.profile);
        } else if (line.starts_with("compiler.cppstd")) {
            line = fmt::format("compiler.cppstd=20");
        } else if (line.starts_with("compiler=")) {
            compiler_detected = true;
        }

        if (boost::contains(line, "=")) {
            status("dependency", "profile {}", line);
        }

        ofs << line << '\n';
    }

    if (!compiler_detected) {
        using namespace compiler;

        compiler::CompilerInfo info;
        if (info.id() == CompilerId::unknown) {
            throw Error { "cannot detect compiler" };
        }

        status("dependency", "detect compiler={}", to_string(info.id()));
        status("dependency", "detect compiler.version={}", info.version());
        status("dependency", "detect compiler.cppstd=20");
        status("dependency", "detect compiler.libcxx={}", info.libcxx());

        ofs << fmt::format("compiler={}\n", to_string(info.id()))
            << fmt::format("compiler.version={}\n", info.version()) << fmt::format("compiler.cppstd=20\n")
            << fmt::format("compiler.libcxx={}\n", info.libcxx());
    }

    ofs.flush();

    if (!ifs.eof() || !ofs) {
        throw Error { "generate conan profile failed" };
    }
}

void cmd::conan_setup(const BuildContext& ctx)
{
    if (!ctx.is_expired(ctx.conan_file)) {
        debug("conanfile is up to date");
        return;
    }

    status("dependency", "start resolving");
    Resolver resolver(ctx.deps_dir, &ctx.manifest, &util::git_clone);
    const auto result = std::move(resolver).resolve();

    status("dependency", "generate conanfile");
    std::ostringstream oss;
    oss << "[requires]\n";
    for (const auto& dep : result.conan_dependencies) {
        const auto& desc = std::get<ConanDep>(dep.desc);
        oss << fmt::format("{}/{}\n", dep.package, desc.version);
    }

    oss << '\n'
        << "[test_requires]\n"
        << "gtest/cci.20210126\n"
        << "benchmark/1.7.1\n";
    for (const auto& dep : result.conan_dev_dependencies) {
        const auto& desc = std::get<ConanDep>(dep.desc);
        oss << fmt::format("{}/{}\n", dep.package, desc.version);
    }

    oss << '\n';
    oss << "[generators]\n"
        << "CMakeDeps\n";

    if (const auto& deps = rng::concat(result.conan_dependencies, result.conan_dev_dependencies); !deps.empty()) {
        oss << "\n[options]\n";

        for (const auto& dep : deps) {
            const auto& desc = get<ConanDep>(dep.desc);
            for (const auto& [key, val] : desc.options) {
                oss << fmt::format("{}/*:{}={}\n", dep.package, key, val);
            }
        }
    }

    write(ctx.git_dep_file, toml::format(result.resolved_dependencies.to_toml()));
    write(ctx.conan_file, oss.str());
}

void cmd::conan_install(const BuildContext& ctx)
{
    if (!ctx.is_expired(ctx.dependency_file)) {
        debug("dependency is up to date");
        return;
    }

    const auto cmd = fmt::format("conan install {} -of {}/conan -pr {} --build=missing",
        ctx.build_dir.string(),
        ctx.profile_dir.string(),
        ctx.conan_profile_path.string());

    status("dependency", "install dependencies: {}", cmd);
    int res = run_cmd(cmd);
    if (res != 0) {
        throw Error { "conan install failed" };
    }

    auto deps = collect_conan_deps(ctx.profile_dir / "conan", ctx.profile);
    auto cppship_deps = toml::get<ResolvedDependencies>(toml::parse(ctx.git_dep_file));

    for (const auto& dep : cppship_deps) {
        deps.insert(dep);
    }

    if (!cppship_deps.empty()) {
        cppship_install(ctx, cppship_deps, deps);
    }

    write(ctx.dependency_file, toml::format(deps.to_toml()));
}

void cmd::cppship_install(
    const BuildContext& ctx, const ResolvedDependencies& cppship_deps, const ResolvedDependencies& all_deps)
{
    cmake::config_packages(cppship_deps, all_deps, { .deps_dir = ctx.deps_dir });
}

void cmd::cmake_setup(const BuildContext& ctx)
{
    const auto& inventory_file = ctx.inventory_file;

    const auto all_files = ctx.layout.all_files();
    const auto files
        = all_files | rng::transform([](const auto& file) { return file.string(); }) | ranges::to<std::set>();
    if (fs::exists(inventory_file)) {
        const auto saved_inventory = toml::parse(inventory_file);
        const auto saved = saved_inventory.at("files").as_array()
            | rng::transform([](const auto& val) { return val.as_string().str; });
        const bool has_lib = saved_inventory.at("lib").as_boolean();
        if (ranges::equal(files, saved) && !ctx.is_expired(inventory_file) && has_lib == ctx.layout.lib().has_value()) {
            debug("files not changed, skip");
            return;
        }
    }

    status("config", "generate cmake files");
    Resolver resolver(ctx.deps_dir, &ctx.manifest, nullptr);
    const auto result = std::move(resolver).resolve();

    ResolvedDependencies deps = toml::get<ResolvedDependencies>(toml::parse(ctx.dependency_file));
    CmakeGenerator gen(&ctx.layout,
        ctx.manifest,
        GeneratorOptions {
            .deps = cmake::resolve_deps(result.dependencies, deps),
            .dev_deps = cmake::resolve_deps(result.dev_dependencies, deps),
        });
    write(ctx.build_dir / "CMakeLists.txt", std::move(gen).build());

    const std::string cmd = fmt::format("cmake -B {} -S build -DCMAKE_BUILD_TYPE={} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "
                                        "-DCONAN_GENERATORS_FOLDER={} -DCPPSHIP_DEPS_DIR={}",
        ctx.profile_dir.string(),
        ctx.profile,
        (ctx.profile_dir / "conan").string(),
        ctx.deps_dir.string());

    status("config", "config cmake: {}", cmd);
    const int res = run_cmd(cmd);
    if (res != 0) {
        throw Error { "config cmake failed" };
    }

    if (const auto compile_db = ctx.profile_dir / "compile_commands.json"; fs::exists(compile_db)) {
        fs::rename(compile_db, ctx.build_dir / "compile_commands.json");
    }

    toml::value value;
    value["files"] = files;
    value["lib"] = ctx.layout.lib().has_value();
    write(inventory_file, toml::format(value));
}

namespace {

std::string_view to_cmake_group(cmd::BuildGroup group, const std::string_view lib)
{
    using namespace cmd;
    using namespace cmake;

    switch (group) {
    case BuildGroup::binaries:
        return kCppshipGroupBinaries;

    case BuildGroup::benches:
        return kCppshipGroupBenches;

    case BuildGroup::tests:
        return kCppshipGroupTests;

    case BuildGroup::examples:
        return kCppshipGroupExamples;

    case BuildGroup::lib:
        return lib;
    }

    std::abort();
}

}

int cmd::cmake_build(const BuildContext& ctx, const BuildOptions& options)
{
    auto cmd = fmt::format("cmake --build {} -j {} --config {}",
        ctx.profile_dir.string(),
        options.max_concurrency,
        to_string(options.profile));
    if (options.target) {
        cmd += fmt::format(" --target {}", *options.target);
    } else if (options.groups.empty()) {
        cmd += fmt::format(" --target {}", cmake::kCppshipGroupBinaries);
    } else {
        for (const auto group : options.groups) {
            cmd += fmt::format(" --target {}", to_cmake_group(group, ctx.manifest.name()));
        }
    }

    status("build", "{}", cmd);
    return run_cmd(cmd);
}
