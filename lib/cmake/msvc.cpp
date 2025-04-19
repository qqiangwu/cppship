#include "cppship/cmake/msvc.h"

#include <boost/algorithm/string.hpp>

#include "cppship/util/io.h"

using namespace cppship;

fs::path msvc::fix_bin_path(const cppship::cmd::BuildContext& ctx, std::string_view bin)
{
    const auto profile = read_as_string(ctx.conan_profile_path);
    if (boost::contains(profile, "compiler=msvc")) {
        return fs::path(ctx.profile) / bin;
    }

    return bin;
}
