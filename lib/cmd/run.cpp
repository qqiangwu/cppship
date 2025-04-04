#include "cppship/cmd/run.h"

#include <cstdlib>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <range/v3/algorithm/none_of.hpp>
#include <spdlog/spdlog.h>

#include "cppship/cmake/msvc.h"
#include "cppship/cmake/naming.h"
#include "cppship/cmd/build.h"
#include "cppship/core/layout.h"
#include "cppship/core/manifest.h"
#include "cppship/core/workspace.h"
#include "cppship/util/cmd.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"
#include "cppship/util/repo.h"

using namespace cppship;

namespace {

void validate_options(const cmd::BuildContext& ctx, const cmd::RunOptions& options)
{
    if (options.bin && options.example) {
        throw Error { "should not specify --bin and --example in the same time" };
    }
    if (options.package) {
        if (ctx.manifest.get(*options.package) == nullptr) {
            throw InvalidCmdOption("package", "invalid package specified by --package");
        }
    }
}

void validate_target(const Workspace& workspace, const cmd::RunOptions& opt)
{
    const auto layouts = workspace.layouts();
    if (const auto& bin = opt.bin) {
        if (ranges::none_of(layouts, [&](const Layout& layout) { return layout.binary(*bin).has_value(); })) {
            throw Error { fmt::format("binary `{}` not found", *bin) };
        }
    } else if (const auto& example = opt.example) {
        if (ranges::none_of(layouts, [&](const Layout& layout) { return layout.example(*example).has_value(); })) {
            throw Error { fmt::format("example `{}` not found", *example) };
        }
    }
}

std::optional<std::string> choose_binary(const cmd::RunOptions& options, const PackageManifest* manifest)
{
    if (options.example) {
        return options.example;
    }
    if (options.bin) {
        return options.bin;
    }
    if (options.package) {
        return options.package;
    }
    if (manifest != nullptr) {
        return std::make_optional<std::string>(manifest->name());
    }

    return std::nullopt;
}
}

int cmd::run_run(const RunOptions& options)
{
    BuildContext ctx(options.profile);
    validate_options(ctx, options);
    validate_target(ctx.workspace, options);

    cmake::NameTargetMapper mapper;
    const auto bin = choose_binary(options, ctx.manifest.get_if_package());
    if (!bin) {
        warn("no target selected");
        return EXIT_SUCCESS;
    }

    const auto target = options.example ? mapper.example(*bin) : *bin;
    const int result = run_build({ .profile = options.profile, .cmake_target = target });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    const fs::path fixed_bin = msvc::fix_bin_path(ctx, *bin);
    const auto bin_file = options.example ? ctx.profile_dir / kExamplesPath / fixed_bin : ctx.profile_dir / fixed_bin;
    if (!has_cmd(bin_file.string())) {
        error("no binary to run: {}", bin_file.string());
        return EXIT_FAILURE;
    }

    const auto cmd = fmt::format("{} {}", bin_file.string(), options.args);
    status("run", "{}", cmd);
    return boost::process::system(cmd);
}
