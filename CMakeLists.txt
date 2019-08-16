cmake_minimum_required(VERSION 2.8)

project(circ C)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "debug build" FORCE)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(PrettyCompilerColors)

add_subdirectory(log)
add_subdirectory(libirc)

set(CIRC_SOURCES
	src/config/config.h
	src/config/config.c
	src/core_hooks.c
	src/scheme/scheme.h
	src/scheme/scmapi.c
	src/scheme/scheme.c
	src/circ.c
)

set(CIRC_SOURCES_EXTRA
	thirdparty/cJSON/cJSON.h
	thirdparty/cJSON/cJSON.c
)

add_executable(circ ${CIRC_SOURCES} ${CIRC_SOURCES_EXTRA})

find_library(LIBEV_LIBS      NAMES ev           REQUIRED)
find_library(LIBGNUTLS_LIBS  NAMES gnutls       REQUIRED)
find_library(LIBSQLITE3_LIBS NAMES sqlite3      REQUIRED)
find_package(GLIB            COMPONENTS gobject REQUIRED)

# get and build chibi
if(NOT IS_DIRECTORY ${CMAKE_BINARY_DIR}/chibi-scheme)
	execute_process(
		COMMAND git clone git@github.com:ashinn/chibi-scheme
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	)
endif()

include(ProcessorCount)
ProcessorCount(num_cpus)

execute_process(
	COMMAND make -j${num_cpus}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/chibi-scheme
)

find_library(LIBCHIBI_LIBS
	NAMES chibi-scheme
	PATHS ${CMAKE_BINARY_DIR}/chibi-scheme
	REQUIRED
)

target_include_directories(circ
	PRIVATE src
	PRIVATE thirdparty
	PRIVATE ${GLIB_INCLUDE_DIRS}
	PRIVATE ${CMAKE_BINARY_DIR}/chibi-scheme/include
)

target_link_libraries(circ
	irc
	log
	${LIBEV_LIBS}
	${LIBGNUTLS_LIBS}
	${LIBSQLITE3_LIBS}
	${GLIB_GOBJECT_LIBRARIES}
	${GLIB_LIBRARIES}
	${LIBCHIBI_LIBS}
)

include(ClangFormat)

clangformat_setup(
	${LOG_SOURCES}
	${IRC_SOURCES}
	${CIRC_SOURCES}
)

add_dependencies(circ clangformat)
