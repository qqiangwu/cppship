#include "cppship/core/workspace.h"

#include <functional>
#include <tuple>
#include <utility>

#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/transform.hpp>

#include "cppship/core/manifest.h"

using namespace ranges::views;

namespace cppship {

namespace {

template <class Layouts, class Proj>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
void check_duplicate_target(std::string_view kind, const Layouts& layouts, Proj&& proj)
{
    std::map<std::string, std::string> target2package;

    for (const auto& layout : layouts) {
        for (const auto& target : std::invoke(proj, layout)) {
            auto [it, inserted] = target2package.emplace(target.name, layout.package());
            if (!inserted) {
                throw Error { fmt::format(
                    "duplicate {} {} from package {} and {}", kind, target.name, it->second, layout.package()) };
            }
        }
    }
}

}

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

    check_duplicate_target("binary", layouts(), &Layout::binaries);
}

std::set<fs::path> Workspace::list_files() const
{
    return join(values(packages_) | transform([](const auto& layout) { return layout.all_files(); }))
        | ranges::to<std::set>();
}

const Layout* Workspace::layout(std::string_view package) const
{
    const auto l = layouts();
    auto it = ranges::find_if(l, [package](const Layout& layout) { return layout.package() == package; });

    return it == l.end() ? nullptr : &*it;
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
