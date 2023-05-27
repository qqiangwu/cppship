#pragma once

#include <string_view>

// boost::process depends on boost::system but not include it
// clang-format off
#include <boost/system/error_code.hpp>
#include <boost/process/exception.hpp>
#include <boost/process/io.hpp>
#include <boost/process/system.hpp>
// clang-format on

#include "cppship/exception.h"

namespace cppship {

bool has_cmd(std::string_view cmd);

inline void require_cmd(std::string_view cmd)
{
    if (!has_cmd(cmd)) {
        throw CmdNotFound { std::string { cmd } };
    }
}

// output can be suppress by -q
int run_cmd(std::string_view cmd);

std::string check_output(std::string_view cmd);

}
