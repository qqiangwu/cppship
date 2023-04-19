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

namespace {

void validate_bin(const fs::path& root, const std::string_view name, const std::optional<std::string>& bin)
{
    if (!bin || *bin == name) {
        if (!fs::exists(root / kSrcPath / "main.cpp")) {
            throw Error { fmt::format("binary src/main.cpp is not found") };
        }
    } else if (bin) {
        if (!fs::exists(root / kSrcPath / "bin" / *bin)) {
            throw Error { fmt::format("binary src/bin/{}.cpp not found", *bin) };
        }
    }
}

}

int cmd::run_run(const RunOptions& options)
{
    BuildContext ctx(options.profile);
    Manifest manifest(ctx.metafile);

    validate_bin(ctx.root, manifest.name(), options.bin);

    const auto bin = options.bin.value_or(fmt::format("{}", manifest.name()));
    const auto target = options.bin.value_or(fmt::format("{}_bin", manifest.name()));
    const int result = run_build({ .profile = options.profile, .target = target });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    const auto bin_file = ctx.profile_dir / bin;
    if (!fs::exists(bin_file)) {
        warn("no binary to run: {}", bin_file.string());
        return EXIT_SUCCESS;
    }

    const auto cmd = fmt::format("{} {}", bin_file.string(), options.args);
    status("run", "{}", cmd);
    return boost::process::system(cmd);
}