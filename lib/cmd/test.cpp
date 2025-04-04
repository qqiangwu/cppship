#include "cppship/cmd/test.h"

#include <cstdlib>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <range/v3/algorithm/all_of.hpp>
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
    if (options.target && options.name_regex) {
        throw Error { "testname and -R should not be specified both" };
    }
}

}

int cmd::run_test(const TestOptions& options)
{
    valid_options(options);

    cmake::NameTargetMapper mapper;

    BuildContext ctx(options.profile);
    BuildOptions build_opts { .profile = options.profile };
    if (options.target && !options.rerun_failed) {
        if (ranges::all_of(
                ctx.workspace.layouts(), [&](const auto& layout) { return !layout.test(*options.target); })) {
            throw Error { fmt::format("test `{}` not found", *options.target) };
        }

        build_opts.cmake_target = mapper.test(*options.target);
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
