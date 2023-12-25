#include "cppship/cmake/generator.h"
#include "cppship/core/manifest.h"
#include "cppship/util/fs.h"
#include "cppship/util/io.h"
#include "cppship/util/repo.h"

#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <gtest/gtest.h>

using namespace cppship;
using namespace cppship::cmake;

namespace {

Manifest mock_manifest(const std::string_view content)
{
    static std::atomic<int> S_COUNTER { 0 };

    const auto file = fs::temp_directory_path() / fmt::format("generator-{}", ++S_COUNTER);
    write(file, content);

    return Manifest { file };
}

}

TEST(generator, BinVersion)
{
    const auto dir = fs::temp_directory_path();
    create_if_not_exist(dir / kSrcPath);
    write(dir / kSrcPath / "main.cpp", "");

    Layout layout(dir, "tmp");
    auto meta = mock_manifest(R"([package]
name = "abc-abc-abc"
version = "0.1.0"
    )");
    CmakeGenerator gen(&layout, meta, {});
    const auto content = std::move(gen).build();
    EXPECT_TRUE(boost::contains(content, "target_compile_definitions(tmp PRIVATE ABC_ABC_ABC_VERSION="));
}