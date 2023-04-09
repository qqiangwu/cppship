file(GLOB lib_srcs ${CMAKE_CURRENT_SOURCE_DIR}/**.cpp)
file(GLOB srcs ${CMAKE_CURRENT_SOURCE_DIR}/*/**.cpp)

add_library(cppship ${lib_srcs} ${srcs})

target_include_directories(cppship PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(cppship PUBLIC ${CMAKE_SOURCE_DIR}/include)

find_package(boost 1.81.0 REQUIRED)
find_package(structopt REQUIRED)
find_package(libgit2 REQUIRED)
find_package(fmt REQUIRED)
find_package(range-v3 REQUIRED)
find_package(bshoshany-thread-pool REQUIRED)
find_package(Microsoft.GSL REQUIRED)

target_link_libraries(cppship PUBLIC Boost::headers)
target_link_libraries(cppship PUBLIC structopt::structopt)
target_link_libraries(cppship PUBLIC libgit2::libgit2)
target_link_libraries(cppship PUBLIC fmt::fmt)
target_link_libraries(cppship PUBLIC range-v3::range-v3)
target_link_libraries(cppship PUBLIC bshoshany-thread-pool::bshoshany-thread-pool)
target_link_libraries(cppship PUBLIC Microsoft.GSL::GSL)