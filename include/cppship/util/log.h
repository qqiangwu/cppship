#pragma once

#include <cassert>

#include <fmt/color.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

namespace cppship {

template <class... Args> inline void debug(fmt::format_string<Args...> fmt, Args&&... args)
{
    spdlog::debug("{} {}", fmt::styled("Debug", fmt::fg(fmt::color::gray) | fmt::emphasis::bold),
        fmt::format(fmt, std::forward<Args>(args)...));
}

template <class... Args> inline void status(std::string_view event, fmt::format_string<Args...> fmt, Args&&... args)
{
    assert(event.size() <= 15);

    spdlog::info("{:>15} {}", fmt::styled(event, fmt::fg(fmt::color::green) | fmt::emphasis::bold),
        fmt::format(fmt, std::forward<Args>(args)...));
}

template <class... Args> inline void warn(fmt::format_string<Args...> fmt, Args&&... args)
{
    spdlog::info("{:>15} {}", fmt::styled("warn", fmt::fg(fmt::color::yellow) | fmt::emphasis::bold),
        fmt::format(fmt, std::forward<Args>(args)...));
}

template <class... Args> inline void error(fmt::format_string<Args...> fmt, Args&&... args)
{
    spdlog::info("{:>15} {}", fmt::styled("error", fmt::fg(fmt::color::red) | fmt::emphasis::bold),
        fmt::format(fmt, std::forward<Args>(args)...));
}

}