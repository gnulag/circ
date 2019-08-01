cmake_minimum_required(VERSION 2.8)

project(circ C)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_BUILD_TYPE Debug)

include(PrettyCompilerColors)
find_package(GLIB COMPONENTS gobject REQUIRED)

include_directories(
    ./lib
    ${GLIB_INCLUDE_DIRS}
)

add_executable(circ
  "lib/irc-parser/ircium-message.c"
  "lib/irc-parser/ircium-message-tag.c"
  "lib/config/config.c"
  "lib/config/config.h"
  "lib/cJSON/cJSON.c"
  "lib/cJSON/cJSON.h"
  "lib/log/log.c"
  "lib/log/log.h"
  "lib/irc/irc.c"
  "lib/irc/hooks.c"
  "lib/irc/core_hooks.c"
  "lib/b64/decode.c"
  "lib/b64/encode.c"
  "src/circ.c"
)

find_library(LIBEV_LIBS        NAMES ev          REQUIRED)
find_library(LIBGNUTLS_LIBS    NAMES gnutls      REQUIRED)
find_library(LIBUNISTRING_LIBS NAMES unistring   REQUIRED)

target_link_libraries(
	circ
	${LIBEV_LIBS}
	${LIBGNUTLS_LIBS}
	${LIBUNISTRING_LIBS}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
)

