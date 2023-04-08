add_executable(cppship_bin main.cpp)

target_link_libraries(cppship_bin cppship)
set_target_properties(cppship_bin PROPERTIES OUTPUT_NAME "cppship")

install(TARGETS cppship_bin)