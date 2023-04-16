#include <algorithm>
#include <iterator>
#include <sstream>

#include <gtest/gtest.h>

#include "cppship/exception.h"
#include "cppship/util/io.h"

using namespace cppship;

TEST(fs, write)
{
    const auto tmpdir = fs::temp_directory_path();
    const auto tmpfile = tmpdir / "test";
    constexpr auto kContent = "abc\ndef\n";

    write(tmpfile, kContent);

    const auto content = read_as_string(tmpfile);
    ASSERT_EQ(kContent, content);

    write(tmpfile, "");
    ASSERT_EQ(read_as_string(tmpfile), "");
}

TEST(io, read_as_string)
{
    const std::string content = "abc\ndef\n\n";
    std::istringstream iss(content);

    const auto out = read_as_string(iss);
    ASSERT_EQ(out, content);

    std::istringstream iss2;
    const auto empty_out = read_as_string(iss2);
    ASSERT_TRUE(empty_out.empty());

    const std::string binary(127, '\0');
    std::istringstream iss3(binary);

    const std::string binary_out = read_as_string(iss3);
    ASSERT_TRUE(binary_out == binary);
}

TEST(io, read_as_string_by_path)
{
    const auto tmpfile = fs::temp_directory_path() / __func__;

    fs::remove(tmpfile);

    EXPECT_THROW(read_as_string(tmpfile), IOError);
}