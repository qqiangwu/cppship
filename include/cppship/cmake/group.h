#pragma once

#include <string_view>

namespace cppship::cmake {

inline constexpr std::string_view kCppshipGroupLib = "cppship_group_lib";
inline constexpr std::string_view kCppshipGroupBinaries = "cppship_group_binaries";
inline constexpr std::string_view kCppshipGroupTests = "cppship_group_tests";
inline constexpr std::string_view kCppshipGroupBenches = "cppship_group_benches";
inline constexpr std::string_view kCppshipGroupExamples = "cppship_group_examples";

}