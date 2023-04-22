#include "cppship/cmd/lint.h"

#include <cstdlib>
#include <string_view>
#include <thread>
#include <vector>

#include <BS_thread_pool_light.hpp>
#include <boost/process/system.hpp>
#include <fmt/core.h>
#include <range/v3/algorithm/any_of.hpp>

#include "cppship/util/cmd.h"
#include "cppship/util/log.h"
#include "cppship/util/repo.h"

constexpr std::string_view kLintCmd = "clang-tidy";

using namespace cppship;

int cmd::run_lint(const LintOptions& options)
{
    require_cmd(kLintCmd);

    ScopedCurrentDir guard(get_project_root());
    const auto files = options.all
        ? list_all_files()
        : list_changed_files({ .cached_only = options.cached_only, .commit = options.commit });
    const auto concurrency = options.max_concurrency;

    BS::thread_pool_light pool(concurrency);
    std::vector<std::future<int>> tasks;
    tasks.reserve(files.size());

    status("lint", "run clang-tidy");
    for (const auto& file : files) {
        if (file.extension() != ".cpp") {
            continue;
        }

        auto fut = pool.submit([file] {
            status("lint", "{}", file.string());

            return boost::process::system(
                fmt::format("{} {} -p build --warnings-as-errors=true --quiet", kLintCmd, file.c_str()).c_str(),
                boost::process::std_err > boost::process::null);
        });
        tasks.push_back(std::move(fut));
    }

    pool.wait_for_tasks();

    return ranges::any_of(tasks, [](auto& fut) { return fut.get() != 0; }) ? EXIT_FAILURE : EXIT_SUCCESS;
}