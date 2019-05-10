cmake_minimum_required(VERSION 2.8)

project(circ)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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
