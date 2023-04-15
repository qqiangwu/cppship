#include <cstdlib>

#include "cppship/cmd/init.h"
#include "cppship/core/manifest.h"
#include "cppship/core/template.h"
#include "cppship/exception.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

using namespace cppship;

int cmd::run_init(const InitOptions& options)
{
    if (options.lib && options.bin) {
        throw Error { "--bin and --lib cannot co-exist" };
    }

    const std::string name = options.name.value_or(options.dir.filename());
    generate_manifest(name, options.std, options.dir);

    if (options.lib) {
        generate_lib_template(options.dir);

        status("created", "library `{}` package", name);
    } else {
        generate_bin_template(options.dir);

        status("created", "binary `{}` package", name);
    }

    return EXIT_SUCCESS;
}