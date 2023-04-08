#pragma once

#include <string>
#include <string_view>

#include <boost/process/search_path.hpp>
#include <fmt/core.h>

#include "cppship/exception.h"

namespace cppship {

void require_cmd(std::string_view cmd)
{
    std::string cmd_s { cmd };
    if (boost::process::search_path(cmd_s).empty()) {
        throw CmdNotFound { cmd_s };
    }
}

}
