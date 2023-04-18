#include "cppship/cmake/lib.h"
#include "cppship/util/repo.h"

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

using namespace cppship;
using namespace cppship::cmake;
using namespace ranges::views;

CmakeLib::CmakeLib(LibDesc desc)
    : mName(std::move(desc.name))
    , mIncludeDir(std::move(desc.include_dir))
    , mSourceDir(std::move(desc.source_dir))
    , mDeps(desc.deps)
    , mDefinitions(std::move(desc.definitions))
{
    if (mSourceDir) {
        auto sources = list_sources(mSourceDir.value());
        mSources = sources | transform([](const auto& path) { return path.string(); })
            | ranges::to<std::vector<std::string>>();
    }
}

void CmakeLib::build(std::ostream& out) const
{
    out << "\n# LIB\n";
    if (is_interface()) {
        out << fmt::format("add_library({} INTERFACE)\n", mName);
    } else {
        out << fmt::format("add_library({} {})\n", mName, boost::join(mSources, "\n"));
    }

    const std::string_view lib_type = is_interface() ? "INTERFACE" : "PUBLIC";
    out << "\n"
        << fmt::format("target_include_directories({} {} ${{CMAKE_SOURCE_DIR}}/{})\n", mName, lib_type, kLibPath)
        << fmt::format("target_include_directories({} {} ${{CMAKE_SOURCE_DIR}}/{})\n", mName, lib_type, kIncludePath)
        << std::endl;

    for (const auto& dep : mDeps) {
        out << fmt::format("target_link_libraries({} {} {})\n", mName, lib_type, boost::join(dep.cmake_targets, " "));
    }

    if (const auto& defs = mDefinitions; !defs.empty()) {
        out << fmt::format("\ntarget_compile_definitions({} {} {})\n", mName, lib_type, boost::join(defs, " "));
    }
}