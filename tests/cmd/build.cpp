#include "cppship/cmd/build.h"

#include <gtest/gtest.h>

#include "cppship/core/profile.h"
#include "cppship/util/io.h"

namespace cppship {

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
                const auto parent = fs_path.parent_path();
                if (!parent.empty()) {
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

TEST(build, is_expired)
{
    const std::set<std::string_view> package_manifests = {
        "cppship.toml",
        "package-1/cppship.toml",
        "dir/package-2/cppship.toml",
        "dir/package-3/cppship.toml",
    };
    DirTree tree(package_manifests);

    write(tree.root() / "cppship.toml", R"(
[workspace]
members = ["package-1", "dir/package-2", "dir/package-3"])");
    write(tree.root() / "package-1/cppship.toml", R"(
[package]
version = "1.0.0"
name = "p1")");
    write(tree.root() / "dir/package-2/cppship.toml", R"(
[package]
version = "1.0.0"
name = "p2")");
    write(tree.root() / "dir/package-3/cppship.toml", R"(
[package]
version = "1.0.0"
name = "p3")");

    const auto test_file = tree.root() / "test";
    cmd::BuildContext ctx(Profile::debug);
    ASSERT_TRUE(ctx.is_expired(test_file));

    touch(test_file);
    ASSERT_TRUE(!ctx.is_expired(test_file));

    for (const auto manifest : package_manifests) {
        touch(manifest);
        ASSERT_TRUE(ctx.is_expired(test_file));

        touch(test_file);
        ASSERT_TRUE(!ctx.is_expired(test_file));
    }
}

}
