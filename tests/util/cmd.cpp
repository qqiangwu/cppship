#include <gtest/gtest.h>

#include "cppship/util/cmd.h"

using namespace cppship;

TEST(cmd, has_cmd)
{
    ASSERT_FALSE(has_cmd("something-not-exist"));
    ASSERT_TRUE(has_cmd("sh"));
}

TEST(cmd, check_output)
{
    const auto out = check_output("echo hello");
    ASSERT_EQ(out, "hello\n");

    const auto empty_out = check_output("true");
    ASSERT_EQ(empty_out, "");
}