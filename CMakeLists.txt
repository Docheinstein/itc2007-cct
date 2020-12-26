cmake_minimum_required(VERSION 3.0)
project(itc2007_cct C)

set(CMAKE_C_STANDARD 99)

file(GLOB_RECURSE sources
        "src/*.c"
        "src/*.h"
)

include_directories(src)

add_executable(itc2007-cct ${sources})