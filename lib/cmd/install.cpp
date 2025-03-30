#include "cppship/cmd/install.h"

#include <cstdlib>

#include <gsl/narrow>
#include <spdlog/spdlog.h>

#include "cppship/cmd/build.h"
#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

using namespace cppship;

int cmd::run_install([[maybe_unused]] const InstallOptions& options)
{
    // TODO(someone): fix me
#ifdef _WINNDOWS
    throw Error { "install is not supported in Windows" };
#else
    const int result = run_build({ .profile = options.profile });
    if (result != 0) {
        return EXIT_FAILURE;
    }

    BuildContext ctx(options.profile);
    Manifest manifest(ctx.metafile);
    if (manifest.is_workspace()) {
        throw Error { "install for workspace is not supported now" };
    }

    const auto bin_file = ctx.profile_dir / manifest.get_if_package()->name();
    if (!fs::exists(bin_file)) {
        warn("no binary to install");
        return EXIT_SUCCESS;
    }

    const auto dst = fmt::format("/usr/local/bin/{}", manifest.get_if_package()->name());
    status("install", "{} to {}", bin_file.string(), dst);
    fs::copy_file(bin_file, dst, fs::copy_options::overwrite_existing);
    return EXIT_SUCCESS;
#endif
}
