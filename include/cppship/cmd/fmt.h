#pragma once

namespace cppship::cmd {

struct FmtOptions {
    bool all = false;
    bool cached_only = false;
    bool fix = false;
};

int run_fmt(const FmtOptions& options);

}