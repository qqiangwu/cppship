#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>

#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"
#include "cppship/util/repo.h"

namespace cppship::cmd {

struct BuildOptions {
    int max_concurrency = 0;
};

struct BuildContext {
    std::string profile = "Debug";

    fs::path root = get_project_root();
    fs::path build_dir = root / kBuildPath;
    fs::path profile_dir = build_dir / boost::to_lower_copy(profile);
    fs::path metafile = root / "cppship.toml";

    fs::path conan_file = build_dir / "conanfile.txt";
    fs::path conan_profile_path = build_dir / ("conan_profile." + profile);
    fs::path inventory_file = build_dir / "inventory.toml";
    fs::path dependency_file = build_dir / "dependency.toml";

    Manifest manifest { metafile };

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