#pragma once

#include <string>
#include <string_view>
#include <vector>

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
        return kProfileDebug;

    case Profile::release:
        return kProfileRelease;
    }

    std::abort();
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

struct ProfileOptions {
    std::string cxxflags;
    std::vector<std::string> definitions;
};

}