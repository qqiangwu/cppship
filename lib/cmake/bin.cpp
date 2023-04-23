#include "cppship/cmake/bin.h"
#include "cppship/exception.h"
#include "cppship/util/repo.h"

#include <boost/algorithm/string/join.hpp>
#include <fmt/core.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

using namespace cppship;
using namespace cppship::cmake;
using namespace ranges::views;

CmakeBin::CmakeBin(BinDesc desc)
    : mDesc(std::move(desc))
{
    if (mDesc.sources.empty()) {
        throw Error { "bin sources are empty" };
    }
}

void CmakeBin::build(std::ostream& out) const
{
    out << "\n# BIN\n";
    out << fmt::format("add_executable({} {})\n", mDesc.name,
        boost::join(mDesc.sources | transform([](const auto& path) { return path.string(); }), "\n"));

    if (mDesc.include_dir) {
        out << "\n"
            << fmt::format(
                   "target_include_directories({} PRIVATE ${{CMAKE_SOURCE_DIR}}/{})\n", mDesc.name, *mDesc.include_dir);
    }

    if (mDesc.lib) {
        out << "\n" << fmt::format("target_link_libraries({} PRIVATE {})\n", mDesc.name, *mDesc.lib);
    }

    if (!mDesc.deps.empty()) {
        out << "\n";
        for (const auto& dep : mDesc.deps) {
            out << fmt::format(
                "target_link_libraries({} PRIVATE {})\n", mDesc.name, boost::join(dep.cmake_targets, " "));
        }
    }

    if (const auto& defs = mDesc.definitions; !defs.empty()) {
        out << fmt::format("\ntarget_compile_definitions({} PRIVATE {})\n", mDesc.name, boost::join(defs, " "));
    }

    if (mDesc.name_alias || mDesc.runtime_dir) {
        out << "\n";

        if (const auto& name_alias = mDesc.name_alias) {
            out << fmt::format(R"(set_target_properties({} PROPERTIES OUTPUT_NAME "{}"))", mDesc.name, *name_alias)
                << '\n';
        }
        if (const auto& runtime_dir = mDesc.runtime_dir) {
            out << fmt::format(
                R"(set_target_properties({} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${{CMAKE_BINARY_DIR}}/{}"))",
                mDesc.name, *runtime_dir)
                << '\n';
        }
    }

    if (mDesc.need_install) {
        out << "\n" << fmt::format("install(TARGETS {})\n", mDesc.name);
    }
}