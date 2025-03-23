#include "cppship/util/fs.h"

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
