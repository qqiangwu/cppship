file(GLOB srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

find_package(GTest REQUIRED)

foreach(file ${srcs})
    get_filename_component(file_stem ${file} NAME_WE)
    set(test_target ${file_stem}_test)

    add_executable(${test_target} ${file})

    target_link_libraries(${test_target} PRIVATE ${PROJECT_NAME})
    target_link_libraries(${test_target} PRIVATE GTest::gtest_main)

    add_test(${test_target} ${test_target})
endforeach()