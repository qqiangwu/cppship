#pragma once

#include <filesystem> // IWYU pragma: export

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

void create_if_not_exist(const fs::path& path);

inline constexpr std::string_view kCppShipDirName = ".cppship";
fs::path get_cppship_dir();

}
