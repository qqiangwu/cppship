#include "cppship/cmd/cmake.h"

#include <cstdlib>

#include <boost/algorithm/string/replace.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/concat.hpp>

#include "cppship/cmake/generator.h"
#include "cppship/cmd/build.h"
#include "cppship/core/resolver.h"
#include "cppship/util/io.h"
#include "cppship/util/log.h"

using namespace cppship;
using namespace cppship::cmake;
using namespace ranges::views;

int cmd::run_cmake(const CmakeOptions&)
{
    status("cmake", "generate config");

    BuildContext ctx({});
    ScopedCurrentDir guard(ctx.root);
    conan_detect_profile(ctx);
    conan_setup(ctx);
    conan_install(ctx);

    Resolver resolver(ctx.deps_dir, &ctx.manifest, nullptr);
    const auto result = std::move(resolver).resolve();

    ResolvedDependencies deps = toml::get<ResolvedDependencies>(toml::parse(ctx.dependency_file));
    const auto declared_deps = concat(result.dependencies, result.dev_dependencies) | ranges::to<std::vector>();
    CmakeGenerator gen(&ctx.layout,
        ctx.manifest,
        GeneratorOptions {
            .deps = cmake::resolve_deps(result.dependencies, deps),
            .dev_deps = cmake::resolve_deps(result.dev_dependencies, deps),
            .injector = std::make_unique<CmakeDependencyInjector>(
                ctx.deps_dir, declared_deps, result.resolved_dependencies, deps),
        });
    auto script = std::move(gen).build();

    boost::replace_all(script, ctx.root.string(), "${CMAKE_SOURCE_DIR}");
    write(ctx.root / "CMakeLists.txt", script);

    fs::copy_file(ctx.conan_file, ctx.root / "conanfile.txt", fs::copy_options::overwrite_existing);

    return 0;
}
