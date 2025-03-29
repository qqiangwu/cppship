#include "cppship/util/cmd.h"

#include <cstdlib>

#include <boost/algorithm/string/trim.hpp>
#include <gtest/gtest.h>

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

TEST(cmd, remove_env)
{
#ifndef _WINDOWS
    ::setenv("CMAKE_GENERATOR", "?", 1); // NOLINT
    ASSERT_EQ(run_cmd("[[ $CMAKE_GENERATOR ]] && exit 1 || exit 0"), 0);
#endif
}