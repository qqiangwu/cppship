#pragma once

#include <string>

namespace cppship::cmd {

struct RunOptions {
    std::string args;
};

int run_run(const RunOptions& options);

}