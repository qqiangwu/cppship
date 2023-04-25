#include "cppship/cmake/naming.h"

#include <fmt/core.h>

using namespace cppship::cmake;

std::string NameTargetMapper::test(std::string_view name) { return fmt::format("{}_test", name); }

std::string NameTargetMapper::example(std::string_view name) { return fmt::format("{}_example", name); }

std::string NameTargetMapper::bench(std::string_view name) { return fmt::format("{}_bench", name); }