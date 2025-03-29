#pragma once

#include <string_view>

#include "cppship/cmd/build.h"

namespace cppship::msvc {

// msvc generator has different binary path, fix it
fs::path fix_bin_path(const cppship::cmd::BuildContext& ctx, std::string_view bin);

}