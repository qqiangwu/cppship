#include <cstdlib>

#include <boost/algorithm/string/replace.hpp>

#include "cppship/cmake/generator.h"
#include "cppship/cmd/build.h"
#include "cppship/cmd/cmake.h"
#include "cppship/util/io.h"
#include "cppship/util/log.h"

using namespace cppship;
using namespace cppship::cmake;

int cmd::run_cmake(const CmakeOptions&)
{
    status("cmake", "generate config");

    BuildContext ctx({});
    ScopedCurrentDir guard(ctx.root);
    conan_detect_profile(ctx);
    conan_setup(ctx);
    conan_install(ctx);

    ResolvedDependencies deps = toml::get<ResolvedDependencies>(toml::parse(ctx.dependency_file));
    CmakeGenerator gen(&ctx.layout, ctx.manifest, deps, { .injector = std::make_unique<CmakeDependencyInjector>() });
    auto script = std::move(gen).build();

    boost::replace_all(script, ctx.root.string(), "${CMAKE_SOURCE_DIR}");
    write(ctx.root / "CMakeLists.txt", script);

    fs::copy_file(ctx.conan_file, ctx.root / "conanfile.txt", fs::copy_options::overwrite_existing);

    return 0;
}