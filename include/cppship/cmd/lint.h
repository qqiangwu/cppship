#pragma once

namespace cppship::cmd {

struct LintOptions {
    bool all = false;
    bool cached_only = false;
    int max_concurrency = 0;
};

int run_lint(const LintOptions& options);

}