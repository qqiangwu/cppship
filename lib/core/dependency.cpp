#include "cppship/core/dependency.h"
#include "cppship/exception.h"
#include "cppship/util/string.h"

#include <boost/algorithm/string.hpp>

using namespace cppship;

namespace {

inline std::string_view retrieve_package_name(const std::string_view target)
{
    auto pos = target.find("_LIBRARIES");
    if (pos == std::string_view::npos) {
        throw Error { fmt::format("cmake target file bad target {}", target) };
    }

    return target.substr(0, pos);
}

constexpr int kFieldsInComponentLine = 5;
constexpr int kFieldsInTargetLine = 3;

}

Dependency cppship::parse_dep(std::string_view cmake_package, const fs::path& target_file)
{
    Dependency dep;
    dep.cmake_package = cmake_package;

    std::ifstream ofs(target_file);
    if (!ofs) {
        throw Error { fmt::format("cmake target file {} not found", target_file.string()) };
    }

    std::string line;
    while (std::getline(ofs, line)) {
        if (boost::contains(line, "AGGREGATED GLOBAL TARGET WITH THE COMPONENTS")) {
            while (std::getline(ofs, line)) {
                boost::trim(line);
                const auto fields = util::split(line, boost::is_space());

                // set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::disable_autolinking APPEND)
                if (fields.size() < kFieldsInComponentLine) {
                    break;
                }

                dep.components.emplace_back(fields[4]);
            }
        }

        if (boost::contains(line, "FindXXX")) {
            std::getline(ofs, line);
            if (!line.starts_with("set")) {
                throw Error { fmt::format("cmake target file bad target line: {}", line) };
            }

            // set(boost_LIBRARIES_DEBUG boost::boost)
            const std::vector<std::string> fields = util::split(line, boost::is_any_of("() "));
            if (fields.size() < kFieldsInTargetLine) {
                throw Error { fmt::format("cmake target file bad target line: {}", line) };
            }

            dep.package = retrieve_package_name(fields[1]);
            dep.cmake_target = fields[2];
        }
    }

    if (dep.package.empty()) {
        throw Error { fmt::format("parse cmake target file {} failed", target_file.string()) };
    }

    return dep;
}

ResolvedDependencies cppship::collect_conan_deps(const fs::path& conan_dep_dir, std::string_view profile)
{
    std::map<std::string, Dependency> deps;

    for (const auto& dentry : fs::directory_iterator { conan_dep_dir }) {
        const std::string filename = dentry.path().filename().string();
        const std::string suffix = fmt::format("-Target-{}.cmake", boost::to_lower_copy(std::string { profile }));
        if (!filename.ends_with(suffix)) {
            continue;
        }

        const std::string_view cmake_package = std::string_view { filename }.substr(0, filename.size() - suffix.size());
        try {
            auto dep = parse_dep(cmake_package, dentry.path());
            deps.emplace(dep.package, std::move(dep));
        } catch (std::exception& e) {
            throw Error { fmt::format("invalid cmake target file {}: {}", filename, e.what()) };
        }
    }

    return deps;
}