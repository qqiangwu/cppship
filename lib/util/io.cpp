#include "cppship/util/io.h"
#include "cppship/exception.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

void cppship::write(const fs::path& file, std::string_view content)
{
    std::ofstream ofs(file);
    if (!ofs) {
        throw IOError { fmt::format("cannot open file {} to write", file.string()) };
    }

    ofs << content;
    ofs.flush();

    if (!ofs) {
        throw IOError { fmt::format("write file {} failed", file.string()) };
    }
}

std::string cppship::read_as_string(const fs::path& file)
{
    try {
        std::ifstream ifs(file);
        return read_as_string(ifs);
    } catch (const Error&) {
        throw IOError { fmt::format("read file {} failed", file.string()) };
    }
}

std::string cppship::read_as_string(std::istream& iss)
{
    std::ostringstream oss;
    std::copy(std::istreambuf_iterator<char> { iss }, std::istreambuf_iterator<char> {},
        std::ostreambuf_iterator<char> { oss });

    if (!iss || !oss) {
        throw IOError { "drain stream failed" };
    }

    return oss.str();
}