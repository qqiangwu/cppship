#pragma once

#include <filesystem>
#include <string_view>

namespace fs = std::filesystem;

namespace cppship {

class ScopedCurrentDir {
public:
    explicit ScopedCurrentDir(const fs::path& cwd)
        : mPrevCwd(fs::current_path())
    {
        fs::current_path(cwd);
    }

    ~ScopedCurrentDir() { fs::current_path(mPrevCwd); }

    ScopedCurrentDir(const ScopedCurrentDir&) = delete;
    ScopedCurrentDir(ScopedCurrentDir&&) = delete;

    ScopedCurrentDir& operator=(const ScopedCurrentDir&) = delete;
    ScopedCurrentDir& operator=(ScopedCurrentDir&&) = delete;

private:
    fs::path mPrevCwd;
};

inline void create_if_not_exist(const fs::path& path)
{
    if (fs::exists(path)) {
        return;
    }

    fs::create_directory(path);
}

}