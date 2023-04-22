#pragma once

#include <string>

namespace cppship::cmd {

struct LintOptions {
    bool all = false;
    bool cached_only = false;
    int max_concurrency = 0;
    std::string commit;
};

int run_lint(const LintOptions& options);

}