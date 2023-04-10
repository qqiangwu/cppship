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

        const auto deps = get(value, "dependencies");
        for (const auto& [name, dep] : deps.as_table()) {
            mDependencies.push_back({ .package = name, .version = dep.as_string().str });
        }
    } catch (const std::out_of_range& e) {
        throw Error { e.what() };
    } catch (const toml::exception& e) {
        throw Error { fmt::format("invalid manifest format: {}", e.what()) };
    }
}