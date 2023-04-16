#pragma once

#include <string_view>

#include <boost/process/exception.hpp>
#include <boost/process/io.hpp>
#include <boost/process/system.hpp>
#include <fmt/core.h>

#include "cppship/exception.h"
#include "cppship/util/io.h"

namespace cppship {

inline bool has_cmd(std::string_view cmd)
{
    using namespace boost::process;

    try {
        return boost::process::system(fmt::format("type {}", cmd), std_err > null, std_out > null, shell) == 0;
    } catch (const process_error&) {
        return false;
    }
}

inline void require_cmd(std::string_view cmd)
{
    if (!has_cmd(cmd)) {
        throw CmdNotFound { std::string { cmd } };
    }
}

inline std::string check_output(std::string_view cmd)
{
    using namespace boost::process;

    pstream pipe;
    const int res = system(std::string(cmd), std_out > pipe, shell);
    if (res != 0) {
        throw RunCmdFailed(res, cmd);
    }

    return read_as_string(pipe);
}

}
