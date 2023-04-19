#include "cppship/cmake/generator.h"
#include "cppship/cmake/bin.h"
#include "cppship/cmake/lib.h"
#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/log.h"
#include "cppship/util/repo.h"

#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
#include <range/v3/action/push_back.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

using namespace ranges::views;

using namespace cppship;

CmakeGenerator::CmakeGenerator(const Manifest& manifest, const ResolvedDependencies& deps)
    : mManifest(manifest)
{
    for (const auto& dep : manifest.dependencies()) {
        const auto& resolved_dep = deps.at(dep.package);

        if (dep.components.empty()) {
            mDeps.push_back(
                { .cmake_package = resolved_dep.cmake_package, .cmake_targets = { resolved_dep.cmake_target } });
        } else {
            std::set<std::string> resolved_components(resolved_dep.components.begin(), resolved_dep.components.end());
            std::vector<std::string> targets_required;

            for (const auto& declared_component : dep.components) {
                auto component = fmt::format("{}::{}", resolved_dep.cmake_package, declared_component);
                if (!resolved_components.contains(component)) {
                    throw Error { fmt::format("invalid component {} in manifest", declared_component) };
                }

                targets_required.push_back(std::move(component));
            }

            mDeps.push_back({ .cmake_package = resolved_dep.cmake_package, .cmake_targets = targets_required });
        }
    }
}

std::string CmakeGenerator::build() &&
{
    emit_header_();
    emit_package_finders_();

    add_lib_sources_();
    add_app_sources_();

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
    const auto root = get_project_root();
    const auto include_dir = root / kIncludePath;
    const auto source_dir = root / kLibPath;
    if (!fs::exists(include_dir)) {
        if (fs::exists(source_dir)) {
            warn("lib without a include dir(eg. {}) is not a valid profile", include_dir.string());
        }

        return;
    }

    cmake::CmakeLib lib({ .name = std::string { mName },
        .include_dir = include_dir,
        .source_dir = fs::exists(source_dir) ? std::optional { source_dir } : std::nullopt,
        .deps = mDeps,
        .definitions = mManifest.definitions() });
    lib.build(mOut);

    mHasLib = true;
}

void CmakeGenerator::add_app_sources_()
{
    const auto root = get_project_root();
    const auto bins = list_sources(root / kSrcPath / kBinPath);

    auto sources = list_sources(root / kSrcPath);
    if (sources.empty()) {
        return;
    }

    for (const auto& bin : bins) {
        cmake::CmakeBin gen({ .name = bin.stem().string(),
            .sources = { bin },
            .lib = mHasLib ? std::optional<std::string> { mName } : std::nullopt,
            .deps = mDeps,
            .definitions = mManifest.definitions() });

        gen.build(mOut);
    }

    std::erase_if(sources, [&bins](const auto& path) { return bins.contains(path); });
    if (sources.empty()) {
        return;
    }

    std::vector<std::string> definitions { fmt::format(
        R"({}_VERSION="${{PROJECT_VERSION}}")", boost::to_upper_copy(std::string { mName })) };
    ranges::push_back(definitions, mManifest.definitions());

    cmake::CmakeBin gen({ .name = fmt::format("{}_bin", mName),
        .name_alias = std::string { mName },
        .sources = sources | ranges::to<std::vector<fs::path>>(),
        .include_dir = root / kSrcPath,
        .lib = mHasLib ? std::optional<std::string> { mName } : std::nullopt,
        .deps = mDeps,
        .definitions = definitions });

    gen.build(mOut);
}

void CmakeGenerator::add_bin_sources_() { }

void CmakeGenerator::add_test_sources_()
{
    const auto sources = list_sources_(kTestsPath);
    if (sources.empty()) {
        return;
    }

    mOut << "\n # Tests\n";
    mOut << R"(file(GLOB_RECURSE srcs RELATIVE "${CMAKE_SOURCE_DIR}/tests" "${CMAKE_SOURCE_DIR}/tests/**.cpp")

find_package(GTest REQUIRED)

foreach(file ${srcs})
    # a/b/c.cpp => a_b_c_test
    string(REPLACE "/" "_" test_target ${file})
    string(REPLACE ".cpp" "_test" test_target ${test_target})

    add_executable(${test_target} "${CMAKE_SOURCE_DIR}/tests/${file}")

    target_link_libraries(${test_target} PRIVATE ${PROJECT_NAME})
    target_link_libraries(${test_target} PRIVATE GTest::gtest_main)

    add_test(${test_target} ${test_target})
endforeach()
)";
}

void CmakeGenerator::emit_footer_()
{
    mOut << "\n# Footer\n"
         << R"(set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
)";
}

std::set<std::string> CmakeGenerator::list_sources_(std::string_view dir)
{
    auto sources = list_sources(dir);
    return sources | transform([](const auto& path) { return path.string(); }) | ranges::to<std::set<std::string>>();
}
