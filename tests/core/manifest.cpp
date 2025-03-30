#include "cppship/core/manifest.h"

#include <atomic>
#include <stdexcept>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include <range/v3/algorithm/sort.hpp>

#include "cppship/exception.h"
#include "cppship/util/io.h"

using namespace cppship;
using namespace cppship::core;

namespace {

PackageManifest mock_manifest(const std::string_view content)
{
    static std::atomic<int> S_COUNTER { 0 };

    const auto file = fs::temp_directory_path() / std::to_string(++S_COUNTER);
    write(file, content);

    Manifest m { file };
    const auto* p = m.get_if_package();
    if (p == nullptr) {
        throw std::runtime_error { "invalid manifest" };
    }

    return *p;
}

}

TEST(manifest, PackageBasicFields)
{
    EXPECT_THROW(mock_manifest(""), Error);
    EXPECT_THROW(mock_manifest("[package]"), Error);
    EXPECT_THROW(mock_manifest(R"([package]
    name = "abc")"),
        Error);

    const auto meta = mock_manifest(R"([package]
    name = "abc"
    version = "0.1.0"
    )");

    EXPECT_EQ(meta.name(), "abc");
    EXPECT_EQ(meta.version(), "0.1.0");
    EXPECT_EQ(meta.cxx_std(), CxxStd::cxx17);
    EXPECT_TRUE(meta.dependencies().empty());
}

TEST(manifest, PackageStd)
{
    auto meta = mock_manifest(R"([package]
    name = "abc"
    version = "0.1.0"
    std = 11
    )");

    EXPECT_EQ(meta.name(), "abc");
    EXPECT_EQ(meta.version(), "0.1.0");
    EXPECT_EQ(meta.cxx_std(), CxxStd::cxx11);

    EXPECT_THROW(mock_manifest(R"([package]
    name = "abc"
    version = "0.1.0"
    std = 16
    )"),
        Error);
}

TEST(manifest, Dependencies)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
)");

    ASSERT_TRUE(meta.dependencies().empty());

    meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
boost = { version = "1.81.0", components = ["headers"] }
toml11 = "3.7.1"
)");

    ASSERT_EQ(meta.dependencies().size(), 2);

    auto deps = meta.dependencies();
    ranges::sort(deps, std::less<> {}, &DeclaredDependency::package);

    const auto& dep1 = deps[0];
    EXPECT_EQ(dep1.package, "boost");
    ASSERT_TRUE(dep1.is_conan());
    EXPECT_EQ(get<ConanDep>(dep1.desc).version, "1.81.0");
    EXPECT_EQ(dep1.components, std::vector<std::string> { "headers" });

    const auto& dep2 = deps[1];
    EXPECT_EQ(dep2.package, "toml11");
    ASSERT_TRUE(dep2.is_conan());
    EXPECT_EQ(get<ConanDep>(dep2.desc).version, "3.7.1");
    EXPECT_TRUE(dep2.components.empty());
}

TEST(manifest, DependenciesOptions)
{
    const auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
boost = { version = "1.81.0", components = ["headers"], options = { without_stacktrace = true, i18backend = "icu" } }
)");

    ASSERT_EQ(meta.dependencies().size(), 1);

    const auto& dep = meta.dependencies().front();

    EXPECT_EQ(dep.package, "boost");
    ASSERT_TRUE(dep.is_conan());

    const auto& desc = get<ConanDep>(dep.desc);
    EXPECT_EQ(desc.version, "1.81.0");
    EXPECT_EQ(dep.components, std::vector<std::string> { "headers" });
    EXPECT_EQ(desc.options.at("without_stacktrace"), "True");
    EXPECT_EQ(desc.options.at("i18backend"), "\"icu\"");
}

TEST(manifest, DevDependencies)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
toml11 = "3.7.1"
)");

    ASSERT_EQ(meta.dependencies().size(), 1);
    ASSERT_EQ(meta.dev_dependencies().size(), 0);

    meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
toml11 = "3.7.1"

[dev-dependencies]
boost = "1.81.0"
fmt = "9.1.0"
)");
    ASSERT_EQ(meta.dependencies().size(), 1);
    ASSERT_EQ(meta.dev_dependencies().size(), 2);

    auto deps = meta.dev_dependencies();
    ranges::sort(deps, std::less<> {}, &DeclaredDependency::package);

    EXPECT_EQ(deps[0].package, "boost");
    ASSERT_TRUE(deps[0].is_conan());
    EXPECT_EQ(get<ConanDep>(deps[0].desc).version, "1.81.0");

    EXPECT_EQ(deps[1].package, "fmt");
    ASSERT_TRUE(deps[1].is_conan());
    EXPECT_EQ(get<ConanDep>(deps[1].desc).version, "9.1.0");
}

TEST(manifest, DupDependencies)
{
    EXPECT_THROW(mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
toml11 = "3.7.1"
toml11 = "3.7.2"
)"),
        Error);

    EXPECT_THROW(mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
toml11 = "3.7.1"

[dev-dependencies]
toml11 = "3.7.2"
)"),
        Error);

    EXPECT_THROW(mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dev-dependencies]
toml11 = "3.7.1"
toml11 = "3.7.2"
)"),
        Error);
}

TEST(manifest, GitDependencies)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
scope_guard = { git = "https://github.com/Neargye/scope_guard.git", commit = "fa60305b5805dcd872b3c60d0bc517c505f99502" }
)");

    ASSERT_EQ(meta.dependencies().size(), 1);

    const auto& dep = meta.dependencies()[0];
    ASSERT_TRUE(dep.is_git());

    const auto& desc = get<GitDep>(dep.desc);
    EXPECT_EQ(desc.git, "https://github.com/Neargye/scope_guard.git");
    EXPECT_EQ(desc.commit, "fa60305b5805dcd872b3c60d0bc517c505f99502");

    ASSERT_THROW(mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[dependencies]
scope_guard = { git = "https://github.com/Neargye/scope_guard.git" }
)"),
        Error);
}

TEST(manifest, ProfileDefault)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"
    )");

    ASSERT_TRUE(meta.default_profile().conditional_configs.empty());

    auto prof = meta.default_profile().config;
    ASSERT_TRUE(prof.cxxflags.empty());
    ASSERT_TRUE(prof.definitions.empty());

    meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[profile]
cxxflags = ["-Wall"]
definitions = ["A", "B"]
    )");

    prof = meta.default_profile().config;
    ASSERT_EQ(prof.cxxflags.size(), 1);
    ASSERT_EQ(prof.cxxflags[0], "-Wall");

    ranges::sort(prof.definitions);
    ASSERT_EQ(prof.definitions.size(), 2);
    ASSERT_EQ(prof.definitions[0], "A");
    ASSERT_EQ(prof.definitions[1], "B");
}

TEST(manifest, ProfileDebug)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"
    )");

    auto prof = meta.profile(Profile::debug).config;
    ASSERT_TRUE(prof.cxxflags.empty());
    ASSERT_TRUE(prof.definitions.empty());

    meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[profile.debug]
cxxflags = ["-Wall", "-Wextra"]
definitions = ["A", "B"]
    )");

    prof = meta.profile(Profile::debug).config;
    ASSERT_EQ(prof.cxxflags.size(), 2);
    ASSERT_EQ(prof.cxxflags[0], "-Wall");
    ASSERT_EQ(prof.cxxflags[1], "-Wextra");

    ranges::sort(prof.definitions);
    ASSERT_EQ(prof.definitions.size(), 2);
    ASSERT_EQ(prof.definitions[0], "A");
    ASSERT_EQ(prof.definitions[1], "B");
}

TEST(manifest, ProfileRelease)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"
    )");

    auto prof = meta.profile(Profile::release).config;
    ASSERT_TRUE(prof.cxxflags.empty());
    ASSERT_TRUE(prof.definitions.empty());

    meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[profile.release]
cxxflags = ["-Wall"]
definitions = ["A", "B"]
    )");

    prof = meta.profile(Profile::release).config;
    ASSERT_EQ(prof.cxxflags.size(), 1);
    ASSERT_EQ(prof.cxxflags[0], "-Wall");

    ranges::sort(prof.definitions);
    ASSERT_EQ(prof.definitions.size(), 2);
    ASSERT_EQ(prof.definitions[0], "A");
    ASSERT_EQ(prof.definitions[1], "B");

    for (const auto& prof : { meta.default_profile().config, meta.profile(Profile::debug).config }) {
        ASSERT_TRUE(prof.cxxflags.empty());
        ASSERT_TRUE(prof.definitions.empty());
    }
}

TEST(manifest, ProfileCfg)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[target.'cfg(not(compiler = "msvc"))'.profile]
cxxflags = ["-Wall"]

[target.'cfg(compiler = "msvc")'.profile]
cxxflags = ["/MP"]
definitions = ["A"]
    )");

    const auto& prof = meta.default_profile();
    ASSERT_TRUE(prof.config.cxxflags.empty());
    ASSERT_TRUE(prof.config.definitions.empty());

    const auto& condition_configs = prof.conditional_configs;
    ASSERT_EQ(condition_configs.size(), 2);
    ASSERT_TRUE(meta.profile(Profile::debug).conditional_configs.empty());
    ASSERT_TRUE(meta.profile(Profile::release).conditional_configs.empty());

    int seen = 0;
    for (const auto& [condition, config] : condition_configs) {
        if (condition == cfg::Compiler::msvc) {
            ASSERT_EQ(config.cxxflags.size(), 1);
            EXPECT_EQ(config.cxxflags[0], "/MP");
            ASSERT_EQ(config.definitions.size(), 1);
            EXPECT_EQ(config.definitions[0], "A");

            ++seen;
        } else if (condition == CfgNot { cfg::Compiler::msvc }) {
            ASSERT_EQ(config.cxxflags.size(), 1);
            EXPECT_EQ(config.cxxflags[0], "-Wall");
            EXPECT_EQ(config.definitions.size(), 0);

            ++seen;
        } else {
            FAIL() << "unexpected condition";
        }
    }

    ASSERT_EQ(seen, 2);
}

TEST(manifest, ProfileCfgDebug)
{
    auto meta = mock_manifest(R"([package]
name = "abc"
version = "0.1.0"

[target.'cfg(compiler = "msvc")'.profile.debug]
cxxflags = ["-Wall"]

[target.'cfg(compiler = "msvc")'.profile]
cxxflags = ["/MP"]
definitions = ["A"]
    )");

    const auto& prof = meta.default_profile();
    ASSERT_TRUE(prof.config.cxxflags.empty());
    ASSERT_TRUE(prof.config.definitions.empty());

    const auto& condition_configs = prof.conditional_configs;
    ASSERT_EQ(condition_configs.size(), 1);
    ASSERT_EQ(condition_configs[0].condition, cfg::Compiler::msvc);

    ASSERT_EQ(meta.profile(Profile::debug).conditional_configs.size(), 1);
    ASSERT_EQ(meta.profile(Profile::debug).conditional_configs[0].condition, cfg::Compiler::msvc);

    ASSERT_TRUE(meta.profile(Profile::release).conditional_configs.empty());
}
