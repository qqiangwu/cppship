#include "cppship/util/repo.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>

#include <range/v3/algorithm/contains.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>

#include "cppship/exception.h"
#include "cppship/util/cmd.h"
#include "cppship/util/fs.h"

using namespace cppship;
using namespace ranges;

namespace {

constexpr std::array kSourceExtension { ".cpp", ".h" };

}

fs::path cppship::get_project_root()
{
    auto current_dir = fs::current_path();

    while (true) {
        if (fs::exists(current_dir / kRepoConfigFile)) {
            return current_dir;
        }

        auto parent = current_dir.parent_path();
        if (parent == current_dir) {
            break;
        }

        current_dir.swap(parent);
    }

    throw Error { "not cppship repository" };
}

std::set<fs::path> cppship::list_sources(std::string_view dir)
{
    const auto root = get_project_root();
    const auto source_dir = root / dir;
    if (!fs::exists(source_dir)) {
        return {};
    }

    return list_sources(source_dir);
}

std::set<fs::path> cppship::list_sources(const fs::path& source_dir)
{
    std::set<fs::path> files;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator { source_dir }) {
        if (entry.path().extension() == ".cpp") {
            files.insert(entry.path());
        }
    }

    return files;
}

std::set<fs::path> cppship::list_all_files()
{
    const auto root_dir = get_project_root();

    std::set<fs::path> files;

    for (const auto subdir : { kIncludePath, kSrcPath, kLibPath, kTestsPath, kBenchesPath }) {
        const auto dir = root_dir / subdir;
        if (!fs::exists(dir)) {
            continue;
        }

        for (const fs::directory_entry& dentry : fs::recursive_directory_iterator { dir }) {
            if (ranges::contains(kSourceExtension, dentry.path().extension())) {
                files.insert(dentry.path());
            }
        }
    }

    return files;
}

std::set<fs::path> cppship::list_changed_files(const ListOptions& options)
{
    const auto root = get_project_root();
    if (!fs::exists(root / ".git")) {
        return list_all_files();
    }

    const auto cmd = fmt::format("git diff --name-only {}", (options.cached_only ? "--cached" : ""));
    const auto out = check_output(cmd);

    std::set<std::string> lines;
    boost::split(lines, out, boost::is_any_of("\n"));

    return lines | views::filter([](std::string_view line) { return !line.empty(); })
        | views::transform(
            [root = root.string()](std::string_view line) { return fs::path(fmt::format("{}/{}", root, line)); })
        | views::filter([](const fs::path& path) { return ranges::contains(kSourceExtension, path.extension()); })
        | views::filter([](const fs::path& path) { return fs::exists(path); }) | to<std::set<fs::path>>();
}