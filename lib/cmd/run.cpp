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

void validate_options(const cmd::RunOptions& options)
{
    if (options.bin && options.example) {
        throw Error { "should not specify --bin and --example in the same time" };
    }
}

void validate_bin(const fs::path& root, const std::string_view name, const cmd::RunOptions& opt)
{
    if (const auto& bin = opt.bin) {
        if (*bin == name) {
            if (!fs::exists(root / kSrcPath / "main.cpp")) {
                throw Error { fmt::format("binary src/main.cpp is not found") };
            }
            return;
        }

        if (!fs::exists(root / kSrcPath / "bin" / fmt::format("{}.cpp", *bin))) {
            throw Error { fmt::format("binary src/bin/{}.cpp not found", *bin) };
        }
    } else if (const auto& example = opt.example) {
        if (!fs::exists(root / kExamplesPath / fmt::format("{}.cpp", *example))) {
            throw Error { fmt::format("example examples/{}.cpp not found", *example) };
        }
    }
}

std::string choose_binary(const cmd::RunOptions& options, const Manifest& manifest)
{
    if (options.example) {
        return *options.example;
    }
    if (options.bin) {
        return *options.bin;
    }

    return fmt::format("{}", manifest.name());
}

}

int cmd::run_run(const RunOptions& options)
{
    validate_options(options);

    BuildContext ctx(options.profile);
    Manifest manifest(ctx.metafile);

    validate_bin(ctx.root, manifest.name(), options);

    const auto bin = choose_binary(options, manifest);
    const auto target = options.example ? fmt::format("example_{}", bin) : bin;
    const int result = run_build({ .profile = options.profile, .target = target });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    const auto bin_file = options.example ? ctx.profile_dir / kExamplesPath / bin : ctx.profile_dir / bin;
    if (!fs::exists(bin_file)) {
        warn("no binary to run: {}", bin_file.string());
        return EXIT_SUCCESS;
    }

    const auto cmd = fmt::format("{} {}", bin_file.string(), options.args);
    status("run", "{}", cmd);
    return boost::process::system(cmd);
}