#include "cppship/core/template.h"
#include "cppship/util/repo.h"

using namespace cppship;

void cppship::generate_lib_template(const fs::path& dir)
{
    const auto inc = dir / kIncludePath;
    const auto lib = dir / kLibPath;

    create_if_not_exist(inc);
    create_if_not_exist(lib);

    write_file(lib / "lib.cpp", R"(int add(int x, int y) { return x + y; }
)");
}

void cppship::generate_bin_template(const fs::path& dir)
{
    const auto src = dir / kSrcPath;

    create_if_not_exist(src);

    write_file(src / "main.cpp", R"(#include <iostream>

int main()
{
    std::cout << "Hello cppship\n";
}
)");
}