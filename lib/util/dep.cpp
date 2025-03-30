#include "cppship/util/dep.h"

#include <range/v3/action/push_back.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/to_container.hpp>

#include "cppship/core/dependency.h"
#include "cppship/exception.h"

namespace cppship {

void merge_to(const std::vector<DeclaredDependency>& new_deps, std::vector<DeclaredDependency>& full_deps)
{
    std::vector<DeclaredDependency> deduped_deps;

    for (const auto& dep : new_deps) {
        auto it = ranges::find_if(full_deps, [&dep](const auto& d) { return dep.package == d.package; });
        if (it != full_deps.end()) {
            if (!is_compatible(*it, dep)) {
                throw Error { fmt::format("package {} have incompatible version or options", dep.package) };
            }
            continue;
        }

        deduped_deps.push_back(dep);
    }

    ranges::push_back(full_deps, deduped_deps);
}

bool is_compatible(const DeclaredDependency& d1, const DeclaredDependency& d2)
{
    if (d1.desc.index() != d2.desc.index()) {
        return false;
    }

    if (const auto* conan_dep = get_if<ConanDep>(&d1.desc)) {
        const auto& conan_dep2 = get<ConanDep>(d2.desc);
        return conan_dep->version == conan_dep2.version
            && ranges::to<std::map<std::string, std::string>>(conan_dep->options)
            == ranges::to<std::map<std::string, std::string>>(conan_dep2.options);
    }

    const auto& git1 = get<GitDep>(d1.desc);
    const auto& git2 = get<GitDep>(d2.desc);
    return git1.git == git2.git && git1.commit == git2.commit;
}

}
