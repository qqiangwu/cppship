#include "cppship/cmake/dependency_injector.h"
#include "cppship/cmake/package_configurer.h"
#include "cppship/util/fs.h"
#include "cppship/util/io.h"

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fmt/format.h>
#include <fmt/os.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>

using namespace cppship;
using namespace cppship::cmake;
using namespace boost::algorithm;
using namespace ranges::views;
using namespace fmt::literals;

namespace {

constexpr std::string_view kConanCmakeScript = R"(# cmake conan adaptor
function(detect_os OS)
    # it could be cross compilation
    message(STATUS "Conan-cmake: cmake_system_name=${CMAKE_SYSTEM_NAME}")
    if(CMAKE_SYSTEM_NAME AND NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
        if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            set(${OS} Macos PARENT_SCOPE)
        elseif(${CMAKE_SYSTEM_NAME} STREQUAL "QNX")
            set(${OS} Neutrino PARENT_SCOPE)
        else()
            set(${OS} ${CMAKE_SYSTEM_NAME} PARENT_SCOPE)
        endif()
    endif()
endfunction()


function(detect_cxx_standard CXX_STANDARD)
    set(${CXX_STANDARD} ${CMAKE_CXX_STANDARD} PARENT_SCOPE)
    if (CMAKE_CXX_EXTENSIONS)
        set(${CXX_STANDARD} "gnu${CMAKE_CXX_STANDARD}" PARENT_SCOPE)
    endif()
endfunction()


function(detect_compiler COMPILER COMPILER_VERSION)
    if(DEFINED CMAKE_CXX_COMPILER_ID)
        set(_COMPILER ${CMAKE_CXX_COMPILER_ID})
        set(_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    else()
        if(NOT DEFINED CMAKE_C_COMPILER_ID)
            message(FATAL_ERROR "C or C++ compiler not defined")
        endif()
        set(_COMPILER ${CMAKE_C_COMPILER_ID})
        set(_COMPILER_VERSION ${CMAKE_C_COMPILER_VERSION})
    endif()

    message(STATUS "Conan-cmake: CMake compiler=${_COMPILER}")
    message(STATUS "Conan-cmake: CMake cmpiler version=${_COMPILER_VERSION}")

    if(_COMPILER MATCHES MSVC)
        set(_COMPILER "msvc")
        string(SUBSTRING ${MSVC_VERSION} 0 3 _COMPILER_VERSION)
    elseif(_COMPILER MATCHES AppleClang)
        set(_COMPILER "apple-clang")
        string(REPLACE "." ";" VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
        list(GET VERSION_LIST 0 _COMPILER_VERSION)
    elseif(_COMPILER MATCHES Clang)
        set(_COMPILER "clang")
        string(REPLACE "." ";" VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
        list(GET VERSION_LIST 0 _COMPILER_VERSION)
    elseif(_COMPILER MATCHES GNU)
        set(_COMPILER "gcc")
        string(REPLACE "." ";" VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
        list(GET VERSION_LIST 0 _COMPILER_VERSION)
    endif()

    message(STATUS "Conan-cmake: [settings] compiler=${_COMPILER}")
    message(STATUS "Conan-cmake: [settings] compiler.version=${_COMPILER_VERSION}")

    set(${COMPILER} ${_COMPILER} PARENT_SCOPE)
    set(${COMPILER_VERSION} ${_COMPILER_VERSION} PARENT_SCOPE)
endfunction()

function(detect_build_type BUILD_TYPE)
    if(NOT CMAKE_CONFIGURATION_TYPES)
        # Only set when we know we are in a single-configuration generator
        # Note: we may want to fail early if `CMAKE_BUILD_TYPE` is not defined
        set(${BUILD_TYPE} ${CMAKE_BUILD_TYPE} PARENT_SCOPE)
    endif()
endfunction()

# https://github.com/conan-io/conan/blob/47af537a5cc4722cf9186fb0f9ba4786aeba0086/conans/client/conf/detect.py#L181
function(detect_libcxx LIBCXX)
    if(CMAKE_CXX_COMPILER_ID MATCHES AppleClang)
        set(_LIBCXX "libc++")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES Clang)
        if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            set(_LIBCXX "libc++")
        else()
            # msvc is already ruled out in detect_compiler
            set(_LIBCXX "libstdc++11")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
        set(_LIBCXX "libstdc++11")
    else()
        message(FATAL_ERROR "${COMPILER} is not supported now")
    endif()

    message(STATUS "Conan-cmake: [settings] compiler.libcxx=${_LIBCXX}")

    set(${LIBCXX} ${_LIBCXX} PARENT_SCOPE)
endfunction()


function(detect_host_profile output_file)
    detect_os(MYOS)
    detect_compiler(MYCOMPILER MYCOMPILER_VERSION)
    detect_cxx_standard(MYCXX_STANDARD)
    detect_build_type(MYBUILD_TYPE)
    detect_libcxx(MYLIBCXX)

    set(PROFILE "")
    string(APPEND PROFILE "include(default)\n")
    string(APPEND PROFILE "[settings]\n")
    if(MYOS)
        string(APPEND PROFILE os=${MYOS} "\n")
    endif()
    if(MYCOMPILER)
        string(APPEND PROFILE compiler=${MYCOMPILER} "\n")
    endif()
    if(MYCOMPILER_VERSION)
        string(APPEND PROFILE compiler.version=${MYCOMPILER_VERSION} "\n")
    endif()
    if(MYCXX_STANDARD)
        string(APPEND PROFILE compiler.cppstd=${MYCXX_STANDARD} "\n")
    endif()
    if(MYBUILD_TYPE)
        string(APPEND PROFILE "build_type=${MYBUILD_TYPE}\n")
    endif()
    if(MYLIBCXX)
        string(APPEND PROFILE compiler.libcxx=${MYLIBCXX} "\n")
    endif()

    if(NOT DEFINED output_file)
        set(_FN "${CMAKE_BINARY_DIR}/profile")
    else()
        set(_FN ${output_file})
    endif()

    string(APPEND PROFILE "[conf]\n")
    string(APPEND PROFILE "tools.cmake.cmaketoolchain:generator=${CMAKE_GENERATOR}\n")

    message(STATUS "Conan-cmake: Creating profile ${_FN}")
    file(WRITE ${_FN} ${PROFILE})
    message(STATUS "Conan-cmake: Profile: \n${PROFILE}")
endfunction()


function(conan_profile_detect_default)
    message(STATUS "Conan-cmake: Checking if a default profile exists")
    execute_process(COMMAND conan profile path default
                    RESULT_VARIABLE return_code
                    OUTPUT_VARIABLE conan_stdout
                    ERROR_VARIABLE conan_stderr
                    ECHO_ERROR_VARIABLE    # show the text output regardless
                    ECHO_OUTPUT_VARIABLE
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    if(NOT ${return_code} EQUAL "0")
        message(STATUS "Conan-cmake: The default profile doesn't exist, detecting it.")
        execute_process(COMMAND conan profile detect
            RESULT_VARIABLE return_code
            OUTPUT_VARIABLE conan_stdout
            ERROR_VARIABLE conan_stderr
            ECHO_ERROR_VARIABLE    # show the text output regardless
            ECHO_OUTPUT_VARIABLE
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endfunction()


function(conan_install)
    cmake_parse_arguments(ARGS CONAN_ARGS ${ARGN})
    set(CONAN_OUTPUT_FOLDER ${CMAKE_BINARY_DIR}/conan)
    # Invoke "conan install" with the provided arguments
    set(CONAN_ARGS ${CONAN_ARGS} -of=${CONAN_OUTPUT_FOLDER})
    message(STATUS "CMake-conan: conan install ${CMAKE_SOURCE_DIR} ${CONAN_ARGS} ${ARGN}")
    execute_process(COMMAND conan install ${CMAKE_SOURCE_DIR} ${CONAN_ARGS} ${ARGN} --format=json
                    RESULT_VARIABLE return_code
                    OUTPUT_VARIABLE conan_stdout
                    ERROR_VARIABLE conan_stderr
                    ECHO_ERROR_VARIABLE    # show the text output regardless
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    if(NOT "${return_code}" STREQUAL "0")
        message(FATAL_ERROR "Conan install failed='${return_code}'")
    else()
        # the files are generated in a folder that depends on the layout used, if
        # one if specified, but we don't know a priori where this is.
        # TODO: this can be made more robust if Conan can provide this in the json output
        string(JSON CONAN_GENERATORS_FOLDER GET ${conan_stdout} graph nodes 0 generators_folder)
        # message("conan stdout: ${conan_stdout}")
        message(STATUS "CMake-conan: CONAN_GENERATORS_FOLDER=${CONAN_GENERATORS_FOLDER}")
        set(CONAN_GENERATORS_FOLDER "${CONAN_GENERATORS_FOLDER}" PARENT_SCOPE)

        # commit
        file(TIMESTAMP ${CMAKE_SOURCE_DIR}/conanfile.txt CONAN_INSTALL_TIME)

        set(CONAN_INSTALL_SUCCESS ${CONAN_INSTALL_TIME} CACHE STRING "Conan install has been invoked and was successful")
    endif()
endfunction()


function(revalidate_dependencies)
    if(CONAN_INSTALL_SUCCESS)
        file(TIMESTAMP ${CMAKE_SOURCE_DIR}/conanfile.txt CONANTXT_MTIME)

        if(NOT ${CONANTXT_MTIME} STREQUAL ${CONAN_INSTALL_SUCCESS})
            unset(CONAN_INSTALL_SUCCESS CACHE)
        endif()
    endif()
endfunction()


macro(conan_setup)
    revalidate_dependencies()

    if(NOT CONAN_INSTALL_SUCCESS)
        message(STATUS "CMake-conan: Installing dependencies with Conan")
        conan_profile_detect_default()
        detect_host_profile(${CMAKE_BINARY_DIR}/conan_host_profile)
        if(NOT CMAKE_CONFIGURATION_TYPES)
            message(STATUS "CMake-conan: Installing single configuration ${CMAKE_BUILD_TYPE}")
            conan_install(-pr ${CMAKE_BINARY_DIR}/conan_host_profile --build=missing -g CMakeDeps)
        else()
            message(STATUS "CMake-conan: Installing both Debug and Release")
            conan_install(-pr ${CMAKE_BINARY_DIR}/conan_host_profile -s build_type=Release --build=missing -g CMakeDeps)
            conan_install(-pr ${CMAKE_BINARY_DIR}/conan_host_profile -s build_type=Debug --build=missing -g CMakeDeps)
        endif()
        if (CONAN_INSTALL_SUCCESS)
            set(CONAN_GENERATORS_FOLDER "${CONAN_GENERATORS_FOLDER}" CACHE PATH "Conan generators folder")
        endif()
    else()
        message(STATUS "CMake-conan: 'conan install' aready ran")
    endif()

    if (CONAN_GENERATORS_FOLDER)
        list(PREPEND CMAKE_PREFIX_PATH "${CONAN_GENERATORS_FOLDER}")
    endif()
endmacro()

conan_setup())";

void inject_conan_deps(std::ostream& out)
{
    const fs::path cmake_util_dir = "cmake";

    create_if_not_exist(cmake_util_dir);
    write(cmake_util_dir / "conan.cmake", kConanCmakeScript);

    out << "include(cmake/conan.cmake)\n";
}

void inject_git_deps(std::ostream& out, const fs::path& deps_dir, const std::vector<DeclaredDependency>& deps,
    const ResolvedDependencies& cppship_deps, const ResolvedDependencies& all_deps)
{
    const fs::path cmake_util_dir = "cmake";
    create_if_not_exist(cmake_util_dir);

    static constexpr std::string_view kCmakeDepsDir = "${CMAKE_BINARY_DIR}/deps";

    auto oss = fmt::output_file((cmake_util_dir / "deps.cmake").string());
    oss.print("include(FetchContent)\n\n");

    for (const auto& dep : deps) {
        const auto& desc = get<GitDep>(dep.desc);

        oss.print(R"(# Dep for {package}
FetchContent_Declare({package}
    GIT_REPOSITORY "{git}"
    GIT_TAG "{commit}"
    SOURCE_DIR "{deps_dir}/{package}"
)
FetchContent_MakeAvailable({package})
message("-- Deps: download {package} from {git}::{commit}")

)",
            "package"_a = dep.package, "git"_a = desc.git, "commit"_a = desc.commit, "deps_dir"_a = kCmakeDepsDir);
    }

    // commit file
    oss.print(R"(list(PREPEND CMAKE_PREFIX_PATH "${{CMAKE_SOURCE_DIR}}/cmake"))");
    oss.print("\n");
    oss.close();

    auto content_fix
        = [deps_dir = deps_dir.string()](std::string& str) { boost::replace_all(str, deps_dir, kCmakeDepsDir); };
    cmake::config_packages(cppship_deps, all_deps,
        {
            .deps_dir = deps_dir,
            .out_dir = cmake_util_dir,
            .cmake_deps_dir = std::string { kCmakeDepsDir },
            .post_process = std::move(content_fix),
        });

    out << "include(cmake/deps.cmake)\n";
}

}

void CmakeDependencyInjector::inject(std::ostream& out, const Manifest&)
{
    if (mDeclaredDeps.empty()) {
        return;
    }

    const auto has_conan_deps = any_of(mDeclaredDeps, [](const DeclaredDependency& dep) { return dep.is_conan(); });

    out << "\n# Dependency management\n";
    if (has_conan_deps) {
        inject_conan_deps(out);
    }

    if (!mCppshipDeps.empty()) {
        const auto declared_cppship_deps = mDeclaredDeps
            | filter([](const DeclaredDependency& dep) { return dep.is_git(); })
            | ranges::to<std::vector<DeclaredDependency>>();
        inject_git_deps(out, mDepsDir, declared_cppship_deps, mCppshipDeps, mAllDeps);
    }
}