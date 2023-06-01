#include "cppship/cmake/cfg_predicate.h"

#include <cstdlib>

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>
#include <range/v3/view/transform.hpp>

using namespace cppship;
using ranges::views::transform;

namespace {

struct CfgOptionGen {
    // see https://gitlab.kitware.com/cmake/community/-/wikis/doc/tutorials/How-To-Write-Platform-Checks
    std::string operator()(core::cfg::Os os)
    {
        using core::cfg::Os;

        switch (os) {
        case Os::windows:
            return R"(CMAKE_SYSTEM_NAME STREQUAL "Windows")";

        case Os::macos:
            return R"(CMAKE_SYSTEM_NAME STREQUAL "Darwin")";

        case Os::linux:
            return R"(CMAKE_SYSTEM_NAME STREQUAL "Linux")";
        }

        std::abort();
    }

    // see https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html
    std::string operator()(core::cfg::Compiler compiler)
    {
        using core::cfg::Compiler;

        switch (compiler) {
        case Compiler::gcc:
            return R"(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")";

        case Compiler::msvc:
            return R"(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")";

        case Compiler::clang:
            return R"(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")";

        case Compiler::apple_clang:
            return R"(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")";
        }

        std::abort();
    }
};

// NOLINTBEGIN(misc-no-recursion)
struct CfgGen {
    std::string operator()(core::CfgOption cfg) const { return std::visit(CfgOptionGen {}, cfg); }

    std::string operator()(const boost::recursive_wrapper<core::CfgAll>& wrapper) const
    {
        const auto& cfg = wrapper.get();
        auto str = boost::join(
            cfg.predicates | transform([](const auto& opt) { return cmake::generate_predicate(opt); }), " AND ");
        if (cfg.predicates.size() > 1) {
            str = fmt::format("({})", str);
        }

        return str;
    }

    std::string operator()(const boost::recursive_wrapper<core::CfgAny>& wrapper) const
    {
        const auto& cfg = wrapper.get();
        auto str = boost::join(
            cfg.predicates | transform([](const auto& opt) { return cmake::generate_predicate(opt); }), " OR ");
        if (cfg.predicates.size() > 1) {
            str = fmt::format("({})", str);
        }

        return str;
    }

    std::string operator()(const boost::recursive_wrapper<core::CfgNot>& wrapper) const
    {
        const auto& cfg = wrapper.get();
        return fmt::format("NOT ({})", cmake::generate_predicate(cfg.predicate));
    }
};

}

std::string cmake::generate_predicate(const core::CfgPredicate& cfg) { return std::visit(CfgGen {}, cfg); }

// NOLINTEND(misc-no-recursion)
