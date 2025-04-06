#include "cppship/core/workspace.h"

#include <atomic>
#include <stdexcept>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include <range/v3/algorithm/sort.hpp>

#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/io.h"

using namespace cppship;
using namespace cppship::core;

namespace {

class DirTree {
public:
    explicit DirTree(const std::set<std::string_view>& trees)
    {
        fs::create_directory(mRoot);
        fs::current_path(mRoot);

        for (const auto& path : trees) {
            fs::path fs_path { path };

            if (path.ends_with('/')) {
                fs::create_directories(fs_path);
            } else {
                if (fs_path.has_parent_path()) {
                    fs::create_directories(fs_path.parent_path());
                }
                std::ofstream ofs(fs_path);
            }
        }
    }

    ~DirTree()
    {
        fs::current_path(mOriPath);
        fs::remove_all(mRoot);
    }

    const fs::path& root() const { return mRoot; }

    std::string relative(const fs::path& path)
    {
        // use generic_string to deal with windows
        return path.lexically_relative(mRoot).generic_string();
    }

private:
    fs::path mRoot = fs::temp_directory_path() / "cppship.test";
    fs::path mOriPath = fs::current_path();
};

}

TEST(workspace, DuplicateBinary)
{
    DirTree tree({
        "cppship.toml",
        "app_1/cppship.toml",
        "app_1/src/main.cpp",
        "app_2/cppship.toml",
        "app_2/src/bin/app_1.cpp",
    });

    write("cppship.toml", R"([workspace]
members = ["app_1", "app_2"])");
    write("app_1/cppship.toml", R"([package]
version = "1.0.0"
name = "app_1")");
    write("app_2/cppship.toml", R"([package]
version = "1.0.0"
name = "app_2")");

    try {
        Manifest manifest(tree.root() / "cppship.toml");
        Workspace workspace(tree.root(), manifest);
        FAIL();
    } catch (const Error& e) {
        ASSERT_EQ(std::string(e.what()), "duplicate binary app_1 from package app_1 and app_2");
    }
}

TEST(workspace, DuplicateExamples)
{
    DirTree tree({
        "cppship.toml",
        "app_1/cppship.toml",
        "app_1/examples/app_1.cpp",
        "app_2/cppship.toml",
        "app_2/examples/app_1.cpp",
    });

    write("cppship.toml", R"([workspace]
members = ["app_1", "app_2"])");
    write("app_1/cppship.toml", R"([package]
version = "1.0.0"
name = "app_1")");
    write("app_2/cppship.toml", R"([package]
version = "1.0.0"
name = "app_2")");

    Manifest manifest(tree.root() / "cppship.toml");
    Workspace workspace(tree.root(), manifest);
    for (const auto& layout : workspace.layouts()) {
        ASSERT_TRUE(layout.example("app_1").has_value());
    }
}

TEST(workspace, DuplicateBenches)
{
    DirTree tree({
        "cppship.toml",
        "app_1/cppship.toml",
        "app_1/benches/app_1.cpp",
        "app_2/cppship.toml",
        "app_2/benches/app_1.cpp",
    });

    write("cppship.toml", R"([workspace]
members = ["app_1", "app_2"])");
    write("app_1/cppship.toml", R"([package]
version = "1.0.0"
name = "app_1")");
    write("app_2/cppship.toml", R"([package]
version = "1.0.0"
name = "app_2")");

    Manifest manifest(tree.root() / "cppship.toml");
    Workspace workspace(tree.root(), manifest);

    for (const auto& layout : workspace.layouts()) {
        ASSERT_TRUE(layout.bench("app_1").has_value());
    }
}

TEST(workspace, DuplicateTests)
{
    DirTree tree({
        "cppship.toml",
        "app_1/cppship.toml",
        "app_1/tests/app_1.cpp",
        "app_2/cppship.toml",
        "app_2/tests/app_1.cpp",
    });

    write("cppship.toml", R"([workspace]
members = ["app_1", "app_2"])");
    write("app_1/cppship.toml", R"([package]
version = "1.0.0"
name = "app_1")");
    write("app_2/cppship.toml", R"([package]
version = "1.0.0"
name = "app_2")");

    Manifest manifest(tree.root() / "cppship.toml");
    Workspace workspace(tree.root(), manifest);

    for (const auto& layout : workspace.layouts()) {
        ASSERT_TRUE(layout.test("app_1").has_value());
    }
}

TEST(workspace, DuplicateInDifferentKind)
{
    DirTree tree({
        "cppship.toml",
        "app_1/cppship.toml",
        "app_1/examples/key0.cpp",
        "app_1/examples/key1.cpp",
        "app_1/src/bin/key0.cpp",
        "app_1/src/bin/key1.cpp",
        "app_2/cppship.toml",
        "app_2/tests/key0.cpp",
        "app_2/tests/key1.cpp",
        "app_2/benches/key0.cpp",
        "app_2/benches/key1.cpp",
    });

    write("cppship.toml", R"([workspace]
members = ["app_1", "app_2"])");
    write("app_1/cppship.toml", R"([package]
version = "1.0.0"
name = "app_1")");
    write("app_2/cppship.toml", R"([package]
version = "1.0.0"
name = "app_2")");

    Manifest manifest(tree.root() / "cppship.toml");
    Workspace workspace(tree.root(), manifest);
}
