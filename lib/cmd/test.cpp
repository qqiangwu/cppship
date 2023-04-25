#include <cstdlib>
#include <thread>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <spdlog/spdlog.h>

#include "cppship/cmd/build.h"
#include "cppship/cmd/test.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

using namespace cppship;

int cmd::run_test(const TestOptions& options)
{
    const int result = run_build({
        .profile = options.profile,
        .groups = { BuildGroup::tests },
    });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    BuildContext ctx(options.profile);
    ScopedCurrentDir guard(ctx.profile_dir);

    auto cmd = fmt::format("ctest --output-on-failure");
    if (options.rerun_failed) {
        cmd += " --rerun-failed";
    } else if (options.target) {
        cmd += fmt::format(" -R {}", *options.target);
    }
    status("test", "{}", cmd);
    return boost::process::system(cmd);
}