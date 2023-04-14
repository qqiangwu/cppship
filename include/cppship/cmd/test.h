#pragma once

#include <optional>
#include <string>

namespace cppship::cmd {

struct TestOptions {
    std::optional<std::string> target;
};

int run_test(const TestOptions& options);

}