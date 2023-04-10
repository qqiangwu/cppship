#include "cppship/util/fs.h"
#include "cppship/exception.h"

#include <fstream>

void cppship::write_file(const fs::path& file, std::string_view content)
{
    std::ofstream ofs(file);
    ofs << content;
    ofs.flush();

    if (!ofs) {
        throw Error { fmt::format("write file {} failed", file.string()) };
    }
}