#pragma once

#include <filesystem>
#include <set>

#include "cppship/util/fs.h"

namespace cppship::cmd {

struct FmtOptions {
    bool all = false;
    bool fix = false;
};

int run_fmt(const FmtOptions& options);

}