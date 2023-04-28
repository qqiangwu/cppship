#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/fs.h"
#include "cppship/util/io.h"

#include <cstdlib>
#include <set>

#include <fmt/os.h>
#include <range/v3/view/concat.hpp>
#include <toml.hpp>

using namespace cppship;

namespace {

template <class T = toml::value> T get(const toml::value& value, const std::string& key)
{
    if (!value.contains(key)) {
        throw Error { fmt::format("manifest key {} is required", key) };
    }

    return toml::find<T>(value, key);
}

CxxStd get_cxx_std(const toml::value& value)
{
    const auto val = toml::find_or<int>(value, "std", 17);
    const auto std = to_cxx_std(val);
    if (!std) {
        throw Error { fmt::format("manifest invalid std {}", val) };
    }

    return *std;
}

std::string get_option_value(const toml::value& value)
{
    if (value.is_boolean()) {
        return value.as_boolean() ? "True" : "False";
    }

    if (value.is_integer()) {
        return std::to_string(value.as_integer());
    }

    if (value.is_string()) {
        return fmt::format("\"{}\"", value.as_string().str);
    }

    throw Error { "dependency options' value can only be bool/int/string" };
}

std::vector<DeclaredDependency> parse_dependencis(const toml::value& manifest, const std::string& key)
{
    std::vector<DeclaredDependency> declared_deps;

    const auto deps = toml::find_or<toml::table>(manifest, key, {});
    for (const auto& [name, dep] : deps) {
        auto& pkg = declared_deps.emplace_back();

        pkg.package = name;

        if (dep.is_string()) {
            pkg.desc = ConanDep { .version = dep.as_string() };
        } else if (dep.is_table()) {
            if (dep.contains("git")) {
                GitHeaderOnlyDep desc;
                desc.git = find<std::string>(dep, "git");
                desc.commit = find_or<std::string>(dep, "commit", "");
                if (desc.git.empty()) {
                    throw Error { fmt::format("invalid git url {}", desc.git) };
                }
                if (desc.commit.empty()) {
                    throw Error { fmt::format("empty commit for git dependency {}", desc.git) };
                }

                pkg.desc = desc;
            } else {
                ConanDep desc;
                desc.version = toml::find<std::string>(dep, "version");
                for (const auto& [key, val] : toml::find_or<toml::table>(dep, "options", {})) {
                    desc.options.emplace(key, get_option_value(val));
                }

                pkg.components = toml::find_or<std::vector<std::string>>(dep, "components", {});
                pkg.desc = desc;
            }
        } else {
            throw Error { fmt::format("invalid dependency {} in manifest", name) };
        }
    }

    return declared_deps;
}

void check_dependency_dups(const std::vector<DeclaredDependency>& deps, const std::vector<DeclaredDependency>& dev_deps)
{
    std::set<std::string> package_seen;

    for (const auto& dep : ranges::views::concat(deps, dev_deps)) {
        if (package_seen.contains(dep.package)) {
            throw Error { fmt::format("package {} already declared", dep.package) };
        }

        package_seen.insert(dep.package);
    }
}

ProfileOptions parse_profile_options(
    const toml::value& manifest, const std::string& key, const std::string& cxxflags_default = "")
{
    const auto profile = toml::find_or(manifest, key, {});
    return {
        .cxxflags = toml::find_or<std::string>(profile, "cxxflags", cxxflags_default),
        .definitions = toml::find_or<std::vector<std::string>>(profile, "definitions", {}),
    };
}

}

Manifest::Manifest(const fs::path& file)
{
    if (!fs::exists(file)) {
        throw Error { "manifest file not exist" };
    }

    try {
        const auto value = toml::parse(file);
        const auto package = get(value, "package");

        mName = get<std::string>(package, "name");
        mVersion = get<std::string>(package, "version");
        mCxxStd = get_cxx_std(package);

        mDependencies = parse_dependencis(value, "dependencies");
        mDevDependencies = parse_dependencis(value, "dev-dependencies");

        check_dependency_dups(mDependencies, mDevDependencies);

        mProfileDefault = parse_profile_options(value, "profile", "");

        const auto profile = find_or(value, "profile", {});
        mProfileDebug = parse_profile_options(profile, "debug", "-g");
        mProfileRelease = parse_profile_options(profile, "release", "-O3 -DNDEBUG");
    } catch (const std::out_of_range& e) {
        throw Error { e.what() };
    } catch (const toml::exception& e) {
        throw Error { fmt::format("invalid manifest format at {}", e.what()) };
    }
}

const ProfileOptions& Manifest::profile(Profile prof) const
{
    switch (prof) {
    case Profile::debug:
        return mProfileDebug;

    case Profile::release:
        return mProfileRelease;
    }

    std::abort();
}

void cppship::generate_manifest(std::string_view name, CxxStd std, const fs::path& dir)
{
    if (!fs::exists(dir)) {
        fs::create_directory(dir);
    }

    const auto manifest = dir / "cppship.toml";
    if (fs::exists(manifest)) {
        throw Error { "manifest already exist" };
    }

    try {
        auto out = fmt::output_file(manifest.string());

        out.print("[package]\n");
        out.print(R"(name = "{}")", name);
        out.print("\n");
        out.print(R"(version = "0.1.0")");
        out.print("\nstd = {}\n", std);
        out.print("\n[dependencies]\n");

        out.close();
    } catch (const std::system_error& e) {
        throw Error { fmt::format("write filed {} failed: {}", manifest.string(), e.what()) };
    }

    // add .clang-format
    write(dir / ".clang-format", R"(---
Language: Cpp
BasedOnStyle: WebKit
ColumnLimit: 120
)");

    // add .clang-tidy
    write(dir / ".clang-tidy", R"(---
HeaderFilterRegex: ^(include|src|tests|benches)
Checks: -*,boost-*,bugprone-*,-bugprone-narrowing-conversions,-bugprone-easily-swappable-parameters,clang-analyzer-*,concurrency-*,cppcoreguidelines-*,misc-*,modernize-*,-modernize-pass-by-value,-modernize-use-trailing-return-type,-modernize-use-nodiscard,performance-*,portability-*,readability-*,-readability-make-member-function-const,-readability-redundant-access-specifiers,-readability-convert-member-functions-to-static,-readability-magic-numbers,-readability-named-parameter
WarningsAsErrors: '*'
InheritParentConfig: false
)");
}
