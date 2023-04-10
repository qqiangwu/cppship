#pragma once

#include <set>
#include <sstream>
#include <string>

#include "cppship/core/dependency.h"
#include "cppship/core/manifest.h"

namespace cppship {

class CmakeGenerator {
public:
    CmakeGenerator(const Manifest& manifest, const ResolvedDependencies& deps);

    std::string build() &&;

private:
    void emit_header_();

    void emit_package_finders_();

    void add_lib_sources_();

    void add_app_sources_();

    void add_bin_sources_();

    void add_test_sources_();

    void emit_footer_();

private:
    std::set<std::string> list_sources_(std::string_view dir);

private:
    std::ostringstream mOut;

    Manifest mManifest;
    ResolvedDependencies mDeps;

    std::string_view mName = mManifest.name();

    bool mHasLib = false;
};

}