#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include "cppship/core/dependency.h"
#include "cppship/core/profile.h"

namespace cppship {

enum class CxxStd : std::uint8_t { cxx11 = 11, cxx14 = 14, cxx17 = 17, cxx20 = 20, cxx23 = 23 };

constexpr auto format_as(CxxStd std) { return fmt::underlying(std); }

constexpr std::optional<CxxStd> to_cxx_std(int val) noexcept
{
    for (const auto std : { CxxStd::cxx11, CxxStd::cxx14, CxxStd::cxx17, CxxStd::cxx20, CxxStd::cxx23 }) {
        if (val == static_cast<int>(std)) {
            return std;
        }
    }

    return std::nullopt;
}

class PackageManifest {
public:
    explicit PackageManifest(const toml::value& value);

    std::string_view name() const { return mName; }

    std::string_view version() const { return mVersion; }

    CxxStd cxx_std() const { return mCxxStd; }

    const std::vector<DeclaredDependency>& dependencies() const { return mDependencies; }

    const std::vector<DeclaredDependency>& dev_dependencies() const { return mDevDependencies; }

    const ProfileOptions& default_profile() const { return mProfileDefault; }
    const ProfileOptions& profile(Profile prof) const;

private:
    std::string mName;
    std::string mVersion;
    CxxStd mCxxStd = CxxStd::cxx17;

    std::vector<DeclaredDependency> mDependencies;
    std::vector<DeclaredDependency> mDevDependencies;

    ProfileOptions mProfileDefault;
    ProfileOptions mProfileDebug;
    ProfileOptions mProfileRelease;
};

class Manifest {
public:
    explicit Manifest(const fs::path& file);

    bool is_workspace() const { return !std::holds_alternative<PackageManifest>(packages_); }

    const PackageManifest* get_if_package() const { return std::get_if<PackageManifest>(&packages_); }

    const PackageManifest* get(const fs::path& p) const
    {
        const auto* map = std::get_if<1>(&packages_);
        if (map == nullptr) {
            return nullptr;
        }

        auto it = map->find(p);
        return it == map->end() ? nullptr : &it->second;
    }

    const auto& list_packages() const { return std::get<1>(packages_); }

    const std::vector<DeclaredDependency>& dependencies() const { return mDependencies; }

    const std::vector<DeclaredDependency>& dev_dependencies() const { return mDevDependencies; }

private:
    const PackageManifest& as_package_() const { return std::get<PackageManifest>(packages_); }

private:
    std::variant<std::monostate, std::map<fs::path, PackageManifest>, PackageManifest> packages_;

    // merged dependencies for all member packages
    std::vector<DeclaredDependency> mDependencies;
    std::vector<DeclaredDependency> mDevDependencies;
};

void generate_manifest(std::string_view name, CxxStd std, const fs::path& dir);

}
