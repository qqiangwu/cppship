#include <cstdlib>
#include <iostream>
#include <list>
#include <string>
#include <thread>

#include <argparse/argparse.hpp>
#include <boost/algorithm/string/join.hpp>
#include <gsl/narrow>
#include <range/v3/algorithm/find_if.hpp>
#include <spdlog/spdlog.h>

#include "cppship/cppship.h"
#include "cppship/exception.h"
#include "cppship/util/log.h"

#ifndef CPPSHIP_VERSION
#error "CPPSHIP_VERSION is required"
#endif

using namespace cppship;
using argparse::ArgumentParser;

using CommandRunner = int (*)(const ArgumentParser&);

struct SubCommand {
    std::string name;
    ArgumentParser parser;
    CommandRunner cmd_runner;

    SubCommand(std::string_view cmd, const ArgumentParser& common, CommandRunner runner)
        : name(cmd)
        , parser(name, "", argparse::default_arguments::help)
        , cmd_runner(runner)
    {
        parser.add_parents(common);
    }

    int run() { return cmd_runner(parser); }
};

Profile get_profile(const ArgumentParser& cmd)
{
    if (cmd.is_used("profile")) {
        return parse_profile(cmd.get<std::string>("profile"));
    }

    return cmd.get<bool>("r") ? Profile::release : Profile::debug;
}

int get_concurrency(const ArgumentParser& cmd)
{
    const auto jobs = cmd.get<int>("jobs");
    if (jobs <= 0) {
        return gsl::narrow_cast<int>(std::thread::hardware_concurrency());
    }

    return std::min<int>(jobs, gsl::narrow_cast<int>(std::thread::hardware_concurrency()));
}

CxxStd parse_cxx_std(const int value)
{
    const auto cxx = to_cxx_std(value);
    if (!cxx) {
        throw Error { fmt::format("invalid std argument: {}", value) };
    }

    return *cxx;
}

std::list<SubCommand> build_commands(const ArgumentParser& common)
{
    std::list<SubCommand> commands;

    // lint
    auto& lint = commands.emplace_back("lint", common, [](const ArgumentParser& cmd) {
        return cmd::run_lint({ .all = cmd.get<bool>("all"),
            .cached_only = cmd.get<bool>("cached"),
            .max_concurrency = get_concurrency(cmd) });
    });

    lint.parser.add_description("run clang-tidy on the code");
    lint.parser.add_argument("-a", "--all")
        .help("run on all code, default by diff")
        .default_value(false)
        .implicit_value(true);
    lint.parser.add_argument("--cached").help("only lint staged changes").default_value(false).implicit_value(true);
    lint.parser.add_argument("-j", "--jobs")
        .help("concurrent jobs, default is cpu cores")
        .default_value(gsl::narrow_cast<int>(std::thread::hardware_concurrency()))
        .scan<'d', int>();

    // fmt
    auto& fmt = commands.emplace_back("fmt", common, [](const ArgumentParser& cmd) {
        return cmd::run_fmt({
            .all = cmd.get<bool>("all"),
            .cached_only = cmd.get<bool>("cached"),
            .fix = cmd.get<bool>("fix"),
        });
    });

    fmt.parser.add_description("run clang-format on the code");
    fmt.parser.add_argument("-a", "--all").help("run on all code or delta").default_value(false).implicit_value(true);
    fmt.parser.add_argument("--cached").help("only lint staged changes").default_value(false).implicit_value(true);
    fmt.parser.add_argument("-f", "--fix").help("fix or check-only(default)").default_value(false).implicit_value(true);

    // build
    auto& build = commands.emplace_back("build", common, [](const ArgumentParser& cmd) {
        return cmd::run_build(
            { .max_concurrency = get_concurrency(cmd), .profile = get_profile(cmd), .dry_run = cmd.get<bool>("-d") });
    });

    build.parser.add_description("build the project");
    build.parser.add_argument("-j", "--jobs")
        .help("concurrent jobs, default is cpu cores")
        .default_value(gsl::narrow_cast<int>(std::thread::hardware_concurrency()))
        .scan<'d', int>();
    build.parser.add_argument("-r").help("build in release mode").default_value(false).implicit_value(true);
    build.parser.add_argument("-d")
        .help("dry-run, generate compile_commands.json")
        .default_value(false)
        .implicit_value(true);
    build.parser.add_argument("--profile").help("build with specific profile").default_value(kProfileDebug);

    // clean
    auto& clean = commands.emplace_back("clean", common, [](const ArgumentParser&) { return cmd::run_clean({}); });

    clean.parser.add_description("clean build");

    // install
    auto& install = commands.emplace_back(
        "install", common, [](const ArgumentParser& cmd) { return cmd::run_install({ .profile = get_profile(cmd) }); });

    install.parser.add_description("install binary if exists");
    install.parser.add_argument("-r").help("build in release mode").default_value(false).implicit_value(true);
    install.parser.add_argument("--profile")
        .help("build with specific profile")
        .metavar("profile")
        .default_value(kProfileDebug);

    // run
    auto& run = commands.emplace_back("run", common, [](const ArgumentParser& cmd) {
        const auto remaining = cmd.present<std::vector<std::string>>("--").value_or(std::vector<std::string> {});
        return cmd::run_run(
            { .profile = get_profile(cmd), .args = boost::join(remaining, " "), .bin = cmd.present("--bin") });
    });

    run.parser.add_description("run binary");
    run.parser.add_argument("-r").help("build in release mode").default_value(false).implicit_value(true);
    run.parser.add_argument("--profile")
        .help("build with specific profile")
        .metavar("profile")
        .default_value(kProfileDebug);
    run.parser.add_argument("--bin").help("name of binary to run").metavar("name");
    run.parser.add_argument("--").help("extra args").metavar("args").remaining();

    // test
    auto& test = commands.emplace_back(
        "test", common, [](const ArgumentParser& cmd) { return cmd::run_test({ .profile = get_profile(cmd) }); });

    test.parser.add_description("run tests");
    test.parser.add_argument("-r").help("build in release mode").default_value(false).implicit_value(true);
    test.parser.add_argument("--profile")
        .help("build with specific profile")
        .metavar("profile")
        .default_value(kProfileDebug);

    // init
    auto& init = commands.emplace_back("init", common, [](const ArgumentParser& cmd) {
        cmd::InitOptions options {
            .vcs = cmd.get<bool>("vcs"), .lib = cmd.get<bool>("lib"), .bin = cmd.get<bool>("bin")
        };
        if (const std::string& dir = cmd.get("path"); dir == ".") {
            options.dir = fs::current_path();
        } else {
            options.dir = fs::current_path() / dir;
        }
        if (cmd.is_used("name")) {
            options.name = cmd.get("name");
        }
        if (cmd.is_used("std")) {
            options.std = parse_cxx_std(cmd.get<int>("std"));
        }

        return run_init(options);
    });

    init.parser.add_description("create a new package");
    init.parser.add_argument("path").help("the path to create package");
    init.parser.add_argument("--vcs").help("init a git or not").default_value(false).implicit_value(true);
    init.parser.add_argument("--lib").help("use a library template").default_value(false).implicit_value(true);
    init.parser.add_argument("--bin").help("use a bin template (default)").default_value(false).implicit_value(true);
    init.parser.add_argument("--name").help("package name, default to directory name").metavar("name");
    init.parser.add_argument("--std")
        .help("cpp std to use, valid values are 11/14/17/20/23")
        .default_value(fmt::underlying(CxxStd::cxx17))
        .metavar("cxxstd")
        .scan<'d', int>();

    return commands;
}

int main(int argc, const char* argv[])
try {
    spdlog::set_pattern("%v");

    ArgumentParser common("common", "", argparse::default_arguments::none);
    common.add_argument("-V", "--verbose").help("show verbose log").default_value(false).implicit_value(true);
    common.add_argument("-q", "--quiet").help("do not pring log messages").default_value(false).implicit_value(true);

    ArgumentParser app("cppship", CPPSHIP_VERSION);
    app.add_parents(common);

    auto sub_commands = build_commands(common);
    for (auto& cmd : sub_commands) {
        app.add_subparser(cmd.parser);
    }

    try {
        app.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        error("{}\n\ntry -h to get usage", err.what());
        return EXIT_FAILURE;
    }

    try {
        auto iter = ranges::find_if(sub_commands, [&app](const auto& cmd) { return app.is_subcommand_used(cmd.name); });
        if (iter == sub_commands.end()) {
            std::cerr << app << std::endl;
            return EXIT_SUCCESS;
        }

        if (app.get<bool>("-V") || iter->parser.get<bool>("-V")) {
            spdlog::set_level(spdlog::level::debug);
        }
        if (app.get<bool>("-q") || iter->parser.get<bool>("-q")) {
            spdlog::set_level(spdlog::level::warn);
        }

        return iter->run();
    } catch (const CmdNotFound& e) {
        error("{} required, please install it first", e.cmd());
    } catch (const Error& e) {
        error("{}", e.what());
    }

    return EXIT_FAILURE;
} catch (const std::exception& e) {
    error("unknown error {}", e.what());
    return EXIT_FAILURE;
}