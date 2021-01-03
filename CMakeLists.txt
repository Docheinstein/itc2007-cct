cmake_minimum_required(VERSION 3.6)
project(itc2007-cct C CXX)

set(CMAKE_C_STANDARD 99)

message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

file(GLOB_RECURSE sources "src/*.c" "src/*.h")

file(GLOB_RECURSE gurobi "externals/gurobi/*.c" "externals/munit/*.h")
file(GLOB_RECURSE munit "externals/munit/*.c" "externals/munit/*.h")

list(FILTER sources EXCLUDE REGEX ".*main.c$")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

include_directories(src externals)
link_directories("${CMAKE_SOURCE_DIR}/lib")

add_executable(itc2007-cct ${sources} ${gurobi} "src/main.c")
add_executable(itc2007-cct-tests ${sources} ${munit} "tests/main.c")
add_executable(itc2007-cct-validator "validator/main.cpp")

target_include_directories(itc2007-cct PRIVATE ${GLIB_INCLUDE_DIRS})
target_include_directories(itc2007-cct-tests PRIVATE ${GLIB_INCLUDE_DIRS})

target_link_libraries(itc2007-cct ${CMAKE_DL_LIBS} ${GLIB_LIBRARIES} -lgurobi91)
target_link_libraries(itc2007-cct-tests ${CMAKE_DL_LIBS} ${GLIB_LIBRARIES} -lgurobi91)