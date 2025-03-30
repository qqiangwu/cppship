#pragma once

#include "cppship/core/profile.h"

namespace cppship::cmd {

struct InstallOptions {
    Profile profile = Profile::debug;
    std::string root;
};

int run_install(const InstallOptions& options);

}
