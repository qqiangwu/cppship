#include "cppship/cmd/build.h"
#include "cppship/cmake/generator.h"
#include "cppship/core/dependency.h"
#include "cppship/exception.h"
#include "cppship/util/cmd.h"
#include "cppship/util/fs.h"
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
    BuildContext ctx;
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
    if (!ctx.is_expired(ctx.conan_profile_path)) {
        spdlog::info("[conan] profile is up to date");
        return;
    }

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
    while (std::getline(ifs, line)) {
        if (line.starts_with("build_type")) {
            line = fmt::format("build_type={}", ctx.profile);
        } else if (line.starts_with("compiler.cppstd")) {
            line = fmt::format("compiler.cppstd=20");
        }

        ofs << line << '\n';
    }

    if (!ifs.eof() || !ofs) {
        throw Error { "generate conan profile failed" };
    }
}

void cmd::conan_setup(const BuildContext& ctx)
{
    if (!ctx.is_expired(ctx.conan_file)) {
        spdlog::info("[conan] conanfile is up to date");
        return;
    }

    spdlog::info("[conan] setup");

    std::ostringstream oss;
    oss << "[requires]\n";
    for (const auto& dep : ctx.manifest.dependencies()) {
        oss << fmt::format("{}/{}\n", dep.package, dep.version);
    }

    oss << '\n';
    oss << "[generators]\n"
        << "CMakeDeps\n"
        << "CMakeToolchain\n";

    write_file(ctx.conan_file, oss.str());
}

void cmd::conan_install(const BuildContext& ctx)
{
    if (!ctx.is_expired(ctx.dependency_file)) {
        spdlog::info("[conan] dependency is up to date, no need to install");
        return;
    }

    const auto cmd = fmt::format("conan install {} -of {}/conan -pr {} --build=missing", ctx.build_dir.string(),
        ctx.profile_dir.string(), ctx.conan_profile_path.string());

    spdlog::info("[conan] install dependencies: {}", cmd);
    int res = pr::system(cmd);
    if (res != 0) {
        throw Error { "conan install failed" };
    }

    write_file(ctx.dependency_file, toml::format(toml::value(collect_deps(ctx.profile_dir / "conan", ctx.profile))));
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
            spdlog::info("[config] files not changed, skip");
            return;
        }
    }

    spdlog::info("[config] generate cmake file");
    CmakeGenerator gen(ctx.manifest, toml::get<ResolvedDependencies>(toml::parse(ctx.dependency_file)));
    write_file(ctx.build_dir / "CMakeLists.txt", std::move(gen).build());

    if (!fs::exists(ctx.profile_dir)) {
        fs::create_directories(ctx.profile_dir);
    }

    const std::string cmd = fmt::format(
        "cmake -B {} -S build -DCMAKE_BUILD_TYPE={} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCONAN_GENERATORS_FOLDER={}",
        ctx.profile_dir.string(), ctx.profile, (ctx.profile_dir / "conan").string());

    spdlog::info("[config] {}", cmd);
    const int res = boost::process::system(cmd, boost::process::shell);
    if (res != 0) {
        throw Error { "config cmake failed" };
    }

    fs::rename(ctx.profile_dir / "compile_commands.json", ctx.build_dir / "compile_commands.json");

    write_file(inventory_file, toml::format(toml::value({ { "files", files } })));
}

int cmd::cmake_build(const BuildContext& ctx, const BuildOptions& options)
{
    const auto cmd = fmt::format("cmake --build {} -j {}", ctx.profile_dir.string(), options.max_concurrency);

    spdlog::info("[build] {}", cmd);
    return boost::process::system(cmd, boost::process::shell);
}