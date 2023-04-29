#pragma once

#include <optional>
#include <string>

namespace cppship::cmd {

struct CmakeOptions { };

int run_cmake(const CmakeOptions& options);

}