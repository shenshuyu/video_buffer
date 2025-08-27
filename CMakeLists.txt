cmake_minimum_required(VERSION 3.10)
project(test C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/fifo.c
    test.cc
)

include_directories(
    src/
)

add_executable(test ${SOURCES})
target_link_libraries(test pthread)