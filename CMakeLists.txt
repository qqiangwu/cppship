cmake_minimum_required(VERSION 3.17)
project(cppship VERSION 0.6.0)

# cpp std
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

# must go after cpp std block
include(cmake/conan.cmake)

add_definitions("-DBOOST_PROCESS_USE_STD_FS")

# cpp warnings
if(MSVC)
    add_compile_options("/Zc:__cplusplus")
    add_compile_options("/Zc:preprocessor")
    add_compile_options("/MP")
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

add_subdirectory(lib)
add_subdirectory(src)

if(ENABLE_TEST)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
