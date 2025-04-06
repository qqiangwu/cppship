#include "cppship/cmd/run.h"

#include <cstdlib>
#include <functional>
#include <vector>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <range/v3/algorithm/none_of.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
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
        throw Error { "should not specify --bin and --example at the same time" };
    }
    if (options.package) {
        if (ctx.manifest.get(*options.package) == nullptr) {
            throw InvalidCmdOption("package", "invalid package specified by --package");
        }
    }
}

std::string choose_target(const cmd::BuildContext& ctx, const cmd::RunOptions& options)
{
    if (!options.bin && !options.example) {
        const std::string package = std::invoke([&] {
            if (options.package) {
                return *options.package;
            }
            if (const auto& p = ctx.get_active_package()) {
                return *p;
            }

            throw Error { "no target selected" };
        });

        const auto* layout = ctx.workspace.layout(package);
        auto target = layout->binary(package);
        if (!target) {
            throw Error { fmt::format("package {} has no default binary", package) };
        }

        return cmake::NameTargetMapper(package).binary(target->name);
    }

    std::vector<const Layout*> layouts = std::invoke([&]() -> std::vector<const Layout*> {
        if (options.package) {
            return { ctx.workspace.layout(*options.package) };
        }

        return ctx.workspace.layouts() | ranges::views::transform([](const Layout& l) { return &l; })
            | ranges::to<std::vector>();
    });

    const auto candidates = layouts | ranges::views::filter([&](const Layout* layout) {
        if (options.bin) {
            return layout->binary(*options.bin).has_value();
        }
        if (options.example) {
            return layout->example(*options.example).has_value();
        }
        return false;
    }) | ranges::to<std::vector>();

    if (candidates.empty()) {
        throw Error { options.bin ? fmt::format("binary `{}` not found", *options.bin)
                                  : fmt::format("example `{}` not found", *options.example) };
    }
    if (candidates.size() > 1) {
        throw Error { "too many targets selected" };
    }

    const auto* layout = candidates.front();
    cmake::NameTargetMapper mapper(layout->package());
    return options.bin ? mapper.binary(*options.bin) : mapper.example(*options.example);
}

inline std::string_view binary_target_to_name(std::string_view target)
{
    target.remove_suffix(4);
    return target;
}

}

int cmd::run_run(const RunOptions& options)
{
    BuildContext ctx(options.profile);
    validate_options(ctx, options);

    const auto target = choose_target(ctx, options);
    const int result = run_build({ .profile = options.profile, .cmake_target = target });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    const fs::path fixed_bin
        = options.example ? msvc::fix_bin_path(ctx, target) : msvc::fix_bin_path(ctx, binary_target_to_name(target));
    const auto bin_file = options.example ? ctx.profile_dir / kExamplesPath / fixed_bin : ctx.profile_dir / fixed_bin;
    if (!has_cmd(bin_file.string())) {
        error("no binary to run: {}", bin_file.string());
        return EXIT_FAILURE;
    }

    const auto cmd = fmt::format("{} {}", bin_file.string(), options.args);
    status("run", "{}", cmd);
    return boost::process::system(cmd);
}
