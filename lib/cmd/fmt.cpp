#include "cppship/cmd/fmt.h"

#include <iostream>
#include <string_view>

#include <boost/process/system.hpp>
#include <fmt/core.h>

#include "cppship/util/repo.h"

constexpr std::string_view kFmtCmd = "clang-format";

using namespace cppship;

int cmd::run_fmt(const FmtOptions& options)
{
    const auto files = options.all ? list_all_files() : list_changed_files(kRepoHead);

    int exit_code = 0;

    for (const auto& file : files) {
        std::cout << "format file: " << file << std::endl;

        const int err = boost::process::system(
            fmt::format("{} {} {}", kFmtCmd, file.c_str(), (options.fix ? "-i" : "-n --Werror")).c_str());
        if (err != 0) {
            exit_code = EXIT_FAILURE;
        }
    }

    if (exit_code == 0) {
        std::cout << "all files are formated" << std::endl;
    }

    return exit_code;
}