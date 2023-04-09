#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

#include <gsl/narrow>
#include <structopt/app.hpp>
#include <structopt/sub_command.hpp>

#include "cppship/cmd/build.h"
#include "cppship/cmd/fmt.h"
#include "cppship/cmd/lint.h"
#include "cppship/cppship.h"
#include "cppship/exception.h"

// NOLINTBEGIN(readability-identifier-length): structopt macro has this
struct Fmt : structopt::sub_command {
    std::optional<bool> all = false;
    std::optional<bool> fix = false;
};
STRUCTOPT(Fmt, all, fix);

struct Lint : structopt::sub_command {
    std::optional<bool> all = false;
    std::optional<int> jobs = 0;
};
STRUCTOPT(Lint, all, jobs);

struct Build : structopt::sub_command {
    // build concurrency: default to cpus
    std::optional<int> jobs = 0;

    // build target
    std::optional<std::string> target;

    // build only lib
    std::optional<bool> lib = false;

    // build all binaries
    std::optional<bool> bins = false;

    // build all examples
    std::optional<bool> examples = false;

    // build all tests
    std::optional<bool> tests = false;

    // build all benches
    std::optional<bool> benches = false;

    // build all targets
    std::optional<bool> all_targets = true;
};
STRUCTOPT(Build, jobs, target, lib, bins, examples, tests, benches, all_targets);

struct CppShip {
    Lint lint;
    Fmt fmt;
    Build build;
};
STRUCTOPT(CppShip, lint, fmt, build);
// NOLINTEND(readability-identifier-length)

namespace cppship {

int run_impl(const CppShip& options)
{
    if (const auto& opt = options.fmt; opt.has_value()) {
        return cmd::run_fmt({
            .all = *opt.all,
            .fix = *opt.fix,
        });
    }

    if (const auto& opt = options.lint; opt.has_value()) {
        return cmd::run_lint({ .all = *opt.all, .max_concurrency = *opt.jobs });
    }

    if (const auto& opt = options.build; opt.has_value()) {
        return cmd::run_build({ .max_concurrency = *opt.jobs });
    }

    return EXIT_SUCCESS;
}

int run(std::span<char*> args)
{
    try {
        auto options = structopt::app("cppship").parse<CppShip>(gsl::narrow_cast<int>(args.size()), args.data());

        return run_impl(options);
    } catch (const structopt::exception& e) {
        std::cout << e.what() << "\n";
        std::cout << e.help();
    } catch (const CmdNotFound& e) {
        fmt::print("{} required, please install it first\n", e.cmd());
    } catch (const Error& e) {
        fmt::print("execute failed: {}\n", e.what());
    }

    return EXIT_FAILURE;
}

} // namespace cppship