#include <gtest/gtest.h>

#include "cppship/core/cfg.h"

using namespace cppship;
using namespace cppship::core;

namespace {

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
    EXPECT_TRUE(cfg == pred) << cfg_str;
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