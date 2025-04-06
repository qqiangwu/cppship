#include "cppship/cmd/bench.h"

#include <cstdlib>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <spdlog/spdlog.h>

#include "cppship/cmake/msvc.h"
#include "cppship/cmake/naming.h"
#include "cppship/cmd/build.h"
#include "cppship/core/workspace.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

using namespace cppship;
using namespace cppship::cmake;
using namespace ranges::views;

namespace {

int run_one_bench(const cmd::BuildContext& ctx, const std::string_view bench)
{
    const auto bin = ctx.profile_dir / kBenchesPath / msvc::fix_bin_path(ctx, bench);
    const auto cmd = bin.string();
    status("bench", "{}", cmd);
    return boost::process::system(cmd);
}

}

int cmd::run_bench(const BenchOptions& options)
{
    BuildContext ctx(options.profile);
    BuildOptions build_options { .profile = options.profile };
    if (options.name) {
        const auto layouts = ctx.workspace.layouts()
            | filter([&](const Layout& layout) { return layout.bench(*options.name).has_value(); })
            | ranges::to<std::vector>();
        if (layouts.empty()) {
            throw Error { fmt::format("bench `{}` not found", *options.name) };
        }
        if (layouts.size() > 1) {
            throw Error { fmt::format("too many benches with name {}, eg. {}, {}",
                *options.name,
                layouts.front().package(),
                layouts.back().package()) };
        }

        build_options.cmake_target = NameTargetMapper(layouts.front().package()).bench(*options.name);
    } else {
        build_options.package = options.package;
        build_options.groups.insert(BuildGroup::benches);
    }

    int result = run_build(build_options);
    if (result != 0) {
        return EXIT_FAILURE;
    }

    if (build_options.cmake_target) {
        return run_one_bench(ctx, *build_options.cmake_target);
    }

    for (const auto& layout : ctx.workspace.layouts()) {
        for (const auto& bench : layout.benches()) {
            NameTargetMapper mapper(layout.package());

            const int res = run_one_bench(ctx, mapper.bench(bench.name));
            if (res != 0) {
                result = res;
            }
        }
    }

    return result;
}
