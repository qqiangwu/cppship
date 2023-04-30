#include "cppship/util/git.h"
#include "cppship/exception.h"
#include "cppship/util/cmd.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"

#include <fmt/core.h>

using namespace cppship;

void util::git_clone(std::string_view package, const fs::path& deps_dir, std::string_view git, std::string_view commit)
{
    const fs::path package_dep_dir = deps_dir / package;
    fs::remove_all(package_dep_dir);

    int res = run_cmd(fmt::format("git clone {} {}", git, package_dep_dir.string()));
    if (res != 0) {
        throw Error { fmt::format("install {} from {} failed", package, git) };
    }

    res = run_cmd(fmt::format("git -C {} reset --hard {}", package_dep_dir.string(), commit));
    if (res != 0) {
        throw Error { fmt::format("commit {} not found for {}", commit, package) };
    }
}