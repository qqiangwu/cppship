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

TEST(fs, CreateIfNotExist)
{
    const auto tmpdir = fs::temp_directory_path();
    const auto testdir = fs::path("cppship-unitests");

    ScopedCurrentDir guard(tmpdir);

    if (fs::exists(testdir)) {
        fs::remove_all(testdir);
    }
    fs::create_directory(testdir);

    {
        ScopedCurrentDir guard2(testdir);
        const auto newdir = fs::path("subdir1");
        EXPECT_FALSE(fs::exists(newdir));
        cppship::create_if_not_exist(newdir);
        EXPECT_TRUE(fs::exists(newdir));
    }
    {
        ScopedCurrentDir guard2(testdir);
        const auto newdir = fs::path("./subdir2/subsubdir1");
        EXPECT_FALSE(fs::exists(newdir));
        cppship::create_if_not_exist(newdir);
        EXPECT_TRUE(fs::exists(newdir));
    }
    {
        ScopedCurrentDir guard2(testdir);
        const auto newdir = fs::path("subdir3/subsubdir1/subsubsubdir1");
        EXPECT_FALSE(fs::exists(newdir));
        cppship::create_if_not_exist(newdir);
        EXPECT_TRUE(fs::exists(newdir));
    }
}
