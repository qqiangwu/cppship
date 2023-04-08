#include "cppship/util/repo.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>

#include <range/v3/algorithm/contains.hpp>

#include "cppship/exception.h"
#include "cppship/util/fs.h"

using namespace cppship;

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

std::set<fs::path> cppship::list_all_files()
{
    const auto root_dir = get_project_root();

    std::set<fs::path> files;

    for (const auto subdir : { kIncludePath, kSrcPath, kLibPath, kTestsPath, kBenchesPath }) {
        const auto dir = root_dir / subdir;
        if (!fs::exists(dir)) {
            continue;
        }

        for (const fs::directory_entry& e : fs::recursive_directory_iterator { dir }) {
            constexpr std::array kSourceExtension {
                ".cpp",
                ".h"
            };

            if (ranges::contains(kSourceExtension, e.path().extension())) {
                files.insert(e.path());
            }
        }
    }

    return files;
}

std::set<fs::path> cppship::list_changed_files(std::string_view commit)
{
    return list_all_files();
}