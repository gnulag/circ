cmake_minimum_required(VERSION 2.8)

project(circ C)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(PrettyCompilerColors)

add_executable(circ
  "lib/config/config.c"
  "lib/config/config.h"
  "lib/log/log.c"
  "lib/log/log.h"
  "circ.c"
)

find_library(LIBEV_LIBS        NAMES ev        REQUIRED)
find_library(LIBUNISTRING_LIBS NAMES unistring REQUIRED)

target_link_libraries(
	circ
	# ${LIBEV_LIBS}
	# ${LIBUNISTRING_LIBS}
)
