#include "cppship/cmd/fmt.h"

#include <string_view>

#include <boost/process/system.hpp>
#include <fmt/core.h>

#include "cppship/util/cmd.h"
#include "cppship/util/log.h"
#include "cppship/util/repo.h"

constexpr std::string_view kFmtCmd = "clang-format";

using namespace cppship;

namespace pr = boost::process;

int cmd::run_fmt(const FmtOptions& options)
{
    require_cmd(kFmtCmd);

    const auto files = options.all
        ? list_all_files()
        : list_changed_files({ .cached_only = options.cached_only, .commit = options.commit });

    int exit_code = 0;

    status("format", "run clang-format");
    for (const auto& file : files) {
        status("format", "{}", file.string());

        const int err
            = pr::system(fmt::format("{} {} {}", kFmtCmd, file.string(), (options.fix ? "-i" : "-n --Werror")));
        if (err != 0) {
            exit_code = EXIT_FAILURE;
        }
    }

    if (exit_code == 0) {
        status("format", "all files are formated");
    }

    return exit_code;
}