#pragma once

#include <string_view>

#include "cppship/cmd/build.h"
#include "cppship/util/fs.h"

namespace cppship::msvc {

fs::path fix_bin(const cppship::cmd::BuildContext& ctx, std::string_view bin);

}