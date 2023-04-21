#include "cppship/util/cmd.h"
#include "cppship/util/log.h"

using namespace cppship;
using namespace boost::process;

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