#pragma once

#include <optional>
#include <ostream>
#include <set>

#include "cppship/cmake/dep.h"
#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"

namespace cppship::cmake {

struct BinDesc {
    std::string name;
    std::optional<std::string> name_alias;
    std::vector<fs::path> sources;
    std::optional<std::string> include_dir;
    std::optional<std::string> lib;
    std::vector<Dep> deps;
    std::vector<std::string> definitions;
};

class CmakeBin {
public:
    explicit CmakeBin(BinDesc desc);

    void build(std::ostream& out) const;

private:
    BinDesc mDesc;
};

}