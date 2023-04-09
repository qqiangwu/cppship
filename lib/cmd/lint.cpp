#include "cppship/cmd/lint.h"

#include <algorithm>
#include <cstdlib>
#include <string_view>
#include <thread>
#include <vector>

#include <BS_thread_pool_light.hpp>
#include <boost/process/system.hpp>
#include <fmt/core.h>
#include <range/v3/algorithm/any_of.hpp>

#include "cppship/util/repo.h"

constexpr std::string_view kLintCmd = "clang-tidy";

using namespace cppship;

int cmd::run_lint(const LintOptions& options)
{
    const auto files = options.all ? list_all_files() : list_changed_files(kRepoHead);
    const auto concurrency = [max_concurrency = options.max_concurrency] {
        if (max_concurrency <= 0) {
            return std::thread::hardware_concurrency();
        }

        return std::min(std::thread::hardware_concurrency(), static_cast<unsigned>(max_concurrency));
    }();

    BS::thread_pool_light pool(concurrency);
    std::vector<std::future<int>> tasks;
    tasks.reserve(files.size());

    for (const auto& file : files) {
        if (file.extension() != ".cpp") {
            continue;
        }

        auto fut = pool.submit([file] {
            fmt::print("lint file: {}\n", file.c_str());

            return boost::process::system(
                fmt::format("{} {} -p build --warnings-as-errors=true", kLintCmd, file.c_str()).c_str());
        });
        tasks.push_back(std::move(fut));
    }

    pool.wait_for_tasks();

    return ranges::any_of(tasks, [](auto& fut) { return fut.get() != 0; }) ? EXIT_FAILURE : EXIT_SUCCESS;
}