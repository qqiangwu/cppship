#pragma once

#include "cppship/core/dependency.h"

namespace cppship {

void merge_to(const std::vector<DeclaredDependency>& new_deps, std::vector<DeclaredDependency>& full_deps);

bool is_compatible(const DeclaredDependency& d1, const DeclaredDependency& d2);

}
