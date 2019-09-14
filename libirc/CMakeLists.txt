set(IRC_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/irc/irc.h
	${CMAKE_CURRENT_SOURCE_DIR}/irc.c
	${CMAKE_CURRENT_SOURCE_DIR}/irc/hooks.h
	${CMAKE_CURRENT_SOURCE_DIR}/hooks.c
	${CMAKE_CURRENT_SOURCE_DIR}/irc/message.h
	${CMAKE_CURRENT_SOURCE_DIR}/message.c
	${CMAKE_CURRENT_SOURCE_DIR}/irc/serializer.h
	${CMAKE_CURRENT_SOURCE_DIR}/serializer.c
	${CMAKE_CURRENT_SOURCE_DIR}/irc/parser.h
	${CMAKE_CURRENT_SOURCE_DIR}/parser.c
)
set(IRC_SOURCES ${IRC_SOURCES} PARENT_SCOPE)

set(IRCMSG_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/ircmsg/src/parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/ircmsg/include/ircmsg/parser.h
	${CMAKE_CURRENT_SOURCE_DIR}/ircmsg/src/serializer.c
	${CMAKE_CURRENT_SOURCE_DIR}/ircmsg/include/ircmsg/serializer.h
)
set(IRCMSG_SOURCES ${IRCMSG_SOURCES} PARENT_SCOPE)

# get and build ircmsg
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ircmsg/README.md)
	execute_process(
		COMMAND git submodule update --recursive
	)
endif()

set(IRC_SOURCES_EXTRA
	../thirdparty/b64/b64.h
	../thirdparty/b64/decode.c
	../thirdparty/b64/encode.c
)

add_library(irc ${IRC_SOURCES} ${IRCMSG_SOURCES} ${IRC_SOURCES_EXTRA})

find_package(GLIB COMPONENTS gobject REQUIRED)

target_include_directories(irc
	PUBLIC irc
	PUBLIC .
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ircmsg/include
	PRIVATE ../thirdparty
	PRIVATE ../src # TODO: this must be removed
	PRIVATE ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(irc log)
