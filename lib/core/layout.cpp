#include "cppship/core/layout.h"
#include "cppship/exception.h"
#include "cppship/util/repo.h"

#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
#include <range/v3/action.hpp>
#include <range/v3/algorithm/mismatch.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>

using namespace cppship;
using namespace cppship::layout;
using namespace ranges::views;

namespace {

constexpr std::string_view kCppExtension = ".cpp";
constexpr std::string_view kTestSuffix = "_test";

}

Layout::Layout(const fs::path& root, const std::string_view name)
    : mRoot(root)
    , mName(name)
{
    scan_binaries_();
    scan_examples_();
    scan_benches_();
    scan_tests_();
    scan_lib_();
}

void Layout::scan_binaries_()
{
    std::set<fs::path> cpp4_binary_seen;

    for (const auto& path : list_cpp_files(mRoot / kSrcPath / kBinPath)) {
        mSources.insert(path);
        cpp4_binary_seen.insert(path);

        const auto name = path.stem().string();
        const auto& [_, ok] = mBinaries.emplace(name, Target { .name = name, .sources = { path } });
        if (!ok) {
            throw LayoutError { fmt::format("binary {} already exists", name) };
        }
    }

    const auto sources = list_sources(mRoot / kSrcPath);
    const auto bin_files = sources | filter([bin_dir = mRoot / kSrcPath / kBinPath](const fs::path& path) {
        return ranges::mismatch(bin_dir, path).in1 != bin_dir.end();
    }) | ranges::to<std::set<fs::path>>();
    if (!bin_files.empty()) {
        if (mBinaries.contains(mName)) {
            throw LayoutError { fmt::format("binary {} already exists", mName) };
        }

        mBinaries.emplace(mName, Target { .name = mName, .includes = { mRoot / kSrcPath }, .sources = bin_files });

        ranges::insert(mSources, bin_files);
    }
}

void Layout::scan_benches_()
{
    for (const auto& path : list_cpp_files(mRoot / kBenchesPath)) {
        mSources.insert(path);

        const auto name = path.stem().string();
        mBenches.emplace(name, Target { .name = name, .sources = { path } });
    }
}

void Layout::scan_tests_()
{
    for (const auto& path : list_sources(mRoot / kTestsPath)) {
        mSources.insert(path);

        // a/b/c.cpp => a_b_c
        const auto rel_path = path.lexically_relative(mRoot / kTestsPath).string();
        const auto name = boost::replace_all_copy(rel_path, "/", "_").substr(0, rel_path.size() - kCppExtension.size());

        mTests.emplace(name, Target { .name = name, .sources = { path } });
    }
}

void Layout::scan_examples_()
{
    for (const auto& path : list_cpp_files(mRoot / kExamplesPath)) {
        mSources.insert(path);

        const auto name = path.stem().string();
        mExamples.emplace(name, Target { .name = name, .sources = { path } });
    }
}

void Layout::scan_lib_()
{
    std::set<fs::path> lib_sources;
    std::set<fs::path> lib_test_sources;
    for (const auto& path : list_sources(mRoot / kLibPath)) {
        mSources.insert(path);

        if (path.stem().string().ends_with(kTestSuffix)) {
            lib_test_sources.insert(path);
        } else {
            lib_sources.insert(path);
        }
    }

    for (const auto& path : lib_test_sources) {
        const auto rel_path = path.lexically_relative(mRoot / kLibPath).string();
        // a/b/c_test.cpp => a_b_c
        const auto name = boost::replace_all_copy(rel_path, "/", "_")
                              .substr(0, rel_path.size() - kCppExtension.size() - kTestSuffix.size());

        const auto& [_, ok] = mTests.emplace(name, Target { .name = name, .sources = { path } });
        if (!ok) {
            throw LayoutError { fmt::format("test {} already exist", name) };
        }
    }

    if (lib_sources.empty() && !fs::exists(mRoot / kIncludePath)) {
        return;
    }
    mLib.emplace(Target {
        .name = mName,
        .includes = { mRoot / kIncludePath },
        .sources = lib_sources,
    });
}

std::set<fs::path> Layout::all_files() const { return mSources; }

std::optional<Target> Layout::lib() const { return mLib; }

std::optional<Target> Layout::binary(std::string_view name) const
{
    auto iter = mBinaries.find(name);
    if (iter == mBinaries.end()) {
        return std::nullopt;
    }

    return iter->second;
}

std::optional<Target> Layout::example(std::string_view name) const
{
    auto iter = mExamples.find(name);
    if (iter == mExamples.end()) {
        return std::nullopt;
    }

    return iter->second;
}

std::optional<Target> Layout::bench(std::string_view name) const
{
    auto iter = mBenches.find(name);
    if (iter == mBenches.end()) {
        return std::nullopt;
    }

    return iter->second;
}

std::optional<Target> Layout::test(std::string_view name) const
{
    auto iter = mTests.find(name);
    if (iter == mTests.end()) {
        return std::nullopt;
    }

    return iter->second;
}

std::vector<Target> Layout::binaries() const { return mBinaries | values | ranges::to<std::vector<Target>>(); }

std::vector<Target> Layout::examples() const { return mExamples | values | ranges::to<std::vector<Target>>(); }

std::vector<Target> Layout::benches() const { return mBenches | values | ranges::to<std::vector<Target>>(); }

std::vector<Target> Layout::tests() const { return mTests | values | ranges::to<std::vector<Target>>(); }