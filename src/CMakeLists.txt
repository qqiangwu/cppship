add_executable(cppship main.cpp)

target_compile_definitions(cppship PRIVATE CPPSHIP_VERSION="${PROJECT_VERSION}")
target_link_libraries(cppship cppship_lib)

install(TARGETS cppship)