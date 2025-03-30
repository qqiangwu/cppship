#include "cppship/util/fs.h"
#include <cstdlib>
#include <filesystem>

void cppship::create_if_not_exist(const fs::path& path)
{
    if (fs::exists(path)) {
        return;
    }
    fs::create_directories(path);
}

fs::path cppship::get_cppship_dir() { return fs::path(std::getenv("HOME")) / kCppShipDirName; }
