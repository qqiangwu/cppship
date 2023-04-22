#include <cstdlib>
#include <thread>

#include <boost/process/system.hpp>
#include <gsl/narrow>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <spdlog/spdlog.h>

#include "cppship/cmd/bench.h"
#include "cppship/cmd/build.h"
#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"
#include "cppship/util/repo.h"

using namespace cppship;
using namespace ranges::views;

namespace {

void validate_bin(const fs::path& root, const cmd::BenchOptions& opt)
{
    if (const auto& bench = opt.target) {
        if (!fs::exists(root / kBenchesPath / fmt::format("{}.cpp", *bench))) {
            throw Error { fmt::format("bench {}/{}.cpp not found", kBenchesPath, *bench) };
        }
    }
}

int run_one_bench(const cmd::BuildContext& ctx, const std::string_view bench)
{
    const auto bin = (ctx.profile_dir / bench).string();
    status("bench", "{}", bin);
    return boost::process::system(bin);
}

}

int cmd::run_bench(const BenchOptions& options)
{
    BuildContext ctx(options.profile);
    Manifest manifest(ctx.metafile);

    validate_bin(ctx.root, options);

    BuildOptions build_options { .profile = options.profile };
    if (options.target) {
        build_options.target = fmt::format("{}_bench", *options.target);
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

    const auto sources = list_sources(kBenchesPath);
    const auto benches = sources | filter([](const fs::path& path) { return path.extension() == ".cpp"; })
        | transform([](const fs::path& path) { return path.stem().string(); })
        | transform([](const std::string& str) { return fmt::format("{}_bench", str); })
        | ranges::to<std::vector<std::string>>();

    for (const auto& bench : benches) {
        const int res = run_one_bench(ctx, bench);
        if (res != 0) {
            result = res;
        }
    }

    return result;
}