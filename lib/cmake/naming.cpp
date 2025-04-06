#include "cppship/cmake/naming.h"

#include <fmt/core.h>

using namespace cppship::cmake;

std::string NameTargetMapper::binary(std::string_view name) { return fmt::format("{}_bin", name); }

std::string NameTargetMapper::test(std::string_view name) { return fmt::format("{}_{}_test", mPackage, name); }

std::string NameTargetMapper::example(std::string_view name) { return fmt::format("{}_{}_example", mPackage, name); }

std::string NameTargetMapper::bench(std::string_view name) { return fmt::format("{}_{}_bench", mPackage, name); }
