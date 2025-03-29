#include "cppship/util/cmd.h"

#include <boost/process/env.hpp>
#include <boost/process/search_path.hpp>
#include <spdlog/spdlog.h>

#include "cppship/util/io.h"

using namespace cppship;
using namespace boost::process;

bool cppship::has_cmd(std::string_view cmd)
{
    const auto path = search_path(cmd);
    return !path.empty();
}

int cppship::run_cmd(const std::string_view cmd)
{
    if (spdlog::should_log(spdlog::level::info)) {
        // unset CMAKE_GENERATOR: https://github.com/qqiangwu/cppship/issues/75
        return system(std::string { cmd }, shell, env["CMAKE_GENERATOR"] = boost::none);
    }

    ipstream pipe;
    child exe(std::string { cmd }, (std_out & std_err) > pipe, shell, env["CMAKE_GENERATOR"] = boost::none);

    std::string line;
    while (exe.running() && std::getline(pipe, line)) {
        spdlog::info(line);
    }

    exe.wait();
    return exe.exit_code();
}

std::string cppship::check_output(std::string_view cmd)
{
    pstream pipe;
    const int res = system(std::string(cmd), std_out > pipe, shell);
    if (res != 0) {
        throw RunCmdFailed(res, cmd);
    }

    return read_as_string(pipe);
}
