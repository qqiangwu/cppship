#pragma once

namespace cppship::cmd {

struct CleanOptions {
    bool all = false;
    bool fix = false;
};

int run_clean(const CleanOptions& options);

}