#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include <fmt/core.h>

namespace cppship {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class CmdNotFound : public Error {
public:
    explicit CmdNotFound(std::string_view cmd)
        : Error(fmt::format("command {} not found", cmd))
        , mCmd(cmd)
    {
    }

    std::string_view cmd() const noexcept
    {
        return mCmd;
    }

private:
    std::string mCmd;
};

}