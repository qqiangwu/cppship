#pragma once

#include <sstream>
#include <string>

#include "cppship/exception.h"

namespace cppship {

inline std::string read_all(std::istream& iss)
{
    std::ostringstream oss;
    oss << iss.rdbuf();

    if (!oss) {
        throw Error { "drain stream failed" };
    }

    return oss.str();
}

}