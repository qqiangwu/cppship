#pragma once

#include <exception>
#include <string_view>

#include <fmt/core.h>

namespace cppship {

inline void enforce(bool expr, std::string_view msg)
{
    if (!expr) {
        fmt::println(stderr, "unexpected: {}", msg);
        std::terminate();
    }
}

}