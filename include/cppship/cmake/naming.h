#pragma once

#include <string>
#include <string_view>

namespace cppship::cmake {

class NameTargetMapper {
public:
    std::string test(std::string_view name);
    std::string example(std::string_view name);
    std::string bench(std::string_view name);
};

}