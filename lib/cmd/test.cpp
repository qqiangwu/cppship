#include "cppship/cmd/test.h"

#include <cstdlib>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <spdlog/spdlog.h>

#include "cppship/cmake/naming.h"
#include "cppship/cmd/build.h"
#include "cppship/core/workspace.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

using namespace cppship;

namespace {

void valid_options(const cmd::TestOptions& options)
{
    if (options.name && options.name_regex) {
        throw Error { "testname and -R should not be specified both" };
    }
}

}

int cmd::run_test(const TestOptions& options)
{
    valid_options(options);

    BuildContext ctx(options.profile);
    BuildOptions build_opts { .profile = options.profile };
    if (options.name && !options.rerun_failed) {
        const auto layouts = ctx.workspace.layouts()
            | ranges::views::filter([&](const Layout& layout) { return layout.test(*options.name).has_value(); })
            | ranges::to<std::vector>();
        if (layouts.empty()) {
            throw Error { fmt::format("test `{}` not found", *options.name) };
        }
        if (layouts.size() > 1) {
            throw Error { fmt::format("too many tests with name {}, eg. {}, {}",
                *options.name,
                layouts.front().package(),
                layouts.back().package()) };
        }

        build_opts.cmake_target = cmake::NameTargetMapper(layouts.front().package()).test(*options.name);
    } else {
        build_opts.package = options.package;
        build_opts.groups.insert(BuildGroup::tests);
    }

    const int result = run_build(build_opts);
    if (result != 0) {
        return EXIT_FAILURE;
    }

    ScopedCurrentDir guard(ctx.profile_dir);

    auto cmd = fmt::format("ctest --output-on-failure");
    if (options.rerun_failed) {
        cmd += " --rerun-failed";
    } else if (build_opts.cmake_target) {
        cmd += fmt::format(" -R '^{}$'", *build_opts.cmake_target);
    } else if (options.name_regex) {
        cmd += fmt::format(" -R {}", *options.name_regex);
        if (options.package) {
            cmd += fmt::format(" -L '^{}$'", *options.package);
        }
    } else if (options.package) {
        cmd += fmt::format(" -L '^{}$'", *options.package);
    }

    status("test", "{}", cmd);
    return boost::process::system(cmd, boost::process::shell);
}
