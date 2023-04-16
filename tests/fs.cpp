#include <gtest/gtest.h>

#include "cppship/util/fs.h"

using namespace cppship;

TEST(fs, ScopedCurrentDir)
{
    const auto cwd = fs::current_path();
    const auto tmpdir = fs::temp_directory_path();
    EXPECT_NE(cwd, tmpdir);

    {
        ScopedCurrentDir guard(tmpdir);
        EXPECT_TRUE(fs::equivalent(fs::current_path(), tmpdir));
    }

    EXPECT_EQ(fs::current_path(), cwd);
}