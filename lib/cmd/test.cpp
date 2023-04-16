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
    const int result = run_build({ .profile = options.profile });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    BuildContext ctx(options.profile);
    ScopedCurrentDir guard(ctx.profile_dir);

    const auto cmd = fmt::format("ctest --output-on-failure");
    status("test", "{}", cmd);
    return boost::process::system(cmd);
}