#include "cppship/core/resolver.h"

#include <fmt/format.h>
#include <range/v3/action/push_back.hpp>
#include <range/v3/action/reverse.hpp>

#include "cppship/core/manifest.h"
#include "cppship/util/io.h"
#include "cppship/util/log.h"
#include "cppship/util/repo.h"

using namespace cppship;
using namespace fmt::literals;

Resolver::Resolver(const fs::path& deps_dir, const std::vector<DeclaredDependency>& deps,
    const std::vector<DeclaredDependency>& dev_deps, GitFetcher fetcher)
    : mDepsDir(deps_dir)
    , mFetcher(std::move(fetcher))
{
    for (const auto& dep : deps) {
        const auto* conanlib = get_if<ConanDep>(&dep.desc);
        if (conanlib == nullptr) {
            mUnresolved.push(dep);
            continue;
        }

        mPackageSeen.insert(dep.package);
        mResult.conan_dependencies.push_back(dep);
    }

    for (const auto& dep : dev_deps) {
        const auto* conanlib = get_if<ConanDep>(&dep.desc);
        if (conanlib == nullptr) {
            mUnresolved.push(dep);
            continue;
        }

        mResult.conan_dev_dependencies.push_back(dep);
    }

    if (!mUnresolved.empty()) {
        create_if_not_exist(mDepsDir);
    }
}

cppship::ResolveResult Resolver::resolve() &&
{
    while (!mUnresolved.empty()) {
        const auto dep = std::move(mUnresolved.front());
        mUnresolved.pop();

        if (const bool existed = !mPackageSeen.insert(dep.package).second; existed) {
            status("resolve", "package {} already seen, skip", dep.package);
            continue;
        }

        do_resolve_(dep);
    }

    ranges::push_back(mResult.dependencies, mResult.conan_dependencies);
    ranges::reverse(mResult.dependencies);

    // TODO(qqiangwu): fix me
    ranges::push_back(mResult.dev_dependencies, mResult.conan_dev_dependencies);
    return std::move(mResult);
}

namespace {

void verify_git_dependency(const std::string_view package, const fs::path& dep_dir)
{
    if (fs::exists(dep_dir / kRepoConfigFile)) {
        return;
    }

    if (!fs::exists(dep_dir / kIncludePath)) {
        throw Error { fmt::format("git dependency {} has no include/", package) };
    }

    if (fs::exists(dep_dir / kLibPath) || fs::exists(dep_dir / kSrcPath)) {
        throw Error { fmt::format(
            "git dependency {} has {}/ or {}/ for header only lib", package, kLibPath, kSrcPath) };
    }
}

}

void Resolver::do_resolve_(const DeclaredDependency& dep)
{
    const auto& desc = get<GitDep>(dep.desc);
    const auto package_dir = mDepsDir / dep.package;
    const auto footprint = fmt::format("{}/cppship.{}", package_dir.string(), desc.commit);
    if (!fs::exists(footprint) && mFetcher) {
        status("resolve", "fetch {} from {}::{}", dep.package, desc.git, desc.commit);

        mFetcher(dep.package, mDepsDir, desc.git, desc.commit);
        verify_git_dependency(dep.package, package_dir);
    }

    resolve_package_(dep.package, package_dir);

    mResult.dependencies.push_back(dep);
    mResult.resolved_dependencies.insert(Dependency {
        .package = dep.package,
        .cmake_package = dep.package,
        .cmake_target = fmt::format("cppship::{}", dep.package),
    });

    touch(footprint);
}

void Resolver::resolve_package_(std::string_view package, const fs::path& package_dir)
{
    const auto cppship_manifest = package_dir / kRepoConfigFile;
    if (!fs::exists(cppship_manifest)) {
        status("resolve", "header only lib {} found", package);

        return;
    }

    status("resolve", "cppship lib {} found", package);

    Manifest manifest(cppship_manifest);
    if (manifest.is_workspace()) {
        throw Error { fmt::format("package {} is a workspace", package) };
    }

    for (const auto& sub_dep : manifest.dependencies()) {
        if (mPackageSeen.contains(sub_dep.package)) {
            status("resolve", "package {} already seen, skip", sub_dep.package);
            continue;
        }

        if (sub_dep.is_conan()) {
            mPackageSeen.insert(sub_dep.package);
            mResult.conan_dependencies.push_back(sub_dep);
            continue;
        }

        mUnresolved.push(sub_dep);
    }
}
