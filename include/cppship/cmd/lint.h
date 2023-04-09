#pragma once

namespace cppship::cmd {

struct LintOptions {
    bool all = false;
    int max_concurrency = 0;
};

int run_lint(const LintOptions& options);

}