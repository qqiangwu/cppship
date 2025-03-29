#pragma once

#include <ostream>

#include "cppship/core/dependency.h"
#include "cppship/core/manifest.h"
#include "cppship/util/class.h"

namespace cppship::cmake {

class DependencyInjector {
public:
    virtual ~DependencyInjector() = default;

    DECLARE_UNCOPYABLE(DependencyInjector);

    virtual void inject(std::ostream& out, const Manifest& manifest) = 0;
};

// when generate cmake project according to cppship.toml, we need config conan and cppship deps, do the config in this
// injector
class CmakeDependencyInjector : public DependencyInjector {
public:
    CmakeDependencyInjector(const fs::path& deps_dir, const std::vector<DeclaredDependency>& declared_deps,
        const ResolvedDependencies& cppship_deps, const ResolvedDependencies& all_deps)
        : mDepsDir(deps_dir)
        , mDeclaredDeps(declared_deps)
        , mCppshipDeps(cppship_deps)
        , mAllDeps(all_deps)
    {
    }

    void inject(std::ostream& out, const Manifest& manifest) override;

private:
    fs::path mDepsDir;
    std::vector<DeclaredDependency> mDeclaredDeps;
    ResolvedDependencies mCppshipDeps;
    ResolvedDependencies mAllDeps;
};

}