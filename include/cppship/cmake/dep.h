#pragma once

#include "cppship/core/dependency.h"
#include "cppship/core/manifest.h"

#include <string>
#include <vector>

namespace cppship::cmake {

struct Dep {
    std::string cmake_package;
    std::vector<std::string> cmake_targets;
};

std::vector<Dep> collect_cmake_deps(
    const std::vector<DeclaredDependency>& declared_deps, const ResolvedDependencies& deps);

}