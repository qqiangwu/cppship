#include "cppship/cmd/index.h"
#include "cppship/core/manifest.h"
#include "cppship/exception.h"
#include "cppship/util/cmd.h"
#include "cppship/util/fs.h"
#include "cppship/util/log.h"
#include "cppship/util/string.h"
#include <filesystem>
#include <range/v3/algorithm/contains.hpp>
#include <range/v3/view.hpp>
#include <string>
#include <vector>

using namespace cppship;

int cmd::run_index(const IndexOptions& options)
{
    const std::vector<std::string> supported_operations { "add", "remove", "list" };
    if (!ranges::contains(supported_operations, options.operation)) {
        throw Error { std::format("Unrecognized operation '{}'", options.operation) };
    }
    const auto cppship_dir = get_cppship_dir();
    const auto index_store_dir = cppship_dir / "index_store";
    create_if_not_exist(index_store_dir);

    if (options.operation == "list") {
        const auto out = check_output("conan remote list");
        const auto lines = util::split(out, boost::is_any_of("\n"));
        fmt::print("local package indexes:\n");
        for (const auto& line : lines | ranges::views::filter([&index_store_dir](std::string_view line) {
                 return line.find(index_store_dir.string()) != std::string::npos;
                 ;
             })) {
            fmt::print("  {}\n", line);
        }
        return EXIT_SUCCESS;
    }

    if (!options.name) {
        throw Error { "--name option required" };
    }
    const auto index_dir = index_store_dir / options.name.value();
    if (options.operation == "add") {
        if (fs::exists(index_dir)) {
            throw Error { fmt::format("index with name '{}' already exists", options.name.value()) };
        }

        if (!options.path && !options.git) {
            throw Error { std::format("one of --path or --git must be given") };
        }
        const auto url = options.path.value_or(options.git.value_or(""));
        const auto git_cmd = fmt::format("git clone {} {}", url, index_dir.string());
        int res = run_cmd(git_cmd);
        if (res != 0) {
            throw Error { "git clone failed" };
        }

        const auto conan_cmd = fmt::format(
            "conan remote add --type local-recipes-index {} {}", options.name.value(), index_dir.string());
        res = run_cmd(conan_cmd);
        if (res != 0) {
            throw Error { fmt::format("failed adding '{}' as local index", index_dir.string()) };
        }
    }
    if (options.operation == "remove") {
        if (!fs::exists(index_dir)) {
            throw Error { std::format("no index with name '{}' exists", options.name.value()) };
        }
        fs::remove_all(index_dir);

        const auto conan_cmd = fmt::format("conan remote remove {}", options.name.value());
        int res = run_cmd(conan_cmd);
        if (res != 0) {
            throw Error { fmt::format("failed adding '{}' as local index", index_dir.string()) };
        }
    }

    return EXIT_SUCCESS;
}
