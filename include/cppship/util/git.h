#pragma once

#include "cppship/util/fs.h"

#include <string_view>

namespace cppship::util {

void git_clone(std::string_view package, const fs::path& deps_dir, std::string_view git, std::string_view commit);

}