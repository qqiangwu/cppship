add_executable(cppship_bin main.cpp)

target_compile_definitions(cppship_bin PRIVATE CPPSHIP_VERSION="${PROJECT_VERSION}")
target_link_libraries(cppship_bin cppship)
set_target_properties(cppship_bin PROPERTIES OUTPUT_NAME "cppship")

install(TARGETS cppship_bin)