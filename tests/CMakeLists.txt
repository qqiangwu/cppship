file(GLOB_RECURSE srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

find_package(GTest REQUIRED)

foreach(file ${srcs})
    # a/b/c.cpp => a_b_c_test
    string(REPLACE "/" "_" test_target ${file})
    string(REPLACE ".cpp" "_test" test_target ${test_target})

    add_executable(${test_target} ${file})

    target_link_libraries(${test_target} PRIVATE ${PROJECT_NAME}_lib)
    target_link_libraries(${test_target} PRIVATE GTest::gtest_main)

    add_test(${test_target} ${test_target})
endforeach()