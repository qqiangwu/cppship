#pragma once

#include "cppship/util/fs.h"

namespace cppship {

void generate_lib_template(const fs::path& dir);

void generate_bin_template(const fs::path& dir);

}