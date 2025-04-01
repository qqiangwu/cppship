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

    [[nodiscard]] std::string_view cmd() const noexcept { return mCmd; }

private:
    std::string mCmd;
};

class InvalidCmdOption : public Error {
public:
    explicit InvalidCmdOption(std::string_view option, const std::string& msg)
        : Error(msg)
        , mOption(option)
    {
    }

private:
    std::string mOption;
};

class RunCmdFailed : public Error {
public:
    RunCmdFailed(const int status, std::string_view cmd)
        : Error("run cmd failed")
        , mStatus(status)
        , mCmd(cmd)
    {
    }

    int status() const { return mStatus; }

    std::string_view cmd() const { return mCmd; }

private:
    int mStatus;
    std::string mCmd;
};

class IOError : public Error {
public:
    using Error::Error;
};

class LayoutError : public Error {
    using Error::Error;
};

}
