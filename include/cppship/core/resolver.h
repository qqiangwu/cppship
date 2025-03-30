#pragma once

#include <functional>
#include <queue>
#include <set>
#include <vector>

#include <gsl/pointers>

#include "cppship/core/dependency.h"
#include "cppship/core/manifest.h"

namespace cppship {

struct ResolveResult {
    std::vector<DeclaredDependency> conan_dependencies;
    std::vector<DeclaredDependency> conan_dev_dependencies;

    ResolvedDependencies resolved_dependencies;

    std::vector<DeclaredDependency> dependencies;
    std::vector<DeclaredDependency> dev_dependencies;
};

// void(std::string_view package, const fs::path& dep_dir, std::string_view git, std::string_view commit)
using GitFetcher = std::function<void(std::string_view, const fs::path&, std::string_view, std::string_view)>;

// resolve git deps and git-clone it to cmake deps dir
class Resolver {
public:
    Resolver(const fs::path& deps_dir, const Manifest& manifest, GitFetcher fetcher)
        : Resolver(deps_dir, manifest.dependencies(), manifest.dev_dependencies(), std::move(fetcher))
    {
    }

    Resolver(const fs::path& deps_dir, const PackageManifest& manifest, GitFetcher fetcher)
        : Resolver(deps_dir, manifest.dependencies(), manifest.dev_dependencies(), std::move(fetcher))
    {
    }

    ResolveResult resolve() &&;

private:
    Resolver(const fs::path& deps_dir, const std::vector<DeclaredDependency>& deps,
        const std::vector<DeclaredDependency>& dev_deps, GitFetcher fetcher);

    void do_resolve_(const DeclaredDependency& dep);

    void resolve_package_(std::string_view package, const fs::path& package_dir);

private:
    fs::path mDepsDir;
    GitFetcher mFetcher;
    ResolveResult mResult;
    std::queue<DeclaredDependency> mUnresolved;
    std::set<std::string> mPackageSeen;
};

}
