#pragma once

#include <optional>
#include <string>

#include "cppship/core/profile.h"

namespace cppship::cmd {

struct TestOptions {
    Profile profile = Profile::debug;
    std::optional<std::string> target;
};

int run_test(const TestOptions& options);

}