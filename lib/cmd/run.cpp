#include <cstdlib>
#include <thread>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <spdlog/spdlog.h>

#include "cppship/cmake/msvc.h"
#include "cppship/cmake/naming.h"
#include "cppship/cmd/build.h"
#include "cppship/cmd/run.h"
#include "cppship/core/manifest.h"
#include "cppship/util/cmd.h"
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

void validate_target(const cmd::BuildContext& ctx, const cmd::RunOptions& opt)
{
    if (const auto& bin = opt.bin) {
        if (!ctx.layout.binary(*bin)) {
            throw Error { fmt::format("binary `{}` not found", *bin) };
        }
    } else if (const auto& example = opt.example) {
        if (!ctx.layout.example(*example)) {
            throw Error { fmt::format("example `{}` not found", *example) };
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

    validate_target(ctx, options);

    cmake::NameTargetMapper mapper;
    const auto bin = choose_binary(options, manifest);
    const auto target = options.example ? mapper.example(bin) : bin;
    const int result = run_build({ .profile = options.profile, .target = target });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    const fs::path fixed_bin = msvc::fix_bin(ctx, bin);
    const auto bin_file = options.example ? ctx.profile_dir / kExamplesPath / fixed_bin : ctx.profile_dir / fixed_bin;
    if (!has_cmd(bin_file.string())) {
        error("no binary to run: {}", bin_file.string());
        return EXIT_FAILURE;
    }

    const auto cmd = fmt::format("{} {}", bin_file.string(), options.args);
    status("run", "{}", cmd);
    return boost::process::system(cmd);
}