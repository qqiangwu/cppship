#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>

#include "cppship/core/cfg.h"
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

enum class Profile : std::uint8_t { debug, release };

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

struct ProfileConfig {
    std::vector<std::string> cxxflags;
    std::vector<std::string> linkflags;
    std::vector<std::string> definitions;
    std::optional<bool> ubsan;
    std::optional<bool> tsan;
    std::optional<bool> asan;
    std::optional<bool> leak;
};

struct ConditionConfig {
    core::CfgPredicate condition;
    ProfileConfig config;
};

struct ProfileOptions {
    ProfileConfig config;
    std::vector<ConditionConfig> conditional_configs;
};

}
