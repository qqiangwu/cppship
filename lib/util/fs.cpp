#include "cppship/util/fs.h"
#include "cppship/exception.h"

#include <fstream>
#include <sstream>

void cppship::write_file(const fs::path& file, std::string_view content)
{
    std::ofstream ofs(file);
    ofs << content;
    ofs.flush();

    if (!ofs) {
        throw Error { fmt::format("write file {} failed", file.string()) };
    }
}

std::string cppship::read_file(const fs::path& file)
{
    std::ostringstream oss;
    std::ifstream ifs(file);

    ifs.exceptions(std::ios::badbit);
    oss << ifs.rdbuf();

    return oss.str();
}