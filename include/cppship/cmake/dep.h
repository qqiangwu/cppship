#pragma once

#include "cppship/core/dependency.h"

#include <string>
#include <vector>

namespace cppship::cmake {

struct Dep {
    std::string cmake_package;
    std::vector<std::string> cmake_targets;
};

// according `ResolvedDependencies`, resolve declared deps to cmake packages and targets
std::vector<Dep> resolve_deps(const std::vector<DeclaredDependency>& declared_deps, const ResolvedDependencies& resolved);

}