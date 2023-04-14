#include <gtest/gtest.h>

#include "cppship/util/fs.h"

using namespace cppship;

TEST(fs, write_file)
{
    const auto tmpdir = fs::temp_directory_path();
    const auto tmpfile = tmpdir / "test";
    constexpr auto kContent = "abc\ndef\n";

    write_file(tmpfile, kContent);

    const auto content = read_file(tmpfile);
    ASSERT_EQ(kContent, content);
}