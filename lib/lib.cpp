#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <argparse/argparse.hpp>
#include <gsl/narrow>
#include <range/v3/algorithm/find_if.hpp>

#include "cppship/cmd/build.h"
#include "cppship/cmd/clean.h"
#include "cppship/cmd/fmt.h"
#include "cppship/cmd/install.h"
#include "cppship/cmd/lint.h"
#include "cppship/cmd/run.h"
#include "cppship/cmd/test.h"
#include "cppship/cppship.h"
#include "cppship/exception.h"

using argparse::ArgumentParser;

namespace cppship {

using CommandRunner = int (*)(const ArgumentParser&);

struct SubCommand {
    std::string name;
    ArgumentParser parser;
    CommandRunner cmd_runner;

    SubCommand(std::string_view cmd, CommandRunner runner)
        : name(cmd)
        , parser(name)
        , cmd_runner(runner)
    {
    }

    int run() { return cmd_runner(parser); }
};

std::vector<SubCommand> build_commands()
{
    std::vector<SubCommand> commands;

    // lint
    auto& lint = commands.emplace_back("lint", [](const ArgumentParser& cmd) {
        return cmd::run_lint({ .all = cmd.get<bool>("all"), .max_concurrency = cmd.get<int>("jobs") });
    });

    lint.parser.add_description("run clang-tidy on the code");

    lint.parser.add_argument("-a", "--all").help("run on all code or delta").default_value(false).implicit_value(true);
    lint.parser.add_argument("-j", "--jobs")
        .help("concurrent jobs, default is cpu cores")
        .default_value(gsl::narrow_cast<int>(std::thread::hardware_concurrency()));

    // fmt
    auto& fmt = commands.emplace_back("fmt", [](const ArgumentParser& cmd) {
        return cmd::run_fmt({
            .all = cmd.get<bool>("all"),
            .fix = cmd.get<bool>("fix"),
        });
    });

    fmt.parser.add_description("run clang-format on the code");

    fmt.parser.add_argument("-a", "--all").help("run on all code or delta").default_value(false).implicit_value(true);
    fmt.parser.add_argument("-f", "--fix").help("fix or check-only(default").default_value(false).implicit_value(true);

    // build
    auto& build = commands.emplace_back(
        "build", [](const ArgumentParser& cmd) { return cmd::run_build({ .max_concurrency = cmd.get<int>("jobs") }); });

    build.parser.add_description("build the project");

    build.parser.add_argument("-j", "--jobs")
        .help("concurrent jobs, default is cpu cores")
        .default_value(gsl::narrow_cast<int>(std::thread::hardware_concurrency()));

    // clean
    auto& clean = commands.emplace_back("clean", [](const ArgumentParser&) { return cmd::run_clean({}); });
    clean.parser.add_description("clean build");

    // install
    auto& install = commands.emplace_back("install", [](const ArgumentParser&) { return cmd::run_install({}); });
    install.parser.add_description("install binary if exists");

    // run
    auto& run = commands.emplace_back("run", [](const ArgumentParser&) { return cmd::run_run({}); });

    run.parser.add_description("run binary");

    // test
    auto& test = commands.emplace_back("test", [](const ArgumentParser&) { return cmd::run_test({}); });

    test.parser.add_description("run tests");

    return commands;
}

int run(std::span<const char*> args)
{
    argparse::ArgumentParser app(args[0]);

    auto sub_commands = build_commands();
    for (auto& cmd : sub_commands) {
        app.add_subparser(cmd.parser);
    }

    try {
        app.parse_args(gsl::narrow_cast<int>(args.size()), args.data());
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << "\n\n";
        std::cerr << app;
        return EXIT_FAILURE;
    }

    try {
        auto iter = ranges::find_if(sub_commands, [&app](const auto& cmd) { return app.is_subcommand_used(cmd.name); });
        if (iter == sub_commands.end()) {
            std::cerr << app << std::endl;
            return EXIT_FAILURE;
        }

        return iter->run();
    } catch (const CmdNotFound& e) {
        fmt::print("{} required, please install it first\n", e.cmd());
    } catch (const Error& e) {
        fmt::print("execute failed: {}\n", e.what());
    }

    return EXIT_FAILURE;
}

} // namespace cppship