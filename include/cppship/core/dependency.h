#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <toml.hpp>

#include "cppship/util/fs.h"

namespace cppship {

struct Dependency {
    std::string package;
    std::string cmake_package;
    std::string cmake_target;
    std::vector<std::string> components;
};

using ResolvedDependencies = std::map<std::string, Dependency>;

Dependency parse_dep(std::string_view cmake_package, const fs::path& target_file);

ResolvedDependencies collect_conan_deps(const fs::path& conan_dep_dir, std::string_view profile);

}

// NOLINTNEXTLINE(readability-identifier-length): make toml happy
TOML11_DEFINE_CONVERSION_NON_INTRUSIVE(cppship::Dependency, package, cmake_package, cmake_target, components);
