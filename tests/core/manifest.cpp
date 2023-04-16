#include <atomic>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include <range/v3/algorithm/sort.hpp>

#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/io.h"

using namespace cppship;

namespace {

Manifest mock_manifest(const std::string_view content)
{
    static std::atomic<int> S_COUNTER { 0 };

    const auto file = fs::temp_directory_path() / std::to_string(++S_COUNTER);
    write(file, content);

    return Manifest { file };
}

}

TEST(manifest, PackageBasicFields)
{
    EXPECT_THROW(mock_manifest(""), Error);
    EXPECT_THROW(mock_manifest("[package]"), Error);
    EXPECT_THROW(mock_manifest(R"([package]
    name = "abc")"),
        Error);

    const auto meta = mock_manifest(R"([package]
    name = "abc"
    version = "0.1.0"
    )");

    EXPECT_EQ(meta.name(), "abc");
    EXPECT_EQ(meta.version(), "0.1.0");
    EXPECT_EQ(meta.cxx_std(), CxxStd::cxx17);
    EXPECT_TRUE(meta.definitions().empty());
    EXPECT_TRUE(meta.dependencies().empty());
}

TEST(manifest, PackageStd)
{
    auto meta = mock_manifest(R"([package]
    name = "abc"
    version = "0.1.0"
    std = 11
    )");

    EXPECT_EQ(meta.name(), "abc");
    EXPECT_EQ(meta.version(), "0.1.0");
    EXPECT_EQ(meta.cxx_std(), CxxStd::cxx11);

    EXPECT_THROW(mock_manifest(R"([package]
    name = "abc"
    version = "0.1.0"
    std = 16
    )"),
        Error);
}

TEST(manifest, PackageDefinitions)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"
std = 11
definitions = []
)");

    EXPECT_TRUE(meta.definitions().empty());

    meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"
std = 11
definitions = ["A", "B"]
)");
    EXPECT_EQ(meta.definitions().size(), 2);
    EXPECT_EQ(meta.definitions()[0], "A");
    EXPECT_EQ(meta.definitions()[1], "B");
}

TEST(manifest, Dependencies)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
)");

    EXPECT_TRUE(meta.dependencies().empty());

    meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
boost = { version = "1.81.0", components = ["headers"] }
toml11 = "3.7.1"
)");

    EXPECT_EQ(meta.dependencies().size(), 2);

    auto deps = meta.dependencies();
    ranges::sort(deps, std::less<> {}, &DeclaredDependency::package);

    const auto& dep1 = deps[0];
    EXPECT_EQ(dep1.package, "boost");
    EXPECT_EQ(dep1.version, "1.81.0");
    EXPECT_EQ(dep1.components, std::vector<std::string> { "headers" });

    const auto& dep2 = deps[1];
    EXPECT_EQ(dep2.package, "toml11");
    EXPECT_EQ(dep2.version, "3.7.1");
    EXPECT_TRUE(dep2.components.empty());
}