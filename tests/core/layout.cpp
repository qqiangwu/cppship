#include "cppship/core/layout.h"
#include "cppship/exception.h"

#include <fmt/core.h>
#include <fstream>
#include <gtest/gtest.h>
#include <set>
#include <string>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

using namespace cppship;
using namespace ranges::views;

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
                fs::create_directories(fs_path.parent_path());
                std::ofstream ofs(fs_path);
            }
        }
    }

    ~DirTree()
    {
        fs::remove_all(mRoot);
        fs::current_path(mOriPath);
    }

    const fs::path& root() const { return mRoot; }

    std::string relative(const fs::path& path) { return path.lexically_relative(mRoot).string(); }

private:
    fs::path mRoot = fs::temp_directory_path() / "cppship.test";
    fs::path mOriPath = fs::current_path();
};

constexpr std::string_view kApp = "cppship";

}

TEST(layout, main)
{
    DirTree tree({
        "src/main.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(!layout.lib());
    EXPECT_TRUE(layout.tests().empty());
    EXPECT_TRUE(layout.examples().empty());
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_EQ(layout.all_files().size(), 1);
    EXPECT_EQ(layout.binaries().size(), 1);
    EXPECT_TRUE(!layout.binary("main"));
    EXPECT_TRUE(layout.binary(kApp));

    auto target = layout.binary(kApp);
    EXPECT_EQ(target->name, kApp);
    EXPECT_EQ(target->includes.size(), 1);
    EXPECT_EQ(tree.relative(*target->includes.begin()), "src");
    EXPECT_EQ(target->sources.size(), 1);
    EXPECT_EQ(tree.relative(*target->sources.begin()), "src/main.cpp");
}

TEST(layout, bin)
{
    DirTree tree({
        "src/main.cpp",
        "src/a.cpp",
        "src/sub/a.cpp",
        "src/bin/a.cpp",
        "src/bin/b.cpp",
        "src/bin/sub/a.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(!layout.lib());
    EXPECT_TRUE(layout.tests().empty());
    EXPECT_TRUE(layout.examples().empty());
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_EQ(layout.all_files().size(), 5);
    EXPECT_EQ(layout.binaries().size(), 3);
    EXPECT_TRUE(!layout.binary("main"));
    EXPECT_TRUE(layout.binary(kApp));
    EXPECT_TRUE(layout.binary("a"));
    EXPECT_TRUE(layout.binary("b"));

    auto target = layout.binary(kApp);
    EXPECT_EQ(target->name, kApp);
    EXPECT_EQ(target->includes.size(), 1);
    EXPECT_EQ(tree.relative(*target->includes.begin()), "src");
    EXPECT_EQ(target->sources.size(), 3);

    const auto app_sources = target->sources | transform([&tree](const fs::path& path) { return tree.relative(path); })
        | ranges::to<std::set<std::string>>();
    EXPECT_TRUE(app_sources.contains("src/main.cpp"));
    EXPECT_TRUE(app_sources.contains("src/a.cpp"));
    EXPECT_TRUE(app_sources.contains("src/sub/a.cpp"));

    for (const char* name : { "a", "b" }) {
        auto target = layout.binary(name);
        EXPECT_EQ(target->name, name);
        EXPECT_TRUE(target->includes.empty());
        EXPECT_EQ(target->sources.size(), 1);
        EXPECT_EQ(tree.relative(*target->sources.begin()), fmt::format("src/bin/{}.cpp", name));
    }
}

TEST(layout, dup_binary)
{
    DirTree tree({
        "src/main.cpp",
        fmt::format("src/bin/{}.cpp", kApp),
    });

    EXPECT_THROW(Layout(tree.root(), kApp), LayoutError);
}

TEST(layout, examples)
{
    DirTree tree({
        "examples/a.cpp",
        "examples/b.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(!layout.lib());
    EXPECT_TRUE(layout.tests().empty());
    EXPECT_TRUE(layout.binaries().empty());
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_EQ(layout.all_files().size(), 2);

    EXPECT_TRUE(!layout.example(kApp));
    EXPECT_TRUE(layout.example("a"));
    EXPECT_TRUE(layout.example("b"));
    EXPECT_EQ(layout.examples().size(), 2);

    for (const char* name : { "a", "b" }) {
        auto target = layout.example(name);
        EXPECT_EQ(target->name, name);
        EXPECT_TRUE(target->includes.empty());
        EXPECT_EQ(target->sources.size(), 1);
        EXPECT_EQ(tree.relative(*target->sources.begin()), fmt::format("examples/{}.cpp", name));
    }
}

TEST(layout, bin_examples_conflict)
{
    DirTree tree({
        fmt::format("examples/{}.cpp", kApp),
        "examples/x.cpp",
        "src/main.cpp",
        "src/bin/x.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(!layout.lib());
    EXPECT_TRUE(layout.tests().empty());
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_EQ(layout.all_files().size(), 4);

    EXPECT_EQ(layout.binaries().size(), 2);
    EXPECT_EQ(layout.examples().size(), 2);
    EXPECT_TRUE(layout.example(kApp));
    EXPECT_TRUE(layout.example("x"));
    EXPECT_TRUE(layout.binary(kApp));
    EXPECT_TRUE(layout.binary("x"));
}

TEST(layout, benches)
{
    DirTree tree({
        "benches/a.cpp",
        "benches/b.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(!layout.lib());
    EXPECT_TRUE(layout.tests().empty());
    EXPECT_TRUE(layout.binaries().empty());
    EXPECT_TRUE(layout.examples().empty());
    EXPECT_EQ(layout.all_files().size(), 2);

    EXPECT_TRUE(!layout.bench(kApp));
    EXPECT_TRUE(layout.bench("a"));
    EXPECT_TRUE(layout.bench("b"));
    EXPECT_EQ(layout.benches().size(), 2);

    for (const char* name : { "a", "b" }) {
        auto target = layout.bench(name);
        EXPECT_EQ(target->name, name);
        EXPECT_TRUE(target->includes.empty());
        EXPECT_EQ(target->sources.size(), 1);
        EXPECT_EQ(tree.relative(*target->sources.begin()), fmt::format("benches/{}.cpp", name));
    }
}

TEST(layout, bin_bench_example_conflict)
{
    DirTree tree({
        "benches/a.cpp",
        "benches/a.h",
        "examples/a.cpp",
        "examples/a.h",
        "src/bin/a.cpp",
        "src/bin/a.h",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(!layout.lib());
    EXPECT_TRUE(layout.tests().empty());
    EXPECT_EQ(layout.all_files().size(), 3);
    EXPECT_EQ(layout.benches().size(), 1);
    EXPECT_EQ(layout.examples().size(), 1);
    EXPECT_EQ(layout.binaries().size(), 1);
}

TEST(layout, tests)
{
    DirTree tree({
        "tests/a.cpp",
        "tests/b.cpp",
        "tests/sub/a.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(!layout.lib());
    EXPECT_EQ(layout.all_files().size(), 3);
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_TRUE(layout.examples().empty());
    EXPECT_TRUE(layout.binaries().empty());
    EXPECT_EQ(layout.tests().size(), 3);

    for (const char* name : { "a", "b" }) {
        const auto target = layout.test(name);
        EXPECT_EQ(target->name, name);
        EXPECT_TRUE(target->includes.empty());
        EXPECT_EQ(target->sources.size(), 1);
        EXPECT_EQ(tree.relative(*target->sources.begin()), fmt::format("tests/{}.cpp", name));
    }

    const auto target = layout.test("sub_a");
    EXPECT_EQ(target->name, "sub_a");
    EXPECT_TRUE(target->includes.empty());
    EXPECT_EQ(target->sources.size(), 1);
    EXPECT_EQ(tree.relative(*target->sources.begin()), "tests/sub/a.cpp");
}

TEST(layout, inner_test)
{
    DirTree tree({
        "lib/a_test.cpp",
        "lib/b_test.cpp",
        "lib/sub/a_test.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(!layout.lib());
    EXPECT_EQ(layout.all_files().size(), 3);
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_TRUE(layout.examples().empty());
    EXPECT_TRUE(layout.binaries().empty());
    EXPECT_EQ(layout.tests().size(), 3);

    for (const char* name : { "a", "b" }) {
        const auto target = layout.test(name);
        EXPECT_EQ(target->name, name);
        EXPECT_TRUE(target->includes.empty());
        EXPECT_EQ(target->sources.size(), 1);
        EXPECT_EQ(tree.relative(*target->sources.begin()), fmt::format("lib/{}_test.cpp", name));
    }

    const auto target = layout.test("sub_a");
    EXPECT_EQ(target->name, "sub_a");
    EXPECT_TRUE(target->includes.empty());
    EXPECT_EQ(target->sources.size(), 1);
    EXPECT_EQ(tree.relative(*target->sources.begin()), "lib/sub/a_test.cpp");
}

TEST(layout, dup_test)
{
    DirTree tree({
        "lib/a_test.cpp",
        "tests/a.cpp",
    });

    EXPECT_THROW(Layout(tree.root(), kApp), LayoutError);
}

TEST(layout, lib)
{
    DirTree tree({
        "include/app/a.h",
        "lib/a.cpp",
        "lib/b.cpp",
        "lib/sub/a.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(layout.lib());
    EXPECT_EQ(layout.all_files().size(), 3);
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_TRUE(layout.examples().empty());
    EXPECT_TRUE(layout.binaries().empty());
    EXPECT_TRUE(layout.tests().empty());

    const auto lib = *layout.lib();
    EXPECT_EQ(lib.name, kApp);
    EXPECT_EQ(lib.includes.size(), 1);
    EXPECT_EQ(tree.relative(*lib.includes.begin()), "include");
    EXPECT_EQ(lib.sources.size(), 3);

    const auto lib_sources = lib.sources | transform([&tree](const fs::path& path) { return tree.relative(path); })
        | ranges::to<std::set<std::string>>();
    EXPECT_TRUE(lib_sources.contains("lib/a.cpp"));
    EXPECT_TRUE(lib_sources.contains("lib/b.cpp"));
    EXPECT_TRUE(lib_sources.contains("lib/sub/a.cpp"));
}

TEST(layout, src_only_lib)
{
    DirTree tree({
        "lib/a.cpp",
        "lib/b.cpp",
        "lib/sub/a.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(layout.lib());
    EXPECT_EQ(layout.all_files().size(), 3);
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_TRUE(layout.examples().empty());
    EXPECT_TRUE(layout.binaries().empty());
    EXPECT_TRUE(layout.tests().empty());

    const auto lib = *layout.lib();
    EXPECT_EQ(lib.name, kApp);
    EXPECT_EQ(lib.includes.size(), 1);
    EXPECT_EQ(tree.relative(*lib.includes.begin()), "include");
    EXPECT_EQ(lib.sources.size(), 3);

    const auto lib_sources = lib.sources | transform([&tree](const fs::path& path) { return tree.relative(path); })
        | ranges::to<std::set<std::string>>();
    EXPECT_TRUE(lib_sources.contains("lib/a.cpp"));
    EXPECT_TRUE(lib_sources.contains("lib/b.cpp"));
    EXPECT_TRUE(lib_sources.contains("lib/sub/a.cpp"));
}

TEST(layout, header_only_lib)
{
    DirTree tree({
        "include/",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(layout.lib());
    EXPECT_TRUE(layout.all_files().empty());
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_TRUE(layout.examples().empty());
    EXPECT_TRUE(layout.binaries().empty());
    EXPECT_TRUE(layout.tests().empty());

    const auto lib = *layout.lib();
    EXPECT_EQ(lib.name, kApp);
    EXPECT_EQ(lib.includes.size(), 1);
    EXPECT_EQ(tree.relative(*lib.includes.begin()), "include");
    EXPECT_EQ(lib.sources.size(), 0);
}

TEST(layout, lib_test_max)
{
    DirTree tree({
        "lib/a.cpp",
        "lib/a_test.cpp",
        "lib/b.cpp",
        "lib/b_test.cpp",
        "lib/sub/a.cpp",
        "lib/sub/a_test.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_TRUE(layout.lib());
    EXPECT_EQ(layout.all_files().size(), 6);
    EXPECT_TRUE(layout.benches().empty());
    EXPECT_TRUE(layout.examples().empty());
    EXPECT_TRUE(layout.binaries().empty());
    EXPECT_EQ(layout.tests().size(), 3);

    const auto lib = *layout.lib();
    EXPECT_EQ(lib.name, kApp);
    EXPECT_EQ(lib.includes.size(), 1);
    EXPECT_EQ(tree.relative(*lib.includes.begin()), "include");
    EXPECT_EQ(lib.sources.size(), 3);

    const auto lib_sources = lib.sources | transform([&tree](const fs::path& path) { return tree.relative(path); })
        | ranges::to<std::set<std::string>>();
    EXPECT_TRUE(lib_sources.contains("lib/a.cpp"));
    EXPECT_TRUE(lib_sources.contains("lib/b.cpp"));
    EXPECT_TRUE(lib_sources.contains("lib/sub/a.cpp"));

    for (const char* name : { "a", "b" }) {
        const auto target = layout.test(name);
        EXPECT_EQ(target->name, name);
        EXPECT_TRUE(target->includes.empty());
        EXPECT_EQ(target->sources.size(), 1);
        EXPECT_EQ(tree.relative(*target->sources.begin()), fmt::format("lib/{}_test.cpp", name));
    }

    const auto target = layout.test("sub_a");
    EXPECT_EQ(target->name, "sub_a");
    EXPECT_TRUE(target->includes.empty());
    EXPECT_EQ(target->sources.size(), 1);
    EXPECT_EQ(tree.relative(*target->sources.begin()), "lib/sub/a_test.cpp");
}

TEST(layout, full)
{
    DirTree tree({
        "include/app/x.h",
        "lib/a.h",
        "lib/a.cpp",
        "lib/a_test.cpp",
        "lib/b.cpp",
        "lib/b_test.cpp",
        "lib/sub/c.cpp",
        "lib/sub/c.h",
        "lib/sub/c_test.cpp",
        "src/main.cpp",
        "src/a.cpp",
        "src/b.cpp",
        "src/a.h",
        "src/sub/a.h",
        "src/sub/a.cpp",
        "src/bin/a.cpp",
        "src/bin/b.cpp",
        "src/bin/sub/c.cpp",
        "tests/d.cpp",
        "tests/e.cpp",
        "tests/sub/f.cpp",
        "examples/a.cpp",
        "examples/b.cpp",
        "examples/sub/c.cpp",
        "benches/a.cpp",
        "benches/b.cpp",
        "benches/sub/c.cpp",
    });

    Layout layout(tree.root(), kApp);
    EXPECT_EQ(layout.all_files().size(), 19);
    EXPECT_TRUE(layout.lib());

    const auto lib = layout.lib();
    EXPECT_EQ(lib->name, kApp);
    EXPECT_EQ(lib->includes.size(), 1);
    EXPECT_EQ(lib->sources.size(), 3);

    EXPECT_EQ(layout.binaries().size(), 3);
    EXPECT_TRUE(layout.binary(kApp));
    EXPECT_TRUE(layout.binary("a"));
    EXPECT_TRUE(layout.binary("b"));
    EXPECT_EQ(layout.binary(kApp)->sources.size(), 4);

    EXPECT_EQ(layout.examples().size(), 2);
    EXPECT_TRUE(layout.example("a"));
    EXPECT_TRUE(layout.example("b"));

    EXPECT_EQ(layout.benches().size(), 2);
    EXPECT_TRUE(layout.bench("a"));
    EXPECT_TRUE(layout.bench("b"));

    EXPECT_EQ(layout.tests().size(), 6);
    EXPECT_TRUE(layout.test("a"));
    EXPECT_TRUE(layout.test("b"));
    EXPECT_TRUE(layout.test("sub_c"));
    EXPECT_TRUE(layout.test("d"));
    EXPECT_TRUE(layout.test("e"));
    EXPECT_TRUE(layout.test("sub_f"));
}