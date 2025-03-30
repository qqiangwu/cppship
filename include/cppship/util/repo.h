#pragma once

#include <set>
#include <string_view>

#include "cppship/util/fs.h"

// This file describle the layout of a cppship project

namespace cppship {

inline constexpr std::string_view kRepoConfigFile = "cppship.toml";
inline constexpr std::string_view kIncludePath = "include";
inline constexpr std::string_view kSrcPath = "src";
inline constexpr std::string_view kBinPath = "bin";
inline constexpr std::string_view kLibPath = "lib";
inline constexpr std::string_view kTestsPath = "tests";
inline constexpr std::string_view kBenchesPath = "benches";
inline constexpr std::string_view kExamplesPath = "examples";

inline constexpr std::string_view kInnerTestSuffix = "_test.cpp";

// Layout of build dir
//  debug/ or release/: cmake build directory(the profile directory)
//    conan_profile
//    inventory.toml: source file list
//    dependency.toml: resolved conan+git dependencies
//  deps: cppship packages, git packages
//  packages: package cmake config
//    <package-1>.cmake
//    <package-2>.cmake
//  CMakeLists.txt: the generated cmake file
//  conanfile.txt: conan dependencies
//  git_dep.txt: git dependencies
inline constexpr std::string_view kBuildPath = "build";
inline constexpr std::string_view kBuildPackagesPath = "packages";
// all non-conan deps will be put here
inline constexpr std::string_view kBuildDepsPath = "deps";

inline constexpr std::string_view kRepoHead = "HEAD";

fs::path get_project_root();

fs::path get_package_root();

struct ListOptions {
    bool cached_only = true;
    std::string_view commit = kRepoHead;
};

std::set<fs::path> list_sources(std::string_view dir);
std::set<fs::path> list_sources(const fs::path& source_dir);
std::set<fs::path> list_cpp_files(const fs::path& dir);
std::set<fs::path> list_all_files();

std::set<fs::path> list_changed_files(const ListOptions& options = {});

}
