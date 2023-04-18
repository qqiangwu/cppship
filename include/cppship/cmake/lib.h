#pragma once

#include <optional>
#include <ostream>
#include <set>

#include "cppship/cmake/dep.h"
#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"

#include <gsl/pointers>

namespace cppship::cmake {

struct LibDesc {
    std::string name;
    fs::path include_dir;
    std::optional<fs::path> source_dir;
    std::vector<Dep> deps;
    std::vector<std::string> definitions;
};

class CmakeLib {
public:
    explicit CmakeLib(LibDesc desc);

    bool is_interface() const { return mSources.empty(); }

    void build(std::ostream& out) const;

private:
    std::string mName;
    fs::path mIncludeDir;
    std::optional<fs::path> mSourceDir;
    std::vector<std::string> mSources;
    std::vector<Dep> mDeps;
    std::vector<std::string> mDefinitions;
};

}