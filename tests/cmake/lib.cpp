#include "cppship/cmake/lib.h"
#include "cppship/util/io.h"
#include "cppship/util/repo.h"

#include <boost/algorithm/string/join.hpp>
#include <gtest/gtest.h>

using namespace cppship;
using namespace cppship::cmake;

TEST(lib, Interface)
{
    const auto dir = fs::temp_directory_path();
    const auto libdir = dir / kIncludePath;
    fs::create_directory(libdir);

    CmakeLib lib({
        .name = "test",
        .include_dirs = { libdir },
        .deps = { { .cmake_package = "pkg", .cmake_targets = { "pkg1", "pkg2" } } },
        .definitions = { "A", "B" },
    });

    std::ostringstream oss;
    lib.build(oss);

    EXPECT_EQ(lib.target(), "test_lib");
    EXPECT_EQ(oss.str(),
        fmt::format(R"(
# LIB
add_library(test_lib INTERFACE)

target_include_directories(test_lib INTERFACE {})

target_link_libraries(test_lib INTERFACE pkg1 pkg2)

target_compile_definitions(test_lib INTERFACE A B)
)",
            libdir.generic_string()));
}

TEST(lib, Source)
{
    const auto dir = fs::temp_directory_path();
    const auto incdir = dir / kIncludePath;
    fs::create_directory(incdir);

    const auto libdir = dir / kLibPath;
    fs::remove_all(libdir);
    fs::create_directory(libdir);

    const auto file_a = libdir / "a.cpp";
    const auto file_b = libdir / "b.cpp";
    write(file_a, "");
    write(file_b, "");

    CmakeLib lib({
        .name = "test",
        .include_dirs = { incdir },
        .sources = { file_a, file_b },
        .deps = { { .cmake_package = "pkg", .cmake_targets = { "pkg1", "pkg2" } } },
        .definitions = { "A", "B" },
    });

    std::ostringstream oss;
    lib.build(oss);

    EXPECT_EQ(oss.str(),
        fmt::format(R"(
# LIB
add_library(test_lib {}
{})

target_include_directories(test_lib PUBLIC {})

target_link_libraries(test_lib PUBLIC pkg1 pkg2)

target_compile_definitions(test_lib PUBLIC A B)
)",
            file_a.generic_string(), file_b.generic_string(), incdir.generic_string()));
}

TEST(lib, Alias)
{
    const auto dir = fs::temp_directory_path();
    const auto incdir = dir / kIncludePath;
    fs::create_directory(incdir);

    const auto libdir = dir / kLibPath;
    fs::remove_all(libdir);
    fs::create_directory(libdir);

    const auto file_a = libdir / "a.cpp";
    const auto file_b = libdir / "b.cpp";
    write(file_a, "");
    write(file_b, "");

    CmakeLib lib({
        .name = "test",
        .name_alias = "test",
        .include_dirs = { incdir },
        .sources = { file_a, file_b },
        .deps = { { .cmake_package = "pkg", .cmake_targets = { "pkg1", "pkg2" } } },
        .definitions = { "A", "B" },
    });

    std::ostringstream oss;
    lib.build(oss);

    EXPECT_EQ(oss.str(),
        fmt::format(R"(
# LIB
add_library(test_lib {}
{})
set_target_properties(test_lib PROPERTIES OUTPUT_NAME "test")

target_include_directories(test_lib PUBLIC {})

target_link_libraries(test_lib PUBLIC pkg1 pkg2)

target_compile_definitions(test_lib PUBLIC A B)
)",
            file_a.generic_string(), file_b.generic_string(), incdir.generic_string()));
}