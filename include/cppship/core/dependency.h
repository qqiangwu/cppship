#pragma once

#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <toml.hpp>

#include "cppship/util/fs.h"

namespace cppship {

struct ConanDep {
    std::string version;
    std::unordered_map<std::string, std::string> options;
};

struct GitDep {
    std::string git;
    std::string commit;
};

using DependencyDesc = std::variant<ConanDep, GitDep>;

struct DeclaredDependency {
    std::string package;
    std::vector<std::string> components;
    DependencyDesc desc;

    bool is_conan() const { return std::holds_alternative<ConanDep>(desc); }

    bool is_git() const { return std::holds_alternative<GitDep>(desc); }
};

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

TOML11_DEFINE_CONVERSION_NON_INTRUSIVE(cppship::Dependency, package, cmake_package, cmake_target, components);
