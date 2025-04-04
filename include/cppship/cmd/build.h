#pragma once

#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <thread>

#include <boost/algorithm/string/case_conv.hpp>
#include <gsl/narrow>

#include "cppship/core/dependency.h"
#include "cppship/core/manifest.h"
#include "cppship/core/profile.h"
#include "cppship/core/workspace.h"
#include "cppship/util/cmd_runner.h"
#include "cppship/util/fs.h"
#include "cppship/util/repo.h"

namespace cppship::cmd {

enum class BuildGroup : std::uint8_t { lib, binaries, examples, tests, benches };

struct BuildOptions {
    int max_concurrency = gsl::narrow_cast<int>(std::thread::hardware_concurrency());
    Profile profile = Profile::debug;
    bool dry_run = false;
    std::optional<std::string> package;
    std::optional<std::string> cmake_target;
    std::set<BuildGroup> groups;
};

struct BuildContext {
    std::string profile = "Debug";

    fs::path root = get_project_root();
    fs::path package_root = get_package_root();

    fs::path build_dir = root / kBuildPath;
    fs::path packages_dir = build_dir / kBuildPackagesPath;
    fs::path deps_dir = build_dir / kBuildDepsPath;
    fs::path profile_dir = build_dir / boost::to_lower_copy(profile);
    fs::path metafile = root / "cppship.toml";

    fs::path conan_file = build_dir / "conanfile.txt";
    fs::path git_dep_file = build_dir / "git_dep.toml";
    fs::path conan_profile_path = profile_dir / "conan_profile";
    fs::path inventory_file = profile_dir / "inventory.toml";
    fs::path dependency_file = profile_dir / "dependency.toml";

    Manifest manifest { metafile };
    Workspace workspace { root, manifest };

    explicit BuildContext(Profile profile_)
        : profile(to_string(profile_))
    {
        if (!fs::exists(build_dir)) {
            fs::create_directories(build_dir);
        }
    }

    [[nodiscard]] bool is_expired(const fs::path& path) const;

    [[nodiscard]] std::optional<std::string> get_active_package() const;
};

int run_build(const BuildOptions& options);

void conan_detect_profile(const BuildContext& ctx);

void conan_setup(const BuildContext& ctx);

void conan_install(const BuildContext& ctx);

void cppship_install(
    const BuildContext& ctx, const ResolvedDependencies& cppship_deps, const ResolvedDependencies& all_deps);

namespace cmd_internals {

std::string cmake_gen_config(const BuildContext& ctx, bool for_standalone_cmake = false);

}

void cmake_setup(const BuildContext& ctx);

int cmake_build(const BuildContext& ctx, const BuildOptions& options, const util::CmdRunner& runner = {});

}
