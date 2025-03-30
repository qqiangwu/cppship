#include "cppship/cmd/test.h"

#include <cstdlib>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <spdlog/spdlog.h>

#include "cppship/cmake/naming.h"
#include "cppship/cmd/build.h"
#include "cppship/core/workspace.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

using namespace cppship;

int cmd::run_test(const TestOptions& options)
{
    cmake::NameTargetMapper mapper;
    BuildContext ctx(options.profile);
    const auto& layout = enforce_default_package(ctx.workspace);

    BuildOptions build_opts { .profile = options.profile };
    if (options.target && !options.rerun_failed) {
        if (!layout.test(*options.target)) {
            throw Error { fmt::format("test `{}` not found", *options.target) };
        }

        build_opts.target = mapper.test(*options.target);
    } else {
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
    } else if (build_opts.target) {
        cmd += fmt::format(" -R '^{}$'", *build_opts.target);
    } else if (options.name_regex) {
        cmd += fmt::format(" -R {}", *options.name_regex);
    }

    status("test", "{}", cmd);
    return boost::process::system(cmd, boost::process::shell);
}
