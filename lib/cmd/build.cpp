#include "cppship/cmd/build.h"
#include "cppship/cmake/generator.h"
#include "cppship/core/compiler.h"
#include "cppship/core/dependency.h"
#include "cppship/exception.h"
#include "cppship/util/cmd.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"
#include "cppship/util/repo.h"

#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include <boost/algorithm/string/find.hpp>
#include <boost/process/system.hpp>
#include <gsl/util>
#include <range/v3/algorithm.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>
#include <toml.hpp>

using namespace cppship;

namespace rng = ranges::views;
namespace pr = boost::process;

int cmd::run_build(const BuildOptions& options)
{
    BuildContext ctx(options.profile);
    if (!fs::exists(ctx.build_dir)) {
        fs::create_directories(ctx.build_dir);
    }

    ScopedCurrentDir guard(ctx.root);
    conan_detect_profile(ctx);
    conan_setup(ctx);
    conan_install(ctx);
    cmake_setup(ctx);

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
    pr::pstream out;
    int res = pr::system("conan profile path default", pr::std_out > out);
    if (res != 0) {
        res = pr::system("conan profile detect");
        if (res != 0) {
            throw Error { "generate conan profile failed" };
        }

        out = pr::pstream {};
        res = pr::system("conan profile path default", pr::std_out > out);
        if (res != 0) {
            throw Error { "conan default profile not found" };
        }
    }

    std::string conan_default_profile_path;
    out >> conan_default_profile_path;
    if (!out) {
        throw Error { "get conan default profile failed" };
    }

    std::ifstream ifs(conan_default_profile_path);
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

    status("dependency", "generate conanfile");
    std::ostringstream oss;
    oss << "[requires]\n";
    for (const auto& dep : ctx.manifest.dependencies()) {
        oss << fmt::format("{}/{}\n", dep.package, dep.version);
    }

    oss << '\n'
        << "[test_requires]\n"
        << "gtest/cci.20210126\n";

    oss << '\n';
    oss << "[generators]\n"
        << "CMakeDeps\n";

    if (const auto& deps = ctx.manifest.dependencies(); !deps.empty()) {
        oss << "\n[options]\n";

        for (const auto& dep : deps) {
            for (const auto& [key, val] : dep.options) {
                oss << fmt::format("{}/*:{}={}\n", dep.package, key, val);
            }
        }
    }

    write(ctx.conan_file, oss.str());
}

void cmd::conan_install(const BuildContext& ctx)
{
    if (!ctx.is_expired(ctx.dependency_file)) {
        debug("dependency is up to date");
        return;
    }

    const auto cmd = fmt::format("conan install {} -of {}/conan -pr {} --build=missing", ctx.build_dir.string(),
        ctx.profile_dir.string(), ctx.conan_profile_path.string());

    status("dependency", "install dependencies: {}", cmd);
    int res = pr::system(cmd);
    if (res != 0) {
        throw Error { "conan install failed" };
    }

    write(ctx.dependency_file, toml::format(toml::value(collect_deps(ctx.profile_dir / "conan", ctx.profile))));
}

void cmd::cmake_setup(const BuildContext& ctx)
{
    const auto& inventory_file = ctx.inventory_file;

    const auto all_files = list_all_files();
    const auto files = all_files | rng::filter([](const auto& file) { return file.extension() == ".cpp"; })
        | rng::transform([](const auto& file) { return file.string(); }) | ranges::to<std::set<std::string>>();
    if (fs::exists(inventory_file)) {
        const auto saved_inventory = toml::parse(inventory_file);
        const auto saved = saved_inventory.at("files").as_array()
            | rng::transform([](const auto& val) { return val.as_string().str; });
        if (ranges::equal(files, saved) && !ctx.is_expired(inventory_file)) {
            debug("files not changed, skip");
            return;
        }
    }

    status("config", "generate cmake files");
    CmakeGenerator gen(ctx.manifest, toml::get<ResolvedDependencies>(toml::parse(ctx.dependency_file)));
    write(ctx.build_dir / "CMakeLists.txt", std::move(gen).build());

    const std::string cmd = fmt::format(
        "cmake -B {} -S build -DCMAKE_BUILD_TYPE={} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCONAN_GENERATORS_FOLDER={}",
        ctx.profile_dir.string(), ctx.profile, (ctx.profile_dir / "conan").string());

    status("config", "config cmake: {}", cmd);
    const int res = boost::process::system(cmd, boost::process::shell);
    if (res != 0) {
        throw Error { "config cmake failed" };
    }

    if (const auto compile_db = ctx.profile_dir / "compile_commands.json"; fs::exists(compile_db)) {
        fs::rename(compile_db, ctx.build_dir / "compile_commands.json");
    }

    write(inventory_file, toml::format(toml::value({ { "files", files } })));
}

int cmd::cmake_build(const BuildContext& ctx, const BuildOptions& options)
{
    auto cmd = fmt::format("cmake --build {} -j {}", ctx.profile_dir.string(), options.max_concurrency);
    if (options.target) {
        cmd += fmt::format(" --target {}", *options.target);
    }

    status("build", "{}", cmd);
    return boost::process::system(cmd, boost::process::shell);
}