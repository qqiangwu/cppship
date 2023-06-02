#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/fs.h"
#include "cppship/util/io.h"

#include <array>
#include <cstdlib>
#include <set>

#include <boost/algorithm/string.hpp>
#include <fmt/os.h>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/view/concat.hpp>
#include <toml.hpp>

using namespace cppship;
using namespace ranges;

namespace {

template <class T = toml::value> T get(const toml::value& value, const std::string& key)
{
    if (!value.contains(key)) {
        throw Error { fmt::format("manifest key {} is required", key) };
    }

    return toml::find<T>(value, key);
}

std::optional<bool> get_bool(const toml::value& value, const std::string& key)
{
    if (value.is_uninitialized() || !value.contains(key)) {
        return std::nullopt;
    }

    const auto& content = value.at(key);
    if (!content.is_boolean()) {
        throw Error { fmt::format("invalid manifest: {} should be a bool", key) };
    }

    return content.as_boolean();
}

std::vector<std::string> get_list(const toml::value& value, const std::string& key)
{
    if (value.is_uninitialized() || !value.contains(key)) {
        return {};
    }

    const auto& content = value.at(key);
    if (!content.is_array()) {
        throw Error { fmt::format("invalid manifest: {} should be an array", key) };
    }

    return get<std::vector<std::string>>(content);
}

std::unordered_map<std::string, toml::value> get_table(const toml::value& value, const std::string& key)
{
    if (value.is_uninitialized() || !value.contains(key)) {
        return {};
    }

    const auto& content = value.at(key);
    if (!content.is_table()) {
        throw Error { fmt::format("invalid manifest: {} should be a table", key) };
    }

    return get<std::unordered_map<std::string, toml::value>>(content);
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

std::vector<DeclaredDependency> parse_dependencies(const toml::value& manifest, const std::string& key)
{
    std::vector<DeclaredDependency> declared_deps;

    const auto deps = toml::find_or<toml::table>(manifest, key, {});
    for (const auto& [dep_name, dep_config] : deps) {
        auto& pkg = declared_deps.emplace_back();

        pkg.package = dep_name;

        if (dep_config.is_string()) {
            pkg.desc = ConanDep { .version = dep_config.as_string() };
        } else if (dep_config.is_table()) {
            if (dep_config.contains("git")) {
                GitDep desc;
                desc.git = find<std::string>(dep_config, "git");
                desc.commit = find_or<std::string>(dep_config, "commit", "");
                if (desc.git.empty()) {
                    throw Error { fmt::format("invalid git url {}", desc.git) };
                }
                if (desc.commit.empty()) {
                    throw Error { fmt::format("empty commit for git dependency {}", desc.git) };
                }

                pkg.desc = desc;
            } else {
                toml::table empty_table;

                ConanDep desc;
                desc.version = toml::find<std::string>(dep_config, "version");
                for (const auto& [key, val] : toml::find_or<toml::table>(dep_config, "options", empty_table)) {
                    desc.options.emplace(key, get_option_value(val));
                }

                pkg.components = toml::find_or<std::vector<std::string>>(dep_config, "components", {});
                pkg.desc = desc;
            }
        } else {
            throw Error { fmt::format("invalid dependency {} in manifest", dep_name) };
        }
    }

    return declared_deps;
}

void check_dependency_dups(const std::vector<DeclaredDependency>& deps, const std::vector<DeclaredDependency>& dev_deps)
{
    std::set<std::string> package_seen;

    for (const auto& dep : views::concat(deps, dev_deps)) {
        if (package_seen.contains(dep.package)) {
            throw Error { fmt::format("package {} already declared", dep.package) };
        }

        package_seen.insert(dep.package);
    }
}

ProfileConfig parse_profile_options(const toml::value& manifest, const std::string& key)
{
    const auto profile = toml::find_or(manifest, key, {});

    ProfileConfig config = {
        .cxxflags = get_list(profile, "cxxflags"),
        .linkflags = get_list(profile, "linkflags"),
        .definitions = get_list(profile, "definitions"),
        .ubsan = get_bool(profile, "ubsan"),
        .tsan = get_bool(profile, "tsan"),
        .asan = get_bool(profile, "asan"),
        .leak = get_bool(profile, "leak"),
    };

    if (config.tsan && *config.tsan) {
        if (config.asan && *config.asan) {
            throw Error { "tsan cannot be used with asan" };
        }

        if (config.leak && *config.leak) {
            throw Error { "tsan cannot be used with leak" };
        }
    }

    return config;
}

}

Manifest::Manifest(const fs::path& file)
{
    if (!fs::exists(file)) {
        throw Error { "manifest file not exist" };
    }

    try {
        const auto value = toml::parse(file);

        // parse [package]
        const auto package = get(value, "package");
        mName = get<std::string>(package, "name");
        mVersion = get<std::string>(package, "version");
        mCxxStd = get_cxx_std(package);

        mDependencies = parse_dependencies(value, "dependencies");
        mDevDependencies = parse_dependencies(value, "dev-dependencies");

        check_dependency_dups(mDependencies, mDevDependencies);

        // parse [profile] and [profile.debug]
        mProfileDefault.config = parse_profile_options(value, "profile");

        const auto profile = find_or(value, "profile", {});
        mProfileDebug.config = parse_profile_options(profile, "debug");
        mProfileRelease.config = parse_profile_options(profile, "release");

        // parse [target.<cfg>.profile]
        for (const auto& [condition_str, config] : get_table(value, "target")) {
            if (!config.contains("profile")) {
                continue;
            }

            const auto condition = core::parse_cfg(condition_str);

            mProfileDefault.conditional_configs.push_back({
                .condition = condition,
                .config = parse_profile_options(config, "profile"),
            });

            const auto profile = get_table(config, "profile");
            if (profile.contains("debug")) {
                mProfileDebug.conditional_configs.push_back({
                    .condition = condition,
                    .config = parse_profile_options(profile, "debug"),
                });
            }
            if (profile.contains("release")) {
                mProfileRelease.conditional_configs.push_back({
                    .condition = condition,
                    .config = parse_profile_options(profile, "release"),
                });
            }
        }

        // set defaults
        set_defaults_();
    } catch (const std::out_of_range& e) {
        throw Error { e.what() };
    } catch (const toml::exception& e) {
        throw Error { fmt::format("invalid manifest format at {}", e.what()) };
    }
}

void Manifest::set_defaults_()
{
    // TODO
    /*
    const bool ubsan_present
        = any_of(std::array { &mProfileDebug, &mProfileDebug, &mProfileRelease }, [](const ProfileOptions* profile) {
              return profile->config.ubsan.has_value()
                  || any_of(profile->conditional_configs, [](const auto& cc) { return cc.config.ubsan.has_value(); });
          });
    if (ubsan_present) {
        return;
    }

    // in debug profile, enable ubsan if not msvc
    mProfileDebug.conditional_configs.push_back({
        .condition = core::CfgNot { { core::cfg::Compiler::msvc } },
        .config = { .ubsan = true },
    });
    */
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

    // add .gitignore
    write(dir / ".gitignore", R"(# ignore build and clangd .cache
.cache
.build
build
)");
}
