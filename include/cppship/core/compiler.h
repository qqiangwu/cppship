#pragma once

#include <string>

namespace cppship::compiler {

enum class CompilerId { unknown, gcc, clang, apple_clang };

std::string_view to_string(CompilerId compiler_id);

class CompilerInfo {
public:
    CompilerInfo();

    CompilerId id() const { return mId; }

    std::string_view command() const { return mCommand; }

    std::string_view libcxx() const { return mLibCxx; }

    int version() const { return mVersion; }

private:
    std::string mCommand;
    CompilerId mId = CompilerId::unknown;
    int mVersion = 0;
    std::string mLibCxx;
};

}