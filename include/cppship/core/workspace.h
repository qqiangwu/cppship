#pragma once

#include <range/v3/view/map.hpp>

#include "cppship/core/layout.h"
#include "cppship/core/manifest.h"

namespace cppship {

// manage the layout of all packages.
// for simple package, assume it's a workspace with one package.
class Workspace {
public:
    Workspace(const fs::path& project_root, const Manifest& manifest);

    const Layout* get_default() const
    {
        auto it = packages_.find({});
        return it == packages_.end() ? nullptr : &it->second;
    }

    const Layout& as_package() const { return packages_.at({}); }

    std::set<fs::path> list_files() const;

    auto begin() const { return packages_.begin(); }

    auto end() const { return packages_.end(); }

    auto layouts() const { return ranges::views::values(packages_); }

    const Layout* layout(std::string_view package) const;

private:
    fs::path root_;

    // if there is only one package, the key is fs::path{}
    std::map<fs::path, Layout> packages_;
};

const Layout& enforce_default_package(const Workspace& workspace);

}
