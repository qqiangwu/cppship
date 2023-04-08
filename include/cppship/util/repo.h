#pragma once

#include <filesystem>
#include <set>
#include <string_view>

#include "cppship/util/fs.h"

namespace cppship {

constexpr std::string_view kRepoConfigFile = "cppship.toml";
constexpr std::string_view kIncludePath = "include";
constexpr std::string_view kSrcPath = "src";
constexpr std::string_view kLibPath = "lib";
constexpr std::string_view kTestsPath = "tests";
constexpr std::string_view kBenchesPath = "benches";

constexpr std::string_view kRepoHead = "HEAD";

fs::path get_project_root();

std::set<fs::path> list_all_files();
std::set<fs::path> list_changed_files(std::string_view commit);

}