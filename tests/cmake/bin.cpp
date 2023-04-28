#include "cppship/cmake/bin.h"
#include "cppship/util/io.h"

#include <gtest/gtest.h>

using namespace cppship;
using namespace cppship::cmake;

TEST(bin, Basic)
{
    CmakeBin bin({ .name = "test", .sources = { "a.cpp" } });

    std::ostringstream oss;
    bin.build(oss);

    EXPECT_EQ(oss.str(), R"(
# BIN
add_executable(test a.cpp)
)");
}

TEST(bin, NameAlias)
{
    CmakeBin bin({
        .name = "test",
        .name_alias = "xxx",
        .sources = { "a.cpp" },
    });

    std::ostringstream oss;
    bin.build(oss);

    EXPECT_EQ(oss.str(), R"(
# BIN
add_executable(test a.cpp)

set_target_properties(test PROPERTIES OUTPUT_NAME "xxx")
)");
}

TEST(bin, IncludeDir)
{
    CmakeBin bin({
        .name = "test",
        .name_alias = "xxx",
        .sources = { "a.cpp", "b.cpp" },
        .include_dir = "include",
    });

    std::ostringstream oss;
    bin.build(oss);

    EXPECT_EQ(oss.str(), R"(
# BIN
add_executable(test a.cpp
b.cpp)

target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)

set_target_properties(test PROPERTIES OUTPUT_NAME "xxx")
)");
}

TEST(bin, Lib)
{
    CmakeBin bin({
        .name = "test",
        .name_alias = "xxx",
        .sources = { "a.cpp", "b.cpp" },
        .include_dir = "include",
        .lib = "lib",
    });

    std::ostringstream oss;
    bin.build(oss);

    EXPECT_EQ(oss.str(), R"(
# BIN
add_executable(test a.cpp
b.cpp)

target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test PRIVATE lib)

set_target_properties(test PROPERTIES OUTPUT_NAME "xxx")
)");
}

TEST(bin, Deps)
{
    CmakeBin bin({
        .name = "test",
        .name_alias = "xxx",
        .sources = { "a.cpp", "b.cpp" },
        .include_dir = "include",
        .lib = "lib",
        .deps = { { .cmake_package = "pkg", .cmake_targets = { "pkg1", "pkg2" } } },
    });

    std::ostringstream oss;
    bin.build(oss);

    EXPECT_EQ(oss.str(), R"(
# BIN
add_executable(test a.cpp
b.cpp)

target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test PRIVATE lib)
target_link_libraries(test PRIVATE pkg1 pkg2)

set_target_properties(test PROPERTIES OUTPUT_NAME "xxx")
)");
}

TEST(bin, Definitions)
{
    CmakeBin bin({ .name = "test",
        .name_alias = "xxx",
        .sources = { "a.cpp", "b.cpp" },
        .include_dir = "include",
        .lib = "lib",
        .deps = { { .cmake_package = "pkg", .cmake_targets = { "pkg1", "pkg2" } } },
        .definitions = { "A", "B" } });

    std::ostringstream oss;
    bin.build(oss);

    EXPECT_EQ(oss.str(), R"(
# BIN
add_executable(test a.cpp
b.cpp)

target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test PRIVATE lib)
target_link_libraries(test PRIVATE pkg1 pkg2)

target_compile_definitions(test PRIVATE A B)

set_target_properties(test PROPERTIES OUTPUT_NAME "xxx")
)");
}

TEST(bin, Install)
{
    CmakeBin bin({ .name = "test", .sources = { "a.cpp" }, .need_install = true });

    std::ostringstream oss;
    bin.build(oss);

    EXPECT_EQ(oss.str(), R"(
# BIN
add_executable(test a.cpp)

install(TARGETS test)
)");
}

TEST(bin, RuntimeDir)
{
    CmakeBin bin({ .name = "test", .sources = { "a.cpp" }, .runtime_dir = "examples" });

    std::ostringstream oss;
    bin.build(oss);

    EXPECT_EQ(oss.str(), R"(
# BIN
add_executable(test a.cpp)

set_target_properties(test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples")
)");
}

TEST(bin, RuntimeDirAndName)
{
    CmakeBin bin({ .name = "test", .name_alias = "xxx", .sources = { "a.cpp" }, .runtime_dir = "examples" });

    std::ostringstream oss;
    bin.build(oss);

    EXPECT_EQ(oss.str(), R"(
# BIN
add_executable(test a.cpp)

set_target_properties(test PROPERTIES OUTPUT_NAME "xxx")
set_target_properties(test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples")
)");
}