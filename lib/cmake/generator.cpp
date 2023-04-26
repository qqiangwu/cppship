#include "cppship/cmake/generator.h"
#include "cppship/cmake/bin.h"
#include "cppship/cmake/group.h"
#include "cppship/cmake/lib.h"
#include "cppship/cmake/naming.h"
#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/log.h"

#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
#include <range/v3/action/push_back.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

using namespace ranges::views;

using namespace cppship;
using namespace cppship::cmake;

namespace {

std::vector<cmake::Dep> collect_cmake_deps(
    const std::vector<DeclaredDependency>& declared_deps, const ResolvedDependencies& deps)
{
    std::vector<cmake::Dep> result;

    for (const auto& dep : declared_deps) {
        const auto& resolved_dep = deps.at(dep.package);

        if (dep.components.empty()) {
            result.push_back({
                .cmake_package = resolved_dep.cmake_package,
                .cmake_targets = { resolved_dep.cmake_target },
            });

            continue;
        }

        std::set<std::string> resolved_components(resolved_dep.components.begin(), resolved_dep.components.end());
        std::vector<std::string> targets_required;

        for (const auto& declared_component : dep.components) {
            auto component = fmt::format("{}::{}", resolved_dep.cmake_package, declared_component);
            if (!resolved_components.contains(component)) {
                throw Error { fmt::format("invalid component {} in manifest", declared_component) };
            }

            targets_required.push_back(std::move(component));
        }

        result.push_back({
            .cmake_package = resolved_dep.cmake_package,
            .cmake_targets = targets_required,
        });
    }

    return result;
}

}

CmakeGenerator::CmakeGenerator(
    gsl::not_null<const Layout*> layout, const Manifest& manifest, const ResolvedDependencies& deps)
    : mLayout(layout)
    , mManifest(manifest)
    , mDeps(collect_cmake_deps(manifest.dependencies(), deps))
    , mDevDeps(collect_cmake_deps(manifest.dev_dependencies(), deps))
{
    ranges::push_back(mDeps4Dev, mDeps);
    ranges::push_back(mDeps4Dev, mDevDeps);
}

std::string CmakeGenerator::build() &&
{
    emit_header_();
    emit_package_finders_();

    add_lib_sources_();
    add_app_sources_();

    emit_dev_package_finders_();
    add_benches_();
    add_examples_();
    add_test_sources_();

    emit_footer_();

    return mOut.str();
}

void CmakeGenerator::emit_header_()
{
    mOut << "cmake_minimum_required(VERSION 3.16)\n"
         << fmt::format("project({} VERSION {})\n", mName, mManifest.version()) << R"(
include(CTest)
enable_testing()

# cpp warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers")
set(CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../")

# add conan generator folder
list(PREPEND CMAKE_PREFIX_PATH "${CONAN_GENERATORS_FOLDER}")
)";

    mOut << "\n"
         << fmt::format(R"(# cpp std
set(CMAKE_CXX_STANDARD {})
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)
)",
                mManifest.cxx_std());
}

void CmakeGenerator::emit_package_finders_()
{
    mOut << "\n# Package finders\n";

    for (const auto& dep : mDeps) {
        mOut << fmt::format("find_package({} REQUIRED)\n", dep.cmake_package);
    }
}

void CmakeGenerator::add_lib_sources_()
{
    const auto target = mLayout->lib();
    if (!target) {
        return;
    }

    cmake::CmakeLib lib({
        .name = target->name,
        .include_dirs = target->includes,
        .sources = target->sources,
        .deps = mDeps,
        .definitions = mManifest.definitions(),
    });
    lib.build(mOut);

    // view lib as a binary target to easy `cppship build` for header only lib
    mLib.emplace(lib.target());
    mBinaryTargets.emplace(*mLib);
}

void CmakeGenerator::add_app_sources_()
{
    if (mLayout->binaries().empty()) {
        return;
    }

    std::vector<std::string> definitions {
        fmt::format(R"({}_VERSION="${{PROJECT_VERSION}}")", boost::to_upper_copy(std::string { mName })),
    };
    ranges::push_back(definitions, mManifest.definitions());

    for (const auto& bin : mLayout->binaries()) {
        cmake::CmakeBin gen({
            .name = bin.name,
            .sources = bin.sources,
            .lib = mLib,
            .deps = mDeps,
            .definitions = definitions,
            .need_install = true,
        });

        gen.build(mOut);
        mBinaryTargets.emplace(bin.name);
    }
}

void CmakeGenerator::emit_dev_package_finders_()
{
    mOut << "\n# Dev Package finders\n";

    for (const auto& dep : mDevDeps) {
        mOut << fmt::format("find_package({} REQUIRED)\n", dep.cmake_package);
    }
}

void CmakeGenerator::add_benches_()
{
    const auto& benches = mLayout->benches();
    if (benches.empty()) {
        return;
    }

    mOut << R"(# BENCH
find_package(benchmark REQUIRED)
)";

    NameTargetMapper mapper;
    auto deps = mDeps4Dev;
    deps.push_back({
        .cmake_package = "benchmark",
        .cmake_targets = { "benchmark::benchmark" },
    });
    for (const auto& bin : benches) {
        const auto target = mapper.bench(bin.name);

        cmake::CmakeBin gen({
            .name = target,
            .sources = bin.sources,
            .lib = mLib,
            .deps = deps,
            .definitions = mManifest.definitions(),
        });

        gen.build(mOut);
        mBenchTargets.emplace(target);
    }
}

void CmakeGenerator::add_examples_()
{
    const auto& examples = mLayout->examples();
    if (examples.empty()) {
        return;
    }

    NameTargetMapper mapper;
    for (const auto& bin : examples) {
        const auto target = mapper.example(bin.name);

        cmake::CmakeBin gen({
            .name = target,
            .name_alias = bin.name,
            .sources = bin.sources,
            .lib = mLib,
            .deps = mDeps4Dev,
            .definitions = mManifest.definitions(),
            .runtime_dir = "examples",
        });

        gen.build(mOut);
        mExampleTargets.emplace(target);
    }
}

void CmakeGenerator::add_test_sources_()
{
    const auto& tests = mLayout->tests();
    if (tests.empty()) {
        return;
    }

    mOut << R"(
# Tests
find_package(GTest REQUIRED)
)";

    NameTargetMapper mapper;
    for (const auto& test : tests) {
        const auto target = mapper.test(test.name);

        mOut << '\n'
             << fmt::format("add_executable({} {})\n", target, test.sources.begin()->string())
             << fmt::format("target_link_libraries({} PRIVATE GTest::gtest_main)\n", target);

        if (mLib) {
            mOut << fmt::format("target_link_libraries({} PRIVATE {})\n", target, *mLib);
        }
        for (const auto& dep : mDeps4Dev) {
            mOut << fmt::format("target_link_libraries({} PRIVATE {})\n", target, boost::join(dep.cmake_targets, " "));
        }

        mOut << fmt::format("add_test({} {})\n", target, target);

        mTestTargets.emplace(target);
    }
}

void CmakeGenerator::emit_footer_()
{
    mOut << "\n# Groups\n"
         << fmt::format(
                "add_custom_target({} DEPENDS {})\n", cmake::kCppshipGroupBinaries, boost::join(mBinaryTargets, " "))
         << fmt::format(
                "add_custom_target({} DEPENDS {})\n", cmake::kCppshipGroupExamples, boost::join(mExampleTargets, " "))
         << fmt::format(
                "add_custom_target({} DEPENDS {})\n", cmake::kCppshipGroupBenches, boost::join(mBenchTargets, " "))
         << fmt::format(
                "add_custom_target({} DEPENDS {})\n", cmake::kCppshipGroupTests, boost::join(mTestTargets, " "));

    mOut << "\n# Footer\n"
         << R"(set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
)";
}