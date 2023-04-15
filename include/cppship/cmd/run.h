#pragma once

#include <string>

#include "cppship/core/profile.h"

namespace cppship::cmd {

struct RunOptions {
    Profile profile = Profile::debug;
    std::string args;
};

int run_run(const RunOptions& options);

}