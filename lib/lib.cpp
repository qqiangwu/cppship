#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

#include <argparse/argparse.hpp>
#include <gsl/narrow>

#include "cppship/cmd/build.h"
#include "cppship/cmd/fmt.h"
#include "cppship/cmd/lint.h"
#include "cppship/cppship.h"
#include "cppship/exception.h"

namespace cppship {

int run(std::span<char*> args)
{
    argparse::ArgumentParser app("cppship");

    argparse::ArgumentParser cmd_lint("lint");
    cmd_lint.add_description("run clang-tidy on the code");

    cmd_lint.add_argument("-a", "--all").help("run on all code or delta").default_value(false);
    cmd_lint.add_argument("-j", "--jobs")
        .help("concurrent jobs, default is cpu cores")
        .default_value(gsl::narrow_cast<int>(std::thread::hardware_concurrency()));

    app.add_subparser(cmd_lint);

    argparse::ArgumentParser cmd_fmt("fmt");
    cmd_fmt.add_description("run clang-format on the code");

    cmd_fmt.add_argument("-a", "--all").help("run on all code or delta").default_value(false);
    cmd_fmt.add_argument("-f", "--fix").help("fix or check-only(default").default_value(false);

    app.add_subparser(cmd_fmt);

    try {
        app.parse_args(gsl::narrow_cast<int>(args.size()), args.data());
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << app;
        return EXIT_FAILURE;
    }

    try {
        if (app.is_subcommand_used("fmt")) {
            return cmd::run_fmt({
                .all = cmd_fmt.get<bool>("all"),
                .fix = cmd_fmt.get<bool>("fix"),
            });
        }

        if (app.is_subcommand_used("lint")) {
            return cmd::run_lint({ .all = cmd_lint.get<bool>("all"), .max_concurrency = cmd_lint.get<int>("jobs") });
        }

        std::cerr << app << std::endl;
    } catch (const CmdNotFound& e) {
        fmt::print("{} required, please install it first\n", e.cmd());
    } catch (const Error& e) {
        fmt::print("execute failed: {}\n", e.what());
    }

    return EXIT_FAILURE;
}

} // namespace cppship