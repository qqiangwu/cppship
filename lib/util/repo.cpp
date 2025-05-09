#include "cppship/util/repo.h"

#include <array>
#include <filesystem>
#include <optional>

#include <range/v3/algorithm/contains.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <toml/parser.hpp>

#include "cppship/exception.h"
#include "cppship/util/cmd.h"
#include "cppship/util/string.h"

using namespace cppship;
using namespace ranges;

namespace {

constexpr std::array kSourceExtension { ".cpp", ".h" };

std::optional<fs::path> get_workspace_root(const fs::path& root)
{
    auto current = root;
    while (true) {
        const auto manifest = current / kRepoConfigFile;
        if (fs::exists(manifest)) {
            const auto value = toml::parse(manifest);
            if (value.contains("workspace")) {
                return current;
            }
        }

        auto parent = current.parent_path();
        if (current == parent) {
            return std::nullopt;
        }

        current = std::move(parent);
    }
}

}

fs::path cppship::get_project_root()
{
    auto p = get_package_root();
    auto workspace_root = get_workspace_root(p);
    if (workspace_root.has_value()) {
        return std::move(workspace_root).value();
    }

    return p;
}

fs::path cppship::get_package_root()
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
    if (!fs::exists(source_dir)) {
        return {};
    }

    std::set<fs::path> files;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator { source_dir }) {
        if (entry.path().extension() == ".cpp") {
            files.insert(entry.path());
        }
    }

    return files;
}

std::set<fs::path> cppship::list_cpp_files(const fs::path& dir)
{
    if (!fs::exists(dir)) {
        return {};
    }

    std::set<fs::path> files;
    for (const fs::directory_entry& entry : fs::directory_iterator { dir }) {
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

    for (const auto subdir : { kIncludePath, kSrcPath, kLibPath, kTestsPath, kBenchesPath, kExamplesPath }) {
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

    const auto cmd = fmt::format("git diff {} --name-only {}", options.commit, (options.cached_only ? "--cached" : ""));
    const auto out = check_output(cmd);

    const auto lines = util::split(out, boost::is_any_of("\n"));

    return lines | views::filter([](std::string_view line) { return !line.empty(); })
        | views::transform(
            [root = root.string()](std::string_view line) { return fs::path(fmt::format("{}/{}", root, line)); })
        | views::filter([](const fs::path& path) { return ranges::contains(kSourceExtension, path.extension()); })
        | views::filter([](const fs::path& path) { return fs::exists(path); }) | to<std::set>();
}
