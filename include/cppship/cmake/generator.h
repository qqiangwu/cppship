#pragma once

#include <memory>
#include <set>
#include <sstream>
#include <string>

#include <gsl/pointers>

#include "cppship/cmake/dep.h"
#include "cppship/cmake/dependency_injector.h"
#include "cppship/core/dependency.h"
#include "cppship/core/layout.h"
#include "cppship/core/manifest.h"

namespace cppship {

struct GeneratorOptions {
    std::vector<cmake::Dep> deps;
    std::vector<cmake::Dep> dev_deps;
    std::unique_ptr<cmake::DependencyInjector> injector;
};

class CmakeGenerator {
public:
    CmakeGenerator(gsl::not_null<const Layout*> layout, gsl::not_null<const PackageManifest*> manifest,
        GeneratorOptions options = {});

    std::string build() &&;

private:
    void emit_header_();

    void emit_dependency_injector_();

    void emit_package_finders_();

    void add_lib_sources_();

    void add_app_sources_();

    void emit_dev_package_finders_();

    void add_examples_();

    void add_benches_();

    void add_test_sources_();

    void emit_footer_();

private:
    void fill_default_profile_();
    void fill_profile_(Profile profile);

private:
    std::ostringstream mOut;

    gsl::not_null<const Layout*> mLayout;
    gsl::not_null<const PackageManifest*> mManifest;

    std::vector<cmake::Dep> mDeps;
    std::vector<cmake::Dep> mDevDeps;

    std::string_view mName = mManifest->name();

    std::unique_ptr<cmake::DependencyInjector> mInjector;

    std::optional<std::string> mLib;
    std::set<std::string> mBinaryTargets;
    std::set<std::string> mExampleTargets;
    std::set<std::string> mBenchTargets;
    std::set<std::string> mTestTargets;
};

// generate cmake config for single package project
class SimpleGenerator {
public:
    SimpleGenerator(gsl::not_null<const Layout*> layout, gsl::not_null<const PackageManifest*> manifest,
        GeneratorOptions options = {})
        : mLayout(layout)
        , mManifest(manifest)
        , mOptions(std::move(options))
    {
    }

    std::string build() &&;

private:
    gsl::not_null<const Layout*> mLayout;
    gsl::not_null<const PackageManifest*> mManifest;
    GeneratorOptions mOptions;
};

class WorkspaceGenerator {
public:
    WorkspaceGenerator(
        const fs::path& deps_dir, std::function<std::string(std::string_view, std::string_view)> package_handler);

    void add(const Layout& layout, const PackageManifest& manifest, const ResolvedDependencies& resolved_deps);

    std::string build() &&;

private:
    fs::path mDepsDir;
    std::function<std::string(std::string_view, std::string)> mPackageHandler;

    std::ostringstream mOut;
    std::vector<std::string> mPackagesAdded;
};

}
