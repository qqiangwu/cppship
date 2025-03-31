#include "cppship/util/io.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

#include "cppship/exception.h"
#include "cppship/util/fs.h"

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

void cppship::touch(const fs::path& file)
{
    if (!fs::exists(file)) {
        std::ofstream ofs(file);
        ofs.close();
        return;
    }

    fs::last_write_time(file, fs::file_time_type::clock::now());
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
    std::copy(std::istreambuf_iterator<char> { iss },
        std::istreambuf_iterator<char> {},
        std::ostreambuf_iterator<char> { oss });

    if (!iss || !oss) {
        throw IOError { "drain stream failed" };
    }

    return oss.str();
}
