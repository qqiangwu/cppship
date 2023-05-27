#include "cppship/util/cmd.h"
#include "cppship/util/io.h"
#include "cppship/util/log.h"

#include <boost/process/search_path.hpp>
#include <fmt/core.h>

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
        return system(std::string { cmd }, shell);
    }

    ipstream pipe;
    child exe(std::string { cmd }, (std_out & std_err) > pipe, shell);

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