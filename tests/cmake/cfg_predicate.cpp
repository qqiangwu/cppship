#include "cppship/cmake/cfg_predicate.h"

#include <gtest/gtest.h>

using namespace cppship;
using namespace cppship::cmake;
using namespace cppship::core;

TEST(cfg_predicate, Option)
{
    ASSERT_EQ(generate_predicate(CfgOption {}), R"(CMAKE_SYSTEM_NAME STREQUAL "Windows")");
    ASSERT_EQ(generate_predicate(cfg::Os::windows), R"(CMAKE_SYSTEM_NAME STREQUAL "Windows")");
    ASSERT_EQ(generate_predicate(cfg::Compiler::msvc), R"(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")");
}

TEST(cfg_predicate, Not)
{
    ASSERT_EQ(generate_predicate(CfgNot { cfg::Os::windows }), R"(NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))");
    ASSERT_EQ(generate_predicate(CfgNot { cfg::Compiler::msvc }), R"(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))");
}

TEST(cfg_predicate, All)
{
    ASSERT_EQ(generate_predicate(CfgAll {}), "");
    ASSERT_EQ(generate_predicate(CfgAll { { cfg::Os::windows } }), R"(CMAKE_SYSTEM_NAME STREQUAL "Windows")");
    ASSERT_EQ(generate_predicate(CfgAll { { cfg::Os::windows, cfg::Compiler::msvc } }),
        "(CMAKE_SYSTEM_NAME STREQUAL \"Windows\" AND CMAKE_CXX_COMPILER_ID STREQUAL \"MSVC\")");
}

TEST(cfg_predicate, Any)
{
    ASSERT_EQ(generate_predicate(CfgAny {}), "");
    ASSERT_EQ(generate_predicate(CfgAny { { cfg::Os::windows } }), R"(CMAKE_SYSTEM_NAME STREQUAL "Windows")");
    ASSERT_EQ(generate_predicate(CfgAny { { cfg::Os::windows, cfg::Os::linux } }),
        "(CMAKE_SYSTEM_NAME STREQUAL \"Windows\" OR CMAKE_SYSTEM_NAME STREQUAL \"Linux\")");
}

TEST(cfg_predicate, Nested)
{
    ASSERT_EQ(generate_predicate(CfgAny { { cfg::Os::windows, CfgNot { cfg::Os::linux } } }),
        "(CMAKE_SYSTEM_NAME STREQUAL \"Windows\" OR NOT (CMAKE_SYSTEM_NAME STREQUAL \"Linux\"))");
    ASSERT_EQ(generate_predicate(CfgAll { { cfg::Os::windows, CfgNot { cfg::Os::linux } } }),
        "(CMAKE_SYSTEM_NAME STREQUAL \"Windows\" AND NOT (CMAKE_SYSTEM_NAME STREQUAL \"Linux\"))");
}