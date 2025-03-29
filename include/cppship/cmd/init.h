#pragma once

#include <optional>
#include <string>

#include "cppship/core/manifest.h"

namespace cppship::cmd {

struct InitOptions {
    bool vcs;
    fs::path dir;
    bool lib = false;
    bool bin = true;
    CxxStd std = CxxStd::cxx17;
    std::optional<std::string> name;
};

int run_init(const InitOptions& options);

}