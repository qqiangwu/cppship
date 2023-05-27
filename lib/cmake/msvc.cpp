#include "cppship/cmake/msvc.h"
#include "cppship/util/io.h"

#include <boost/algorithm/string.hpp>

using namespace cppship;

fs::path msvc::fix_bin(const cppship::cmd::BuildContext& ctx, std::string_view bin)
{
    const auto profile = read_as_string(ctx.conan_profile_path);
    if (boost::contains(profile, "compiler=msvc")) {
        return fs::path(ctx.profile) / bin;
    }

    return bin;
}