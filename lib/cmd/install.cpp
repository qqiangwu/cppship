#include <cstdlib>

#include <gsl/narrow>
#include <spdlog/spdlog.h>

#include "cppship/cmd/build.h"
#include "cppship/cmd/install.h"
#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

using namespace cppship;

int cmd::run_install([[maybe_unused]] const InstallOptions& options)
{
    // TODO: fix me
#ifdef _WINNDOWS
    throw Error { "install is not supported in Windows" };
#else
    const int result = run_build({ .profile = options.profile });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    BuildContext ctx(options.profile);
    Manifest manifest(ctx.metafile);
    const auto bin_file = ctx.profile_dir / manifest.name();
    if (!fs::exists(bin_file)) {
        warn("no binary to install");
        return EXIT_SUCCESS;
    }

    const auto root = fs::path(options.root);
    const auto dst = root / "bin" / manifest.name();
    create_if_not_exist(dst.parent_path());
    status("install", "{} to {}", bin_file.string(), dst.string());
    fs::copy_file(bin_file, dst, fs::copy_options::overwrite_existing);
    return EXIT_SUCCESS;
#endif
}
