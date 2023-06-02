#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include "cppship/core/dependency.h"
#include "cppship/core/profile.h"
#include "cppship/util/fs.h"

namespace cppship {

enum class CxxStd { cxx11 = 11, cxx14 = 14, cxx17 = 17, cxx20 = 20, cxx23 = 23 };

inline constexpr auto format_as(CxxStd std) { return fmt::underlying(std); }

inline constexpr std::optional<CxxStd> to_cxx_std(int val) noexcept
{
    for (const auto std : { CxxStd::cxx11, CxxStd::cxx14, CxxStd::cxx17, CxxStd::cxx20, CxxStd::cxx23 }) {
        if (val == static_cast<int>(std)) {
            return std;
        }
    }

    return std::nullopt;
}

class Manifest {
public:
    explicit Manifest(const fs::path& file);

    std::string_view name() const { return mName; }

    std::string_view version() const { return mVersion; }

    CxxStd cxx_std() const { return mCxxStd; }

    const std::vector<DeclaredDependency>& dependencies() const { return mDependencies; }

    const std::vector<DeclaredDependency>& dev_dependencies() const { return mDevDependencies; }

    const ProfileOptions& default_profile() const { return mProfileDefault; }
    const ProfileOptions& profile(Profile prof) const;

private:
    void set_defaults_();

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

void generate_manifest(std::string_view name, CxxStd std, const fs::path& dir);

}