#pragma once

#include <sstream>

#include "cppship/util/fs.h"

namespace cppship {

void write(const fs::path& file, std::string_view content);

std::string read_as_string(const fs::path& file);

std::string read_as_string(std::istream& iss);

}