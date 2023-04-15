#pragma once

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

class Manifest {
public:
    explicit Manifest(const fs::path& file);

    std::string_view name() const { return mName; }

    std::string_view version() const { return mVersion; }

    CxxStd cxx_std() const { return mCxxStd; }

    const std::vector<DeclaredDependency>& dependencies() const { return mDependencies; }

private:
    std::string mName;
    std::string mVersion;
    CxxStd mCxxStd = CxxStd::cxx17;

    std::vector<DeclaredDependency> mDependencies;
};

}