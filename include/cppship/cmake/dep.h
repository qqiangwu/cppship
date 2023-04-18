#pragma once

#include <string>
#include <vector>

namespace cppship::cmake {

struct Dep {
    std::string cmake_package;
    std::vector<std::string> cmake_targets;
};

}