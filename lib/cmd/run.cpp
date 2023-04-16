#include <cstdlib>
#include <thread>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <spdlog/spdlog.h>

#include "cppship/cmd/build.h"
#include "cppship/cmd/run.h"
#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"
#include "cppship/util/repo.h"

using namespace cppship;

int cmd::run_run(const RunOptions& options)
{
    BuildContext ctx(options.profile);
    Manifest manifest(ctx.metafile);
    if (!fs::exists(ctx.root / kSrcPath)) {
        warn("no binary to run");
        return EXIT_SUCCESS;
    }

    const int result = run_build({ .profile = options.profile, .target = fmt::format("{}_bin", manifest.name()) });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    const auto bin_file = ctx.profile_dir / manifest.name();
    if (!fs::exists(bin_file)) {
        warn("no binary to run");
        return EXIT_SUCCESS;
    }

    const auto cmd = fmt::format("{} {}", bin_file.string(), options.args);
    status("run", "{}", cmd);
    return boost::process::system(cmd);
}