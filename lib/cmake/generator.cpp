#include "cppship/cmake/generator.h"
#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/repo.h"

#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
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

# cpp std
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

# cpp warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter")
set(CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../")

# add conan generator folder
list(PREPEND CMAKE_PREFIX_PATH "${CONAN_GENERATORS_FOLDER}"))"
         << "\n";
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
    const auto sources = list_sources_(kLibPath);
    if (sources.empty()) {
        return;
    }

    mOut << "\n # LIB\n";
    mOut << fmt::format("add_library({} {})\n", mName, boost::join(sources, "\n"));

    mOut << "\n"
         << fmt::format("target_include_directories({} PRIVATE ${{CMAKE_SOURCE_DIR}}/{})\n", mName, kLibPath)
         << fmt::format("target_include_directories({} PUBLIC ${{CMAKE_SOURCE_DIR}}/{})\n", mName, kIncludePath)
         << std::endl;

    for (const auto& dep : mDeps) {
        mOut << fmt::format("target_link_libraries({} PUBLIC {})\n", mName, boost::join(dep.cmake_targets, " "));
    }

    mHasLib = true;
}

void CmakeGenerator::add_app_sources_()
{
    const auto sources = list_sources_(kSrcPath);
    if (sources.empty()) {
        return;
    }

    mOut << "\n# APP\n" << fmt::format("add_executable({}_bin {})\n", mName, boost::join(sources, "\n"));

    mOut << fmt::format(R"(target_compile_definitions({}_bin PRIVATE {}_VERSION="${{PROJECT_VERSION}}"))", mName,
        boost::to_upper_copy(std::string(mName)));
    mOut << fmt::format("\ntarget_include_directories({}_bin PRIVATE ${{CMAKE_SOURCE_DIR}}/{})\n", mName, kSrcPath);

    mOut << "\n";
    if (mHasLib) {
        mOut << fmt::format("target_link_libraries({0}_bin PRIVATE {0})\n", mName);
    } else {
        for (const auto& dep : mDeps) {
            mOut << fmt::format(
                "target_link_libraries({}_bin PUBLIC {})\n", mName, boost::join(dep.cmake_targets, " "));
        }
    }

    mOut << "\n";
    mOut << fmt::format(R"(set_target_properties({0}_bin PROPERTIES OUTPUT_NAME "{0}"))", mName);
    mOut << fmt::format("\n\ninstall(TARGETS {}_bin)\n", mName);
}

void CmakeGenerator::add_bin_sources_() { }

void CmakeGenerator::add_test_sources_()
{
    const auto sources = list_sources_(kTestsPath);
    if (sources.empty()) {
        return;
    }

    mOut << "\n # Tests\n";
    mOut << R"(file(GLOB srcs ${CMAKE_SOURCE_DIR}/tests/*.cpp)

find_package(GTest REQUIRED)

foreach(file ${srcs})
    get_filename_component(file_stem ${file} NAME_WE)
    set(test_target ${file_stem}_test)

    add_executable(${test_target} ${file})

    target_link_libraries(${test_target} PRIVATE ${PROJECT_NAME})
    target_link_libraries(${test_target} PRIVATE GTest::gtest_main)

    add_test(${test_target} ${test_target})
endforeach())"
         << "\n";
}

void CmakeGenerator::emit_footer_()
{
    mOut << "\n# Footer\n"
         << fmt::format(R"(set(CPACK_PROJECT_NAME {})
set(CPACK_PROJECT_VERSION {})
include(CPack))",
                mName, mManifest.version())
         << std::endl;
}

std::set<std::string> CmakeGenerator::list_sources_(std::string_view dir)
{
    auto sources = list_sources(dir);
    return sources | transform([](const auto& path) { return path.string(); }) | ranges::to<std::set<std::string>>();
}
