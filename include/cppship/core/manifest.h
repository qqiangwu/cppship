#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "cppship/util/fs.h"

namespace cppship {

struct DeclaredDependency {
    std::string package;
    std::string version;
    std::vector<std::string> components;
};

class Manifest {
public:
    explicit Manifest(const fs::path& file);

    std::string_view name() const { return mName; }

    std::string_view version() const { return mVersion; }

    const std::vector<DeclaredDependency>& dependencies() const { return mDependencies; }

private:
    std::string mName;
    std::string mVersion;

    std::vector<DeclaredDependency> mDependencies;
};

}