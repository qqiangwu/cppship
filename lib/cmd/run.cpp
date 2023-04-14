#include <cstdlib>
#include <thread>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <spdlog/spdlog.h>

#include "cppship/cmd/build.h"
#include "cppship/cmd/run.h"
#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"
#include "cppship/util/repo.h"

using namespace cppship;

int cmd::run_run(const RunOptions& options)
{
    const int result = run_build({ .max_concurrency = gsl::narrow_cast<int>(std::thread::hardware_concurrency()) });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    BuildContext ctx;
    Manifest manifest(ctx.metafile);
    const auto bin_file = ctx.profile_dir / manifest.name();
    if (!fs::exists(bin_file)) {
        spdlog::warn("no binary to run");
        return EXIT_SUCCESS;
    }

    const auto cmd = fmt::format("{} {}", bin_file.string(), options.args);
    spdlog::info("run `{}`", cmd);
    return boost::process::system(cmd);
}