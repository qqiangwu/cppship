#include "cppship/cmd/cmake.h"

#include <cstdlib>

#include <boost/algorithm/string/replace.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/concat.hpp>

#include "cppship/cmd/build.h"
#include "cppship/core/workspace.h"
#include "cppship/util/io.h"
#include "cppship/util/log.h"

using namespace cppship;
using namespace ranges::views;

int cmd::run_cmake(const CmakeOptions&)
{
    status("cmake", "generate config");

    BuildContext ctx({});
    std::ignore = enforce_default_package(ctx.workspace);

    ScopedCurrentDir guard(ctx.root);
    conan_detect_profile(ctx);
    conan_setup(ctx);
    conan_install(ctx);

    auto script = cmd_internals::cmake_gen_config(ctx, true);

    boost::replace_all(script, ctx.root.string(), "${CMAKE_SOURCE_DIR}");
    write(ctx.root / "CMakeLists.txt", script);

    fs::copy_file(ctx.conan_file, ctx.root / "conanfile.txt", fs::copy_options::overwrite_existing);

    return 0;
}
