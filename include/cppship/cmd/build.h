#pragma once

#include <map>
#include <string>
#include <vector>

#include <toml.hpp>

#include "cppship/util/fs.h"

namespace cppship::cmd {

struct BuildOptions {
    int max_concurrency = 0;
};

struct ResolvedDeps {
    std::vector<std::string> includes;
    std::vector<std::string> defines;
    std::vector<std::string> lib_dirs;
    std::vector<std::string> libs;
};

struct Manifest {
    fs::path root;

    struct Dependency {
        std::string version;
    };
    std::map<std::string, Dependency> dependencies;
};

namespace detail {

    bool lock_file_expired(const Manifest& manifest, const fs::path& lockfile);

    void lock_dependencies(const fs::path& lockfile, const ResolvedDeps& deps);

    ResolvedDeps load_cached_dependencies(const fs::path& lockfile);

    void install_conan_generator(const Manifest& manifest);

    std::string generate_conan_requires(const Manifest& manifest);

    void generate_conan_file(const Manifest& manifest);

    ResolvedDeps install_conan_packages(const Manifest& manifest);

}

Manifest load_manifest(const fs::path& manifest_file);

ResolvedDeps resolve_dependencies(const Manifest& manifest);

void build(const ResolvedDeps& dependencies);

int run_build(const BuildOptions& options);

}

// NOLINTNEXTLINE(readability-identifier-length): make toml happy
TOML11_DEFINE_CONVERSION_NON_INTRUSIVE(cppship::cmd::ResolvedDeps, includes, defines, lib_dirs, libs);
