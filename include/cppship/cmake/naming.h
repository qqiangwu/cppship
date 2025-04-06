#pragma once

#include <string>
#include <string_view>

namespace cppship::cmake {

class NameTargetMapper {
public:
    explicit NameTargetMapper(std::string_view package)
        : mPackage(package)
    {
    }

    std::string binary(std::string_view name);
    std::string test(std::string_view name);
    std::string example(std::string_view name);
    std::string bench(std::string_view name);

private:
    std::string mPackage;
};

}
