#include "cppship/core/compiler.h"
#include "cppship/util/cmd.h"

#include <cctype>
#include <cstdlib>
#include <exception>
#include <optional>

#include <boost/algorithm/string.hpp>

using namespace std::literals;
using namespace cppship;
using namespace cppship::compiler;

std::string_view compiler::to_string(CompilerId compiler_id)
{
    switch (compiler_id) {
    case CompilerId::unknown:
        return "unknown";

    case CompilerId::apple_clang:
        return "apple-clang";

    case CompilerId::clang:
        return "clang";

    case CompilerId::gcc:
        return "gcc";
    }

    std::terminate();
}

namespace {

std::string detect_compiler_command()
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe): only use in one thread
    if (const auto* env = std::getenv("CXX")) {
        return env;
    }

    if (has_cmd("g++")) {
        return "g++";
    }

    if (has_cmd("clang++")) {
        return "clang++";
    }

    throw Error { "unable to detect compiler" };
}

CompilerId get_compiler_id(const std::string_view out)
{
    if (boost::contains(out, "Apple")) {
        return CompilerId::apple_clang;
    }

    if (boost::contains(out, "clang++")) {
        return CompilerId::clang;
    }

    if (boost::contains(out, "g++")) {
        return CompilerId::gcc;
    }

    return CompilerId::unknown;
}

std::string get_compiler_version_gcc(const std::string_view out)
{
    // g++ (GCC) 11.2.0
    std::vector<std::string> fields;
    boost::split(fields, out, boost::is_any_of(" "));
    if (fields.empty()) {
        throw Error { fmt::format("detect compiler version failed from {}", out) };
    }
    return fields.back();
}

std::string get_compiler_version_clang(const std::string_view out)
{
    // Apple clang version 13.0.0 (...)
    // clang version 12.0.0 (...)
    const std::string_view marker = "version";
    auto pos = out.find(marker);
    if (pos == std::string_view::npos) {
        throw Error { fmt::format("detect compiler version failed from {}", out) };
    }

    pos += marker.size();

    return { out.begin() + pos, out.end() };
}

int get_compiler_version(const std::string_view out, CompilerId compiler_id)
{
    const auto version_str = [=] {
        switch (compiler_id) {
        case CompilerId::apple_clang:
        case CompilerId::clang:
            return get_compiler_version_clang(out);

        case CompilerId::gcc:
            return get_compiler_version_gcc(out);

        case CompilerId::unknown:
            return ""s;
        }

        std::terminate();
    }();

    std::istringstream iss(version_str);
    int version = 0;
    iss >> version;
    if (version == 0) {
        throw Error { fmt::format("detect compiler version failed from {}", out) };
    }

    return version;
}

std::string get_libcxx(CompilerId compiler_id)
{
    switch (compiler_id) {
    case CompilerId::apple_clang:
    case CompilerId::clang:
        return "libc++";

    case CompilerId::gcc:
        return "libstdc++11";

    case CompilerId::unknown:
        return "";
    }

    std::terminate();
}

}

CompilerInfo::CompilerInfo()
    : mCommand(detect_compiler_command())
{
    const auto out = check_output(fmt::format("{} --version | head -n1", mCommand));

    mId = get_compiler_id(out);
    mVersion = get_compiler_version(out, mId);
    mLibCxx = get_libcxx(mId);
}