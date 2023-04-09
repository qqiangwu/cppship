#pragma once

#include <string_view>

#include <boost/process/io.hpp>
#include <boost/process/system.hpp>
#include <fmt/core.h>

#include "cppship/exception.h"

namespace cppship {

inline void require_cmd(std::string_view cmd)
{
    using boost::process::null;
    using boost::process::std_err;
    using boost::process::std_out;

    const int status = boost::process::system(fmt::format("type {}", cmd), std_err > null, std_out > null);
    if (status != 0) {
        throw CmdNotFound { std::string { cmd } };
    }
}

}
