#include <cstdlib>
#include <iostream>
#include <optional>

#include <structopt/app.hpp>

#include "cppship/cmd/fmt.h"
#include "cppship/exception.h"

struct Lint : structopt::sub_command {
    std::optional<bool> all = false;
};
STRUCTOPT(Lint, all);

struct Fmt : structopt::sub_command {
    std::optional<bool> all = false;
    std::optional<bool> fix = false;
};

STRUCTOPT(Fmt, all, fix);

struct CppShip {
    Lint lint;
    Fmt fmt;
};
STRUCTOPT(CppShip, lint, fmt);

namespace cppship {

int run_impl(const CppShip& options)
{
    if (const auto& opt = options.fmt; opt.has_value()) {
        return cmd::run_fmt({
            .all = *opt.all,
            .fix = *opt.fix,
        });
    }

    return EXIT_SUCCESS;
}

int run(int argc, char* argv[])
{
    try {
        auto options = structopt::app("cppship").parse<CppShip>(argc, argv);

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