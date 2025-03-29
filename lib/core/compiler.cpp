#include "cppship/core/compiler.h"
#include "cppship/util/cmd.h"

#include <cctype>
#include <cstdlib>
#include <exception>
#include <sstream>

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

    case CompilerId::msvc:
        return "msvc";
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

    if (boost::contains(out, "clang")) {
        return CompilerId::clang;
    }

    if (boost::contains(out, "g++")) {
        return CompilerId::gcc;
    }

    return CompilerId::unknown;
}

int get_compiler_version(const std::string_view cxx)
{
    const auto out = check_output(fmt::format("{} -dumpversion", cxx));

    std::istringstream iss(out);
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

    case CompilerId::msvc:
    case CompilerId::unknown:
        return "";
    }

    std::terminate();
}

}

CompilerInfo::CompilerInfo()
    : mCommand(detect_compiler_command())
    , mVersion(get_compiler_version(mCommand))
{
    const auto out = check_output(fmt::format("{} --version | head -n1", mCommand));

    mId = get_compiler_id(out);
    mLibCxx = get_libcxx(mId);
}