#pragma once

#include <optional>
#include <string>

#include "cppship/core/profile.h"

namespace cppship::cmd {

struct TestOptions {
    Profile profile = Profile::debug;
    std::optional<std::string> name;
    std::optional<std::string> package;
    std::optional<std::string> name_regex;
    bool rerun_failed = false;
};

int run_test(const TestOptions& options);

}
