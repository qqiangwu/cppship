#pragma once

#include <string>
#include <string_view>

#include <boost/algorithm/string/case_conv.hpp>

#include "cppship/exception.h"

namespace cppship {

struct InvalidProfile : public Error {
    InvalidProfile()
        : Error("invalid profile, only Debug/Release is supported")
    {
    }
};

inline constexpr std::string_view kProfileDebug = "Debug";
inline constexpr std::string_view kProfileRelease = "Release";

enum class Profile { debug, release };

inline std::string_view to_string(Profile profile)
{
    switch (profile) {
    case Profile::debug:
        return "Debug";

    case Profile::release:
        return "Release";
    }

    std::terminate();
}

inline Profile parse_profile(std::string profile)
{
    boost::to_lower(profile);

    if (profile == "debug") {
        return Profile::debug;
    }

    if (profile == "release") {
        return Profile::release;
    }

    throw InvalidProfile {};
}

}