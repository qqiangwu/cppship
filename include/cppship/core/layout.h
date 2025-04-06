#pragma once

#include <map>
#include <optional>
#include <set>
#include <vector>

#include "cppship/util/fs.h"

namespace cppship {

struct Target {
    std::string name;
    std::set<fs::path> includes;
    std::set<fs::path> sources;
};

class Layout {
public:
    Layout(const fs::path& root, std::string_view name);

    std::string_view package() const { return mName; }

    std::set<fs::path> all_files() const;

    std::optional<Target> lib() const;

    std::optional<Target> binary(std::string_view name) const;
    std::optional<Target> example(std::string_view name) const;
    std::optional<Target> bench(std::string_view name) const;
    std::optional<Target> test(std::string_view name) const;

    std::vector<Target> binaries() const;
    std::vector<Target> examples() const;
    std::vector<Target> benches() const;
    std::vector<Target> tests() const;

private:
    void scan_binaries_();

    void scan_benches_();

    void scan_tests_();

    void scan_examples_();

    void scan_lib_();

private:
    fs::path mRoot;
    std::string mName;

    std::set<fs::path> mSources;

    std::optional<Target> mLib;
    std::map<std::string, Target, std::less<>> mBinaries;
    std::map<std::string, Target, std::less<>> mExamples;
    std::map<std::string, Target, std::less<>> mBenches;
    std::map<std::string, Target, std::less<>> mTests;
};

}
