#pragma once

#include <optional>
#include <string>

#include "cppship/util/fs.h"

namespace cppship::cmd {

struct IndexOptions {
    std::string operation;
    std::optional<std::string> name;
    std::optional<std::string> path;
    std::optional<std::string> git;
};

int run_index(const IndexOptions& options);

}
