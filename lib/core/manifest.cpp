#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/fs.h"

#include <stdexcept>
#include <toml.hpp>
#include <toml/get.hpp>
#include <toml/value.hpp>

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

    for (const auto std : { CxxStd::cxx11, CxxStd::cxx14, CxxStd::cxx17, CxxStd::cxx20, CxxStd::cxx23 }) {
        if (val == static_cast<int>(std)) {
            return std;
        }
    }

    throw Error { fmt::format("manifest invalid std {}", val) };
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

        const auto deps = get(value, "dependencies");
        for (const auto& [name, dep] : deps.as_table()) {
            auto& pkg = mDependencies.emplace_back();

            pkg.package = name;

            if (dep.is_string()) {
                pkg.version = dep.as_string();
            } else if (dep.is_table()) {
                pkg.version = toml::find<std::string>(dep, "version");
                pkg.components = toml::find<std::vector<std::string>>(dep, "components");
            } else {
                throw Error { fmt::format("invalid dependency {} in manifest", name) };
            }
        }
    } catch (const std::out_of_range& e) {
        throw Error { e.what() };
    } catch (const toml::exception& e) {
        throw Error { fmt::format("invalid manifest format: {}", e.what()) };
    }
}