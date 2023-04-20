#include "cppship/cmake/lib.h"
#include "cppship/util/io.h"
#include "cppship/util/repo.h"

#include <gtest/gtest.h>

using namespace cppship;
using namespace cppship::cmake;

TEST(lib, Interface)
{
    const auto dir = fs::temp_directory_path();
    const auto libdir = dir / kIncludePath;
    fs::create_directory(libdir);

    CmakeLib lib({ .name = "test",
        .include_dir = libdir,
        .deps = { { .cmake_package = "pkg", .cmake_targets = { "pkg1", "pkg2" } } },
        .definitions = { "A", "B" } });

    std::ostringstream oss;
    lib.build(oss);

    EXPECT_EQ(oss.str(), R"(
# LIB
add_library(test INTERFACE)

target_include_directories(test INTERFACE ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(test INTERFACE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test INTERFACE pkg1 pkg2)

target_compile_definitions(test INTERFACE A B)
)");
}

TEST(lib, Source)
{
    const auto dir = fs::temp_directory_path();
    const auto incdir = dir / kIncludePath;
    fs::create_directory(incdir);

    const auto libdir = dir / kLibPath;
    fs::remove_all(libdir);
    fs::create_directory(libdir);
    write(libdir / "a.cpp", "");
    write(libdir / "b.cpp", "");

    CmakeLib lib({ .name = "test",
        .include_dir = incdir,
        .source_dir = libdir,
        .deps = { { .cmake_package = "pkg", .cmake_targets = { "pkg1", "pkg2" } } },
        .definitions = { "A", "B" } });

    std::ostringstream oss;
    lib.build(oss);

    EXPECT_EQ(oss.str(),
        fmt::format(R"(
# LIB
add_library(test {0}/a.cpp
{0}/b.cpp)

target_include_directories(test PUBLIC ${{CMAKE_SOURCE_DIR}}/lib)
target_include_directories(test PUBLIC ${{CMAKE_SOURCE_DIR}}/include)

target_link_libraries(test PUBLIC pkg1 pkg2)

target_compile_definitions(test PUBLIC A B)
)",
            libdir.string()));
}

TEST(lib, InnerTest)
{
    const auto dir = fs::temp_directory_path();
    const auto incdir = dir / kIncludePath;
    fs::create_directory(incdir);

    const auto libdir = dir / kLibPath;
    fs::remove_all(libdir);
    fs::create_directory(libdir);
    write(libdir / "a.cpp", "");
    write(libdir / "test.cpp", "");
    write(libdir / "a_test.cpp", "");

    CmakeLib lib({ .name = "test",
        .include_dir = incdir,
        .source_dir = libdir,
        .deps = { { .cmake_package = "pkg", .cmake_targets = { "pkg1", "pkg2" } } },
        .definitions = { "A", "B" } });

    std::ostringstream oss;
    lib.build(oss);

    EXPECT_EQ(oss.str(),
        fmt::format(R"(
# LIB
add_library(test {0}/a.cpp
{0}/test.cpp)

target_include_directories(test PUBLIC ${{CMAKE_SOURCE_DIR}}/lib)
target_include_directories(test PUBLIC ${{CMAKE_SOURCE_DIR}}/include)

target_link_libraries(test PUBLIC pkg1 pkg2)

target_compile_definitions(test PUBLIC A B)
)",
            libdir.string()));
}