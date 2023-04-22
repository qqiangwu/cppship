#pragma once

#include <string>

namespace cppship::cmd {

struct FmtOptions {
    bool all = false;
    bool cached_only = false;
    bool fix = false;
    std::string commit;
};

int run_fmt(const FmtOptions& options);

}