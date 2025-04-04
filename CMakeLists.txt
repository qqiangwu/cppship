cmake_minimum_required(VERSION 3.17)
project(cppship VERSION 0.8.1)

# cpp std
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

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

    add_compile_options($<$<CONFIG:Debug>:-fsanitize=undefined>)
    add_link_options($<$<CONFIG:Debug>:-fsanitize=undefined>)

    # macos with gcc11 has bugs in asan:
    # `member call on address 0x60b0000001a0 which does not point to an object of type '_Sp_counted_base'`
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Enable asan")

        add_compile_options($<$<CONFIG:Debug>:-fsanitize=address>)
        add_link_options($<$<CONFIG:Debug>:-fsanitize=address>)
    endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-Wno-restrict)
		add_compile_options(-Wno-free-nonheap-object)
		add_compile_options(-Wno-dangling-reference)
	endif()
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
