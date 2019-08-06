set(IRC_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/irc/irc.h
	${CMAKE_CURRENT_SOURCE_DIR}/irc.c
	${CMAKE_CURRENT_SOURCE_DIR}/irc/hooks.h
	${CMAKE_CURRENT_SOURCE_DIR}/hooks.c
	${CMAKE_CURRENT_SOURCE_DIR}/ircium-parser/ircium-message.h
	${CMAKE_CURRENT_SOURCE_DIR}/ircium-parser/ircium-message.c
	${CMAKE_CURRENT_SOURCE_DIR}/ircium-parser/ircium-message-tag.h
	${CMAKE_CURRENT_SOURCE_DIR}/ircium-parser/ircium-message-tag.c
)
set(IRC_SOURCES ${IRC_SOURCES} PARENT_SCOPE)

set(IRC_SOURCES_EXTRA
	../thirdparty/b64/b64.h
	../thirdparty/b64/decode.c
	../thirdparty/b64/encode.c
)

add_library(irc ${IRC_SOURCES} ${IRC_SOURCES_EXTRA})

find_package(GLIB COMPONENTS gobject REQUIRED)

target_include_directories(irc
	PUBLIC irc
	PUBLIC .
	PRIVATE ../thirdparty
	PRIVATE ../src # TODO: this must be removed
	PRIVATE ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(irc log)
