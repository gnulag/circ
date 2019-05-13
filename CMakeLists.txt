cmake_minimum_required(VERSION 2.8)

project(circ)

file(GLOB SOURCES
  "lib/*/*.h"
  "lib/*/*.c"
  "lib/*.h"
  "lib/*.c"
  "*.c"
  "*.h"
)

message(STATUS "SOURCES=${SOURCES}")

add_executable(circ ${SOURCES})
