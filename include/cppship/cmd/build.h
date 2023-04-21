#pragma once

#include <set>
#include <string>
#include <thread>

#include <boost/algorithm/string/case_conv.hpp>
#include <gsl/narrow>

#include "cppship/core/manifest.h"
#include "cppship/core/profile.h"
#include "cppship/util/fs.h"
#include "cppship/util/repo.h"

namespace cppship::cmd {

enum BuildGroup { lib, binaries, examples, tests, benches };

struct BuildOptions {
    int max_concurrency = gsl::narrow_cast<int>(std::thread::hardware_concurrency());
    Profile profile = Profile::debug;
    bool dry_run = false;
    std::optional<std::string> target;
    std::set<BuildGroup> groups;
};

struct BuildContext {
    std::string profile = "Debug";

    fs::path root = get_project_root();
    fs::path build_dir = root / kBuildPath;
    fs::path profile_dir = build_dir / boost::to_lower_copy(profile);
    fs::path metafile = root / "cppship.toml";

    fs::path conan_file = build_dir / "conanfile.txt";
    fs::path conan_profile_path = profile_dir / "conan_profile";
    fs::path inventory_file = profile_dir / "inventory.toml";
    fs::path dependency_file = profile_dir / "dependency.toml";

    Manifest manifest { metafile };

    explicit BuildContext(Profile profile_)
        : profile(to_string(profile_))
    {
    }

    [[nodiscard]] bool is_expired(const fs::path& path) const
    {
        return !fs::exists(path) || fs::last_write_time(path) < fs::last_write_time(metafile);
    }
};

int run_build(const BuildOptions& options);

void conan_detect_profile(const BuildContext& ctx);

void conan_setup(const BuildContext& ctx);

void conan_install(const BuildContext& ctx);

void cmake_setup(const BuildContext& ctx);

int cmake_build(const BuildContext& ctx, const BuildOptions& options);

}