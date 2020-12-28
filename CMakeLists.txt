cmake_minimum_required(VERSION 3.0)
project(itc2007-cct C)

set(CMAKE_C_STANDARD 99)

message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

file(GLOB_RECURSE sources "src/*.c" "src/*.h")
file(GLOB_RECURSE tests "tests/*.c" "tests/*.h")
file(GLOB_RECURSE munit "externals/munit/*.c" "externals/munit/*.h")

list(FILTER sources EXCLUDE REGEX ".*main.c$")
list(FILTER tests EXCLUDE REGEX ".*main.c$")

include_directories(src externals)

add_executable(itc2007-cct ${sources} "src/main.c")
add_executable(itc2007-cct-tests ${sources} ${tests} ${munit} "tests/main.c")
