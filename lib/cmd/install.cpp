#include "cppship/cmd/install.h"

#include <cstdlib>
#include <functional>

#include <gsl/narrow>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>

#include "cppship/cmake/naming.h"
#include "cppship/cmd/build.h"
#include "cppship/core/layout.h"
#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

using namespace cppship;

namespace {

int do_install(const cmd::BuildContext& ctx, std::span<std::string> binaries, std::string_view prefix)
{
    for (const auto& bin : binaries) {
        const auto bin_file = ctx.profile_dir / bin;
        if (!fs::exists(bin_file)) {
            warn("binary {} not found", bin_file.string());
            continue;
        }

        const auto dst = fmt::format("{}/bin/{}", prefix, bin);
        status("install", "{} to {}", bin_file.string(), dst);
        fs::copy_file(bin_file, dst, fs::copy_options::overwrite_existing);
    }

    return EXIT_SUCCESS;
}

}

int cmd::run_install([[maybe_unused]] const InstallOptions& options)
{
    // TODO(someone): fix me
#ifdef _WINNDOWS
    throw Error { "install is not supported in Windows" };
#else
    BuildContext ctx(options.profile);
    if (ctx.manifest.is_workspace()) {
        throw Error { "install for workspace is not supported" };
    }

    const auto& layout = ctx.workspace.as_package();
    if (options.binary && !layout.binary(*options.binary).has_value()) {
        throw InvalidCmdOption { "--bin", fmt::format("specified binary {} is not found", *options.binary) };
    }

    cmake::NameTargetMapper mapper(layout.package());
    const int result = run_build({
        .profile = options.profile,
        .cmake_target = options.binary.has_value() ? std::make_optional(mapper.binary(*options.binary)) : std::nullopt,
    });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    std::vector<std::string> binaries = std::invoke([&] {
        if (options.binary) {
            return std::vector<std::string> { *options.binary };
        }

        const auto tmp = layout.binaries();
        return tmp | ranges::views::transform(&Target::name) | ranges::to<std::vector>();
    });

    return do_install(ctx, binaries, "/usr/local");
#endif
}
