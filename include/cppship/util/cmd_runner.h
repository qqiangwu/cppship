#pragma once

#include <functional>
#include <string_view>

namespace cppship::util {

class CmdRunner {
public:
    CmdRunner() = default;

    template <class Fn>
        requires(!std::is_same_v<Fn, CmdRunner>)
    explicit CmdRunner(Fn&& fn)
        : mHook(std::forward<Fn>(fn))
    {
    }

    int run(std::string_view cmd) const;

private:
    std::function<int(std::string_view)> mHook;
};

}
