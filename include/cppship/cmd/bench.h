#pragma once

#include <optional>
#include <string>

#include "cppship/core/profile.h"

namespace cppship::cmd {

struct BenchOptions {
    Profile profile = Profile::release;
    std::optional<std::string> target;
    std::optional<std::string> package;
};

int run_bench(const BenchOptions& options);

}
