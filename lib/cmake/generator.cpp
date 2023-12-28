#include "cppship/cmake/generator.h"
#include "cppship/cmake/bin.h"
#include "cppship/cmake/cfg_predicate.h"
#include "cppship/cmake/dep.h"
#include "cppship/cmake/group.h"
#include "cppship/cmake/lib.h"
#include "cppship/cmake/naming.h"
#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/log.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <gsl/pointers>
#include <range/v3/action/push_back.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/transform.hpp>

using namespace ranges::views;

using namespace cppship;
using namespace cppship::cmake;
using namespace fmt::literals;

using boost::join;

std::vector<cmake::Dep> cmake::collect_cmake_deps(
    const std::vector<DeclaredDependency>& declared_deps, const ResolvedDependencies& deps)
{
    std::vector<cmake::Dep> result;

    for (const auto& dep : declared_deps) {
        const auto& resolved_dep = deps.at(dep.package);

        if (dep.components.empty()) {
            result.push_back({
                .cmake_package = resolved_dep.cmake_package,
                .cmake_targets = { resolved_dep.cmake_target },
            });

            continue;
        }

        std::set<std::string> resolved_components(resolved_dep.components.begin(), resolved_dep.components.end());
        std::vector<std::string> targets_required;

        for (const auto& declared_component : dep.components) {
            auto component = fmt::format("{}::{}", resolved_dep.cmake_package, declared_component);
            if (resolved_components.contains(component)) {
                targets_required.push_back(std::move(component));
                continue;
            }
            if (resolved_components.contains(declared_component)) {
                targets_required.push_back(declared_component);
                continue;
            }

            throw Error { fmt::format("invalid component {} in manifest", declared_component) };
        }

        result.push_back({
            .cmake_package = resolved_dep.cmake_package,
            .cmake_targets = targets_required,
        });
    }

    return result;
}

CmakeGenerator::CmakeGenerator(gsl::not_null<const Layout*> layout, const Manifest& manifest, GeneratorOptions options)
    : mLayout(layout)
    , mManifest(manifest)
    , mDeps(std::move(options.deps))
    , mDevDeps(std::move(options.dev_deps))
    , mInjector(std::move(options.injector))
{
}

std::string CmakeGenerator::build() &&
{
    emit_header_();
    emit_dependency_injector_();
    emit_package_finders_();

    add_lib_sources_();
    add_app_sources_();

    emit_dev_package_finders_();
    add_benches_();
    add_examples_();
    add_test_sources_();

    emit_footer_();

    return mOut.str();
}

void CmakeGenerator::emit_header_()
{
    mOut << "cmake_minimum_required(VERSION 3.17)\n"
         << fmt::format("project({} VERSION {})\n", mName, mManifest.version()) << R"(
include(CTest)
enable_testing()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

)";

    mOut << "# cpp options\n";
    fill_default_profile_();

    mOut << "\n# profile cpp options\n";
    fill_profile_(Profile::debug);
    fill_profile_(Profile::release);

    mOut << fmt::format(R"(
# cpp std
set(CMAKE_CXX_STANDARD {})
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)
)",
        mManifest.cxx_std());
}

void CmakeGenerator::emit_dependency_injector_()
{
    if (mInjector) {
        mInjector->inject(mOut, mManifest);
        return;
    }

    mOut << R"(
# add conan generator folder
list(PREPEND CMAKE_PREFIX_PATH "${CONAN_GENERATORS_FOLDER}")
list(PREPEND CMAKE_PREFIX_PATH "${CPPSHIP_DEPS_DIR}")
)";
}

void CmakeGenerator::emit_package_finders_()
{
    mOut << "\n# Package finders\n";

    for (const auto& dep : mDeps) {
        mOut << fmt::format("find_package({} REQUIRED)\n", dep.cmake_package);
    }

    if (!mDeps.empty()) {
        mOut << '\n';
    }

    mOut << fmt::format("add_library({}_deps INTERFACE)\n", mName);

    for (const auto& dep : mDeps) {
        mOut << fmt::format(
            "target_link_libraries({}_deps INTERFACE {})\n", mName, boost::join(dep.cmake_targets, " "));
    }

    mDeps = {
        {
            .cmake_package = "",
            .cmake_targets = { fmt::format("{}_deps", mName) },
        },
    };
}

void CmakeGenerator::add_lib_sources_()
{
    const auto target = mLayout->lib();
    if (!target) {
        return;
    }

    cmake::CmakeLib lib({
        .name = target->name,
        .name_alias = target->name,
        .include_dirs = target->includes,
        .sources = target->sources,
        .deps = mDeps,
    });
    lib.build(mOut);

    // view lib as a binary target to easy `cppship build` for header only lib
    mLib.emplace(lib.target());
    mBinaryTargets.emplace(*mLib);
}

void CmakeGenerator::add_app_sources_()
{
    if (mLayout->binaries().empty()) {
        return;
    }

    std::vector<std::string> definitions {
        fmt::format(R"({}_VERSION="${{PROJECT_VERSION}}")",
            boost::replace_all_copy(boost::to_upper_copy(std::string { mName }), "-", "_")),
    };

    for (const auto& bin : mLayout->binaries()) {
        cmake::CmakeBin gen({
            .name = bin.name,
            .sources = bin.sources,
            .lib = mLib,
            .deps = mDeps,
            .definitions = definitions,
            .need_install = true,
        });

        gen.build(mOut);
        mBinaryTargets.emplace(bin.name);
    }
}

void CmakeGenerator::emit_dev_package_finders_()
{
    mOut << "\n# Dev Package finders\n";

    for (const auto& dep : mDevDeps) {
        mOut << fmt::format("find_package({} REQUIRED)\n", dep.cmake_package);
    }

    if (!mDevDeps.empty()) {
        mOut << '\n';
    }

    mOut << fmt::format("add_library({}_dev_deps INTERFACE)\n", mName);

    for (const auto& dep : concat(mDeps, mDevDeps)) {
        mOut << fmt::format(
            "target_link_libraries({}_dev_deps INTERFACE {})\n", mName, boost::join(dep.cmake_targets, " "));
    }

    mDevDeps = {
        {
            .cmake_package = "",
            .cmake_targets = { fmt::format("{}_dev_deps", mName) },
        },
    };
}

void CmakeGenerator::add_benches_()
{
    const auto& benches = mLayout->benches();
    if (benches.empty()) {
        return;
    }

    mOut << R"(# BENCH
find_package(benchmark REQUIRED)
)";

    NameTargetMapper mapper;
    auto deps = mDevDeps;
    deps.push_back({
        .cmake_package = "benchmark",
        .cmake_targets = { "benchmark::benchmark" },
    });
    for (const auto& bin : benches) {
        const auto target = mapper.bench(bin.name);

        cmake::CmakeBin gen({
            .name = target,
            .sources = bin.sources,
            .lib = mLib,
            .deps = deps,
        });

        gen.build(mOut);
        mBenchTargets.emplace(target);
    }
}

void CmakeGenerator::add_examples_()
{
    const auto& examples = mLayout->examples();
    if (examples.empty()) {
        return;
    }

    NameTargetMapper mapper;
    for (const auto& bin : examples) {
        const auto target = mapper.example(bin.name);

        cmake::CmakeBin gen({
            .name = target,
            .name_alias = bin.name,
            .sources = bin.sources,
            .lib = mLib,
            .deps = mDevDeps,
            .runtime_dir = "examples",
        });

        gen.build(mOut);
        mExampleTargets.emplace(target);
    }
}

void CmakeGenerator::add_test_sources_()
{
    const auto& tests = mLayout->tests();
    if (tests.empty()) {
        return;
    }

    mOut << R"(
# Tests
find_package(GTest REQUIRED)
)";

    NameTargetMapper mapper;
    for (const auto& test : tests) {
        const auto target = mapper.test(test.name);

        mOut << '\n'
             << fmt::format("add_executable({} {})\n", target, test.sources.begin()->generic_string())
             << fmt::format("target_link_libraries({} PRIVATE GTest::gtest_main)\n", target);

        if (mLib) {
            mOut << fmt::format("target_link_libraries({} PRIVATE {})\n", target, *mLib);
        }
        for (const auto& dep : mDevDeps) {
            mOut << fmt::format("target_link_libraries({} PRIVATE {})\n", target, boost::join(dep.cmake_targets, " "));
        }

        mOut << fmt::format("add_test({} {})\n", target, target);

        mTestTargets.emplace(target);
    }
}

void CmakeGenerator::emit_footer_()
{
    mOut << "\n# Groups\n"
         << fmt::format(
                "add_custom_target({} DEPENDS {})\n", cmake::kCppshipGroupBinaries, boost::join(mBinaryTargets, " "))
         << fmt::format(
                "add_custom_target({} DEPENDS {})\n", cmake::kCppshipGroupExamples, boost::join(mExampleTargets, " "))
         << fmt::format(
                "add_custom_target({} DEPENDS {})\n", cmake::kCppshipGroupBenches, boost::join(mBenchTargets, " "))
         << fmt::format(
                "add_custom_target({} DEPENDS {})\n", cmake::kCppshipGroupTests, boost::join(mTestTargets, " "));

    mOut << "\n# Footer\n"
         << R"(set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
)";
}

namespace {

class ProfileOptionGen {
    std::ostream& mOut;

public:
    ProfileOptionGen(std::ostream& out)
        : mOut(out)
    {
    }

    void output(const std::string_view profile, const ProfileConfig& config, std::string_view indent = "")
    {
        for (const auto& opt : config.cxxflags) {
            mOut << fmt::format("{}add_compile_options($<$<CONFIG:{}>:{}>)\n", indent, profile, opt);
        }
        for (const auto& def : config.definitions) {
            mOut << fmt::format("{}add_compile_definitions($<$<CONFIG:{}>:{}>)\n", indent, profile, def);
        }

        if (config.ubsan.value_or(false)) {
            mOut << fmt::format("{}add_compile_options($<$<CONFIG:{}>:{}>)\n", indent, profile, "-fsanitize=undefined");
            mOut << fmt::format("{}add_link_options($<$<CONFIG:{}>:{}>)\n", indent, profile, "-fsanitize=undefined");
            mOut << fmt::format("{}message(STATUS \"Enable ubsan\")\n", indent);
        }

        if (config.tsan.value_or(false)) {
            mOut << fmt::format("{}add_compile_options($<$<CONFIG:{}>:{}>)\n", indent, profile, "-fsanitize=thread");
            mOut << fmt::format("{}add_link_options($<$<CONFIG:{}>:{}>)\n", indent, profile, "-fsanitize=thread");
            mOut << fmt::format("{}message(STATUS \"Enable tsan\")\n", indent);
        }

        if (config.asan.value_or(false)) {
            mOut << fmt::format("{}add_compile_options($<$<CONFIG:{}>:{}>)\n", indent, profile, "-fsanitize=address");
            mOut << fmt::format("{}add_link_options($<$<CONFIG:{}>:{}>)\n", indent, profile, "-fsanitize=address");
            mOut << fmt::format("{}message(STATUS \"Enable asan\")\n", indent);
        }

        if (config.leak.value_or(false)) {
            mOut << fmt::format("{}add_compile_options($<$<CONFIG:{}>:{}>)\n", indent, profile, "-fsanitize=leak");
            mOut << fmt::format("{}add_link_options($<$<CONFIG:{}>:{}>)\n", indent, profile, "-fsanitize=leak");
            mOut << fmt::format("{}message(STATUS \"Enable leak\")\n", indent);
        }
    }

    void output(const ProfileConfig& config, std::string_view indent = "")
    {
        for (const auto& opt : config.cxxflags) {
            mOut << fmt::format("{}add_compile_options({})\n", indent, opt);
        }
        for (const auto& def : config.definitions) {
            mOut << fmt::format("{}add_compile_definitions({})\n", indent, def);
        }

        if (config.ubsan.value_or(false)) {
            mOut << fmt::format("{}add_compile_options({})\n", indent, "-fsanitize=undefined");
            mOut << fmt::format("{}add_link_options({})\n", indent, "-fsanitize=undefined");
            mOut << fmt::format("{}message(STATUS \"Enable ubsan\")\n", indent);
        }

        if (config.tsan.value_or(false)) {
            mOut << fmt::format("{}add_compile_options({})\n", indent, "-fsanitize=thread");
            mOut << fmt::format("{}add_link_options({})\n", indent, "-fsanitize=thread");
            mOut << fmt::format("{}message(STATUS \"Enable tsan\")\n", indent);
        }

        if (config.asan.value_or(false)) {
            mOut << fmt::format("{}add_compile_options({})\n", indent, "-fsanitize=address");
            mOut << fmt::format("{}add_link_options({})\n", indent, "-fsanitize=address");
            mOut << fmt::format("{}message(STATUS \"Enable asan\")\n", indent);
        }

        if (config.leak.value_or(false)) {
            mOut << fmt::format("{}add_compile_options({})\n", indent, "-fsanitize=leak");
            mOut << fmt::format("{}add_link_options({})\n", indent, "-fsanitize=leak");
            mOut << fmt::format("{}message(STATUS \"Enable leak\")\n", indent);
        }
    }
};

}

void CmakeGenerator::fill_default_profile_()
{
    const auto& default_profile = mManifest.default_profile();

    ProfileOptionGen appender(mOut);
    appender.output(default_profile.config);

    for (const auto& [condition, config] : default_profile.conditional_configs) {
        mOut << fmt::format("if({})\n", generate_predicate(condition));
        appender.output(config, "\t");
        mOut << "endif()\n\n";
    }
}

void CmakeGenerator::fill_profile_(Profile profile)
{
    const auto& options = mManifest.profile(profile);
    const auto& profile_str = to_string(profile);

    ProfileOptionGen appender(mOut);
    appender.output(profile_str, options.config);

    for (const auto& [condition, config] : options.conditional_configs) {
        mOut << fmt::format("if({})\n", generate_predicate(condition));
        appender.output(profile_str, config, "\t");
        mOut << "endif()\n\n";
    }
}