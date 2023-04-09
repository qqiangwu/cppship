#include <cstdlib>
#include <iostream>
#include <optional>

#include <structopt/app.hpp>

#include "cppship/cmd/fmt.h"
#include "cppship/cmd/lint.h"
#include "cppship/cppship.h"
#include "cppship/exception.h"

// NOLINTBEGIN(readability-identifier-length): structopt macro has this
struct Fmt : structopt::sub_command {
    std::optional<bool> all = false;
    std::optional<bool> fix = false;
};

struct Lint : structopt::sub_command {
    std::optional<bool> all = false;
    std::optional<int> j = 0;
};
STRUCTOPT(Lint, all);

STRUCTOPT(Fmt, all, fix);

struct CppShip {
    Lint lint;
    Fmt fmt;
};
STRUCTOPT(CppShip, lint, fmt);
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
        return cmd::run_lint({ .all = *opt.all, .max_concurrency = *opt.j });
    }

    return EXIT_SUCCESS;
}

int run(std::span<char*> args)
{
    try {
        // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions): safe narrow conversion
        auto options = structopt::app("cppship").parse<CppShip>(args.size(), args.data());

        return run_impl(options);
    } catch (const structopt::exception& e) {
        std::cout << e.what() << "\n";
        std::cout << e.help();
    } catch (const CmdNotFound& e) {
        std::cout << fmt::format("{} required, please install it first", e.cmd()) << std::endl;
    }

    return EXIT_FAILURE;
}

} // namespace cppship