#include "cppship/util/fs.h"
#include <cstdlib>

void cppship::create_if_not_exist(const fs::path& path)
{
    if (fs::exists(path)) {
        return;
    }

    if (path.has_parent_path()) {
        create_if_not_exist(path.parent_path());
    }

    fs::create_directory(path);
}

fs::path cppship::get_cppship_dir() { return fs::path(std::getenv("HOME")) / kCppShipDirName; }
