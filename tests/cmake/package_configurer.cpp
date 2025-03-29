#include "cppship/cmake/package_configurer.h"
#include "cppship/core/dependency.h"
#include "cppship/util/fs.h"
#include "cppship/util/io.h"
#include "cppship/util/repo.h"

#include <string>

#include <boost/algorithm/string/replace.hpp>
#include <fmt/core.h>
#include <gtest/gtest.h>

using namespace cppship;
using namespace std::string_literals;

TEST(package_configurer, HeaderOnly)
{
    const auto package = "test_pack"s;
    const auto tmpdir = fs::temp_directory_path();
    const auto deps_dir = tmpdir / "deps";
    const auto package_dir = deps_dir / package;
    fs::remove_all(deps_dir);

    fs::create_directory(deps_dir);
    fs::create_directory(package_dir);
    fs::create_directory(package_dir / "include");

    cppship::ResolvedDependencies deps { {
        Dependency {
            .package = package,
            .cmake_package = package,
            .cmake_target = fmt::format("cppship::{}", package),
        },
    } };

    cmake::config_packages(deps, deps,
        {
            .deps_dir = deps_dir,
            .post_process
            = [deps_dir](std::string& str) { boost::replace_all(str, deps_dir.string(), "${CMAKE_BINARY_DIR}/deps"); },
        });

    const auto package_config_file = deps_dir / fmt::format("{}-config.cmake", package);
    ASSERT_TRUE(fs::exists(package_config_file));
    ASSERT_EQ(read_as_string(package_config_file), R"(# header only lib config generated by cppship
add_library(cppship::test_pack INTERFACE IMPORTED)
target_include_directories(cppship::test_pack INTERFACE ${CMAKE_SOURCE_DIR}/deps/test_pack/include)
)");
}

TEST(package_configurer, CppshipHeaderOnly)
{
    const auto package = "test_pack"s;
    const auto tmpdir = fs::temp_directory_path();
    const auto deps_dir = tmpdir / "deps";
    const auto package_dir = deps_dir / package;
    fs::remove_all(deps_dir);

    fs::create_directory(deps_dir);
    fs::create_directory(package_dir);
    fs::create_directory(package_dir / "include");
    write(package_dir / kRepoConfigFile, R"([package]
name = "test_pack"
version = "0.1.0"

[dependencies]
fmt = "9.1.0"
)");

    cppship::ResolvedDependencies deps { {
        Dependency {
            .package = package,
            .cmake_package = package,
            .cmake_target = fmt::format("cppship::{}", package),
        },
    } };
    cppship::ResolvedDependencies all_deps = deps;
    all_deps.insert(Dependency{
        .package = "fmt",
        .cmake_package = "fmt",
        .cmake_target = "fmt::fmt",
    });

    cmake::config_packages(deps, all_deps,
        {
            .deps_dir = deps_dir,
            .post_process
            = [deps_dir](
                  std::string& str) { boost::replace_all(str, deps_dir.generic_string(), "${CMAKE_BINARY_DIR}/deps"); },
        });

    const auto package_config_file = deps_dir / fmt::format("{}-config.cmake", package);
    ASSERT_TRUE(fs::exists(package_config_file));
    ASSERT_EQ(read_as_string(package_config_file), R"(
# LIB
add_library(test_pack_lib INTERFACE)

target_include_directories(test_pack_lib INTERFACE ${CMAKE_BINARY_DIR}/deps/test_pack/include)

target_link_libraries(test_pack_lib INTERFACE fmt::fmt)

add_library(cppship::test_pack ALIAS test_pack_lib)
)");
}

TEST(package_configurer, CppshipLib)
{
    const auto package = "test_pack"s;
    const auto tmpdir = fs::temp_directory_path();
    const auto deps_dir = tmpdir / "deps";
    const auto package_dir = deps_dir / package;
    const auto lib_dir = package_dir / kLibPath;
    fs::remove_all(deps_dir);

    fs::create_directory(deps_dir);
    fs::create_directory(package_dir);
    fs::create_directory(package_dir / "include");
    fs::create_directory(lib_dir);

    write(package_dir / kRepoConfigFile, R"([package]
name = "test_pack"
version = "0.1.0"

[dependencies]
fmt = "9.1.0"
)");
    touch(lib_dir / "a.cpp");
    touch(lib_dir / "b.cpp");
    touch(lib_dir / "a_test.cpp");

    cppship::ResolvedDependencies deps { {
        Dependency {
            .package = package,
            .cmake_package = package,
            .cmake_target = fmt::format("cppship::{}", package),
        },
    } };
    cppship::ResolvedDependencies all_deps = deps;
    all_deps.insert(Dependency{
        .package = "fmt",
        .cmake_package = "fmt",
        .cmake_target = "fmt::fmt",
    });

    cmake::config_packages(deps, all_deps,
        {
            .deps_dir = deps_dir,
            .post_process
            = [deps_dir](
                  std::string& str) { boost::replace_all(str, deps_dir.generic_string(), "${CMAKE_BINARY_DIR}/deps"); },
        });

    const auto package_config_file = deps_dir / fmt::format("{}-config.cmake", package);
    ASSERT_TRUE(fs::exists(package_config_file));
    ASSERT_EQ(read_as_string(package_config_file), R"(
# LIB
add_library(test_pack_lib ${CMAKE_BINARY_DIR}/deps/test_pack/lib/a.cpp
${CMAKE_BINARY_DIR}/deps/test_pack/lib/b.cpp)

target_include_directories(test_pack_lib PUBLIC ${CMAKE_BINARY_DIR}/deps/test_pack/include)

target_link_libraries(test_pack_lib PUBLIC fmt::fmt)

add_library(cppship::test_pack ALIAS test_pack_lib)
)");
}