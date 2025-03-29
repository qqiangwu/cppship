#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <toml/value.hpp>
#include <variant>
#include <vector>

#include <fmt/core.h>
#include <range/v3/view/map.hpp>
#include <toml.hpp>

#include "cppship/util/assert.h"
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

class ResolvedDependencies {
public:
    ResolvedDependencies() = default;

    explicit ResolvedDependencies(std::initializer_list<Dependency> deps)
    {
        for (const auto& d: deps) {
            insert(d);
        }
    }

    explicit ResolvedDependencies(const toml::value& value)
        : mDeps(toml::get<decltype(mDeps)>(value))
    {
    }

    bool empty() const noexcept { return mDeps.empty(); }

    auto begin() const { return ranges::views::values(mDeps).begin(); }

    auto end() const { return ranges::views::values(mDeps).end(); }

    const Dependency& get_or_die(std::string_view package) const
    {
        auto it = mDeps.find(package);
        enforce(it != mDeps.end(), fmt::format("unexpected package {}", package));
        return it->second;
    }

    template <class D> bool insert(D&& dep)
    {
        return mDeps.emplace(std::string(dep.package), std::forward<D>(dep)).second;
    }

    toml::value to_toml() const { return toml::value(mDeps); }

private:
    std::map<std::string, Dependency, std::less<>> mDeps;
};

namespace dep_internals {

    Dependency parse_conan_cmake_target_file(std::string_view cmake_package, const fs::path& target_file);

}

ResolvedDependencies collect_conan_deps(const fs::path& conan_dep_dir, std::string_view profile);

}

TOML11_DEFINE_CONVERSION_NON_INTRUSIVE(cppship::Dependency, package, cmake_package, cmake_target, components);
