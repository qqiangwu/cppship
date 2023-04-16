#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/fs.h"

#include <stdexcept>

#include <fmt/os.h>
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
        mDefinitions = toml::find_or<std::vector<std::string>>(package, "definitions", {});

        const auto deps = toml::find_or<std::unordered_map<std::string, toml::value>>(value, "dependencies", {});
        for (const auto& [name, dep] : deps) {
            auto& pkg = mDependencies.emplace_back();

            pkg.package = name;

            if (dep.is_string()) {
                pkg.version = dep.as_string();
            } else if (dep.is_table()) {
                pkg.version = toml::find<std::string>(dep, "version");
                pkg.components = toml::find_or<std::vector<std::string>>(dep, "components", {});
                for (const auto& [key, val] :
                    toml::find_or<std::unordered_map<std::string, toml::value>>(dep, "options", {})) {
                    pkg.options.emplace(key, get_option_value(val));
                }
            } else {
                throw Error { fmt::format("invalid dependency {} in manifest", name) };
            }
        }
    } catch (const std::out_of_range& e) {
        throw Error { e.what() };
    } catch (const toml::exception& e) {
        throw Error { fmt::format("invalid manifest format at {}", e.what()) };
    }
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
}
