#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "cppship/util/fs.h"

namespace cppship {

struct DeclaredDependency {
    std::string package;
    std::string version;
    std::vector<std::string> components;
};

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

    const std::vector<std::string>& definitions() const { return mDefinitions; }

    const std::vector<DeclaredDependency>& dependencies() const { return mDependencies; }

private:
    std::string mName;
    std::string mVersion;
    CxxStd mCxxStd = CxxStd::cxx17;

    std::vector<std::string> mDefinitions;
    std::vector<DeclaredDependency> mDependencies;
};

void generate_manifest(std::string_view name, CxxStd std, const fs::path& dir);

}