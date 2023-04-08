#include "cppship/cmd/lint.h"

#include <string_view>

#include <boost/process/system.hpp>
#include <fmt/core.h>

#include "cppship/util/repo.h"

constexpr std::string_view kLintCmd = "clang-tidy";

using namespace cppship;

int cmd::run_lint(const LintOptions& options)
{
    const auto files = options.all ? list_all_files() : list_changed_files(kRepoHead);

    int r = 0;

    for (const auto& file : files) {
        if (file.extension() != ".cpp") {
            continue;
        }

        fmt::print("lint file: {}\n", file.c_str());

        const int e = boost::process::system(fmt::format("{} {} -p build", kLintCmd, file.c_str()).c_str());
        if (e != 0) {
            r = EXIT_FAILURE;
        }
    }

    return r;
}