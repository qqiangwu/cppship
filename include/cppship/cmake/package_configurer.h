#pragma once

#include "cppship/core/dependency.h"

#include <functional>

namespace cppship::cmake {

struct ConfigOptions {
    fs::path deps_dir;
    fs::path out_dir = deps_dir;
    std::string cmake_deps_dir = "${CMAKE_SOURCE_DIR}/deps";
    std::function<void(std::string&)> post_process;
};

// for cppship deps, generate cmake packages for them
void config_packages(
    const ResolvedDependencies& cppship_deps, const ResolvedDependencies& all_deps, const ConfigOptions& options);

}