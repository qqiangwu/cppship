#include <cstdlib>

#include "cppship/cmd/clean.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"
#include "cppship/util/repo.h"

using namespace cppship;

int cmd::run_clean(const CleanOptions&)
{
    status("clean", "clean builds");
    fs::remove_all(get_project_root() / kBuildPath);
    return EXIT_SUCCESS;
}