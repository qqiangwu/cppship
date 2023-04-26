#pragma once

#include <set>
#include <sstream>
#include <string>

#include <gsl/pointers>

#include "cppship/cmake/dep.h"
#include "cppship/core/dependency.h"
#include "cppship/core/layout.h"
#include "cppship/core/manifest.h"

namespace cppship {

class CmakeGenerator {
public:
    CmakeGenerator(gsl::not_null<const Layout*> layout, const Manifest& manifest, const ResolvedDependencies& deps);

    std::string build() &&;

private:
    void emit_header_();

    void emit_package_finders_();

    void add_lib_sources_();

    void add_app_sources_();

    void emit_dev_package_finders_();

    void add_examples_();

    void add_benches_();

    void add_test_sources_();

    void emit_footer_();

private:
    std::set<std::string> list_sources_(std::string_view dir);

private:
    std::ostringstream mOut;

    gsl::not_null<const Layout*> mLayout;

    Manifest mManifest;
    std::vector<cmake::Dep> mDeps;
    std::vector<cmake::Dep> mDevDeps;

    // mDeps + mDevDeps
    std::vector<cmake::Dep> mDeps4Dev;

    std::string_view mName = mManifest.name();

    std::optional<std::string> mLib;
    std::set<std::string> mBinaryTargets;
    std::set<std::string> mExampleTargets;
    std::set<std::string> mBenchTargets;
    std::set<std::string> mTestTargets;
};

}