cmake_minimum_required(VERSION 2.8)

project(circ C)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "debug build" FORCE)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(PrettyCompilerColors)

add_subdirectory(log)
add_subdirectory(libirc)

set(CIRC_SOURCES
	src/config/config.c
	src/core_hooks.c
	src/scheme/api.c
	src/scheme/scheme.c
	src/circ.c

	thirdparty/cJSON/cJSON.c
)

add_executable(circ ${CIRC_SOURCES})

find_library(LIBEV_LIBS     NAMES ev           REQUIRED)
find_library(LIBGNUTLS_LIBS NAMES gnutls       REQUIRED)
find_package(GLIB           COMPONENTS gobject REQUIRED)
find_library(LIBCHIBI_LIBS  NAMES chibi-scheme REQUIRED)

target_include_directories(circ
	PRIVATE src
	PRIVATE thirdparty
	PRIVATE ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(circ
	irc
	log
	${LIBEV_LIBS}
	${LIBGNUTLS_LIBS}
	${GLIB_GOBJECT_LIBRARIES}
	${GLIB_LIBRARIES}
	${LIBCHIBI_LIBS}
)
