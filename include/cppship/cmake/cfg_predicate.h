#pragma once

#include <string>

#include "cppship/core/cfg.h"

namespace cppship::cmake {

std::string generate_predicate(const core::CfgPredicate& cfg);

}