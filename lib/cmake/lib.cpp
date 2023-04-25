#include "cppship/cmake/lib.h"

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>

using namespace cppship;
using namespace cppship::cmake;
using namespace ranges::views;

namespace {

std::set<std::string> to_strings(const std::set<fs::path>& paths)
{
    return paths | transform([](const fs::path& path) { return path.string(); }) | ranges::to<std::set<std::string>>();
}

}

CmakeLib::CmakeLib(LibDesc desc)
    : mName(std::move(desc.name))
    , mIncludes(to_strings(desc.include_dirs))
    , mSources(to_strings(desc.sources))
    , mDeps(desc.deps)
    , mDefinitions(std::move(desc.definitions))
{
}

void CmakeLib::build(std::ostream& out) const
{
    const auto lib_name = target();

    out << "\n# LIB\n";
    if (is_interface()) {
        out << fmt::format("add_library({} INTERFACE)\n", lib_name);
    } else {
        out << fmt::format("add_library({} {})\n", lib_name, boost::join(mSources, "\n"));
    }
    out << fmt::format(R"(set_target_properties({} PROPERTIES OUTPUT_NAME "{}"))", lib_name, mName) << '\n';

    const std::string_view lib_type = is_interface() ? "INTERFACE" : "PUBLIC";
    if (!mIncludes.empty()) {
        out << "\n";

        for (const auto& dir : mIncludes) {
            out << fmt::format("target_include_directories({} {} {})\n", lib_name, lib_type, dir);
        }

        out << "\n";
    }

    for (const auto& dep : mDeps) {
        out << fmt::format(
            "target_link_libraries({} {} {})\n", lib_name, lib_type, boost::join(dep.cmake_targets, " "));
    }

    if (const auto& defs = mDefinitions; !defs.empty()) {
        out << fmt::format("\ntarget_compile_definitions({} {} {})\n", lib_name, lib_type, boost::join(defs, " "));
    }
}