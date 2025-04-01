#include "cppship/util/cmd_runner.h"

#include "cppship/util/cmd.h"

namespace cppship::util {

int CmdRunner::run(std::string_view cmd) const
{
    if (mHook) {
        return mHook(cmd);
    }

    return run_cmd(cmd);
}

}
