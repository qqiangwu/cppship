#include <gtest/gtest.h>

#include <boost/algorithm/string/trim.hpp>

#include "cppship/util/cmd.h"

using namespace cppship;

TEST(cmd, has_cmd)
{
    ASSERT_FALSE(has_cmd("something-not-exist"));

#ifdef _WINDOWS
    ASSERT_TRUE(has_cmd("cmd"));
#else
    ASSERT_TRUE(has_cmd("sh"));
#endif
}

TEST(cmd, check_output)
{
    const auto out = check_output("echo hello");
    ASSERT_EQ(boost::trim_copy(out), "hello");

#ifndef _WINDOWS
    const auto empty_out = check_output("true");
    ASSERT_EQ(empty_out, "");
#endif
}