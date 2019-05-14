cmake_minimum_required(VERSION 2.8)

project(circ)

set(SOURCES
  "lib/config/config.c"
  "lib/config/config.h"
  "lib/log/log.c"
  "lib/log/log.h"
  "circ.c"
)

message(STATUS "SOURCES=${SOURCES}")

add_executable(circ ${SOURCES})
