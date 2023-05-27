#include <set>

#include <gtest/gtest.h>

#include "cppship/util/string.h"

using namespace cppship::util;

TEST(string, split)
{
    const auto res = split("a b c", boost::is_space());
    ASSERT_EQ(res.size(), 3);
    ASSERT_EQ(res[0], "a");
    ASSERT_EQ(res[1], "b");
    ASSERT_EQ(res[2], "c");
}

TEST(string, split_empty)
{
    auto res = split("", boost::is_space());
    ASSERT_EQ(res.size(), 1);
    ASSERT_TRUE(res[0].empty());

    res = split(" ", boost::is_space());
    ASSERT_EQ(res.size(), 2);
    ASSERT_TRUE(res[0].empty());
    ASSERT_TRUE(res[1].empty());

    res = split("  ", boost::is_space());
    ASSERT_EQ(res.size(), 2);
    ASSERT_TRUE(res[0].empty());
    ASSERT_TRUE(res[1].empty());
}

TEST(string, split_multiple_delim)
{
    const auto res = split("a  b", boost::is_space());
    ASSERT_EQ(res.size(), 2);
    EXPECT_EQ(res[0], "a");
    EXPECT_EQ(res[1], "b");
}

TEST(string, split_to)
{
    const auto res = split_to<std::set>("a b c", boost::is_space());
    ASSERT_EQ(res.size(), 3);
    ASSERT_TRUE(res.contains("a"));
    ASSERT_TRUE(res.contains("b"));
    ASSERT_TRUE(res.contains("c"));
}