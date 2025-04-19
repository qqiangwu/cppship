#include "cppship/cmd/bench.h"

#include <cstdlib>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <spdlog/spdlog.h>

#include "cppship/cmake/msvc.h"
#include "cppship/cmake/naming.h"
#include "cppship/cmd/build.h"
#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

using namespace cppship;
using namespace cppship::cmake;
using namespace ranges::views;

namespace {

int run_one_bench(const cmd::BuildContext& ctx, const std::string_view bench)
{
    const auto bin = ctx.profile_dir / msvc::fix_bin_path(ctx, bench);
    const auto cmd = bin.string();
    status("bench", "{}", cmd);
    return boost::process::system(cmd);
}

}

int cmd::run_bench(const BenchOptions& options)
{
    BuildContext ctx(options.profile);
    Manifest manifest(ctx.metafile);

    NameTargetMapper mapper;
    BuildOptions build_options { .profile = options.profile };
    if (options.target) {
        if (!ctx.layout.bench(*options.target)) {
            throw Error { fmt::format("bench `{}` not found", *options.target) };
        }

        build_options.target = mapper.bench(*options.target);
    } else {
        build_options.groups.insert(BuildGroup::benches);
    }

    int result = run_build(build_options);
    if (result != 0) {
        return EXIT_FAILURE;
    }

    if (build_options.target) {
        return run_one_bench(ctx, *build_options.target);
    }

    for (const auto& bench : ctx.layout.benches()) {
        const int res = run_one_bench(ctx, mapper.bench(bench.name));
        if (res != 0) {
            result = res;
        }
    }

    return result;
}
