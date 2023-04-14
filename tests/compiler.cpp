#include <gtest/gtest.h>

#include "cppship/core/compiler.h"

using namespace cppship;
using namespace cppship::compiler;

TEST(compiler, to_string)
{
    ASSERT_EQ(to_string(CompilerId::apple_clang), "apple-clang");
    ASSERT_EQ(to_string(CompilerId::clang), "clang");
    ASSERT_EQ(to_string(CompilerId::gcc), "gcc");
    ASSERT_EQ(to_string(CompilerId::unknown), "unknown");
}

TEST(compiler, detect)
{
    CompilerInfo info;

    ASSERT_TRUE(info.id() != CompilerId::unknown);
    ASSERT_NE(info.command(), "");
    ASSERT_NE(info.libcxx(), "");
    ASSERT_NE(info.version(), 0);
}