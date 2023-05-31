#include <gtest/gtest.h>
#include <range/v3/algorithm/equal.hpp>

#include "cppship/core/cfg.h"

using namespace cppship;
using namespace cppship::core;

namespace {

struct CfgEquals {
    bool operator()(const CfgPredicate& a, const CfgPredicate& b) const
    {
        if (a.index() != b.index()) {
            return false;
        }

        if (a.valueless_by_exception()) {
            return true;
        }

        return std::visit(CfgEquals {}, a, b);
    }

    template <class T> bool operator()(const boost::recursive_wrapper<T>& a, const boost::recursive_wrapper<T>& b) const
    {
        return (*this)(a.get(), b.get());
    }

    template <class T>
    requires std::is_same_v<T, CfgAll> || std::is_same_v<T, CfgAny>
    bool operator()(const T& a, const T& b) const { return ranges::equal(a.predicates, b.predicates, *this); }

    bool operator()(const CfgNot& a, const CfgNot& b) const { return (*this)(a.predicate, b.predicate); }

    bool operator()(const CfgOption& a, const CfgOption& b) const { return a == b; }

    template <class T1, class T2> bool operator()(const T1&, const T2&) const { return false; }
};

bool equals(const CfgPredicate& a, const CfgPredicate& b) { return CfgEquals {}(a, b); }

void expect_parse_error(std::string_view cfg, std::string_view expect)
try {
    parse_cfg(cfg);
    FAIL() << cfg;
} catch (const ManifestCfgParseError& e) {
    EXPECT_EQ(e.expect(), expect) << cfg;
}

void expect_cfg(std::string_view cfg_str, const CfgPredicate& pred)
{
    const auto cfg = parse_cfg(cfg_str);
    EXPECT_TRUE(equals(cfg, pred)) << cfg_str;
}

}

TEST(cfg, basic)
{
    expect_cfg(R"(cfg(compiler = "msvc"))", cfg::Compiler::msvc);
    expect_cfg(R"(cfg(not(compiler="msvc")))", CfgNot { cfg::Compiler::msvc });
    expect_cfg(R"(cfg(all(compiler="msvc")))", CfgAll { { cfg::Compiler::msvc } });
    expect_cfg(R"(cfg(all(compiler="msvc", os = "linux")))", CfgAll { { cfg::Compiler::msvc, cfg::Os::linux } });
    expect_cfg(R"(cfg(any(compiler="msvc")))", CfgAny { { cfg::Compiler::msvc } });
    expect_cfg(R"(cfg(any(compiler="msvc", os = "linux")))", CfgAny { { cfg::Compiler::msvc, cfg::Os::linux } });
}

TEST(cfg, error)
{
    expect_parse_error(R"(compiler = "msvc")", "cfg");
    expect_parse_error(R"(cfg(compiler = "icc"))", "<compiler>");

    expect_parse_error(R"(cfg())", "<CfgPredicate>");

    expect_parse_error(R"(cfg(all(compiler>"msvc", os="unix")))", "\"=\"");
    expect_parse_error(R"(cfg(all(compiler="msvc", os="unix")))", "<os>");
}

TEST(cfg, nested)
{
    expect_cfg(R"(cfg(all(compiler = "msvc", not(os = "linux"))))",
        CfgAll { {
            cfg::Compiler::msvc,
            CfgNot { cfg::Os::linux },
        } });
    expect_cfg(R"(cfg(all(compiler = "msvc", any(os = "linux", os = "macos"))))",
        CfgAll { {
            cfg::Compiler::msvc,
            CfgAny { { cfg::Os::linux, cfg::Os::macos } },
        } });
}