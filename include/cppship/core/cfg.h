#pragma once

#include "cppship/exception.h"

#include <string_view>
#include <variant>
#include <vector>

#include <boost/variant/recursive_wrapper.hpp>
#include <fmt/core.h>

namespace cppship::core {

class ManifestCfgParseError : public Error {
public:
    ManifestCfgParseError(std::string_view expect, std::string_view rest)
        : Error(fmt::format("expect {}, next is {}", expect, rest))
        , mExpect(expect)
        , mRest(rest)
    {
    }

    std::string_view expect() const noexcept { return mExpect; }

    std::string_view rest() const noexcept { return mRest; }

private:
    std::string mExpect;
    std::string mRest;
};

namespace cfg {
    enum class Os : std::uint8_t {
        windows,
        macos,
        linux,
    };

    enum class Compiler : std::uint8_t {
        gcc,
        msvc,
        clang,
        apple_clang,
    };
}

struct CfgNot;
struct CfgAll;
struct CfgAny;

using CfgOption = std::variant<cfg::Os, cfg::Compiler>;

using CfgPredicate = std::variant<CfgOption, boost::recursive_wrapper<CfgAll>, boost::recursive_wrapper<CfgAny>,
    boost::recursive_wrapper<CfgNot>>;

struct CfgNot {
    CfgPredicate predicate;
};

struct CfgAll {
    std::vector<CfgPredicate> predicates;
};

struct CfgAny {
    std::vector<CfgPredicate> predicates;
};

bool operator==(const CfgPredicate& a, const CfgPredicate& b);

CfgPredicate parse_cfg(std::string_view cfg);

}