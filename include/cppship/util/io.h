#pragma once

#include "cppship/util/fs.h"

namespace cppship {

void write(const fs::path& file, std::string_view content);

void touch(const fs::path& file);

std::string read_as_string(const fs::path& file);

std::string read_as_string(std::istream& iss);

}
