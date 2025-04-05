#pragma once

#include "cppship/core/profile.h"

namespace cppship::cmd {

struct InstallOptions {
    Profile profile = Profile::debug;
    std::optional<std::string> binary;
};

int run_install(const InstallOptions& options);

}
