cmake_minimum_required(VERSION 3.16)
project(cppship VERSION 0.4.0)

# cpp std
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

# must go after cpp std block
include(cmake/conan.cmake)

# cpp warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers")

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
