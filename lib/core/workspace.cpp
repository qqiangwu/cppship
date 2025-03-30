#include "cppship/core/workspace.h"

#include <tuple>
#include <utility>

#include <range/v3/algorithm/transform.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/transform.hpp>

#include "cppship/core/manifest.h"

using namespace ranges::views;

namespace cppship {

Workspace::Workspace(const fs::path& project_root, const Manifest& manifest)
    : root_(project_root)
{
    if (const auto* p = manifest.get_if_package()) {
        packages_.emplace(
            std::piecewise_construct, std::forward_as_tuple(fs::path {}), std::forward_as_tuple(root_, p->name()));
        return;
    }

    for (const auto& [path, package_manifest] : manifest.list_packages()) {
        packages_.emplace(std::piecewise_construct,
            std::forward_as_tuple(path),
            std::forward_as_tuple(root_ / path, package_manifest.name()));
    }
}

std::set<fs::path> Workspace::list_files() const
{
    return join(values(packages_) | transform([](const auto& layout) { return layout.all_files(); }))
        | ranges::to<std::set>();
}

const Layout& enforce_default_package(const Workspace& workspace)
{
    const auto* l = workspace.get_default();
    if (l == nullptr) {
        throw Error { "workspace is not supported for this command" };
    }

    return *l;
}

}
