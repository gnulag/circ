set(IRC_SOURCES
	irc.c
	hooks.c
	ircium-parser/ircium-message.c
	ircium-parser/ircium-message-tag.c

	../thirdparty/b64/decode.c
	../thirdparty/b64/encode.c
)

add_library(irc ${IRC_SOURCES})

find_package(GLIB COMPONENTS gobject REQUIRED)

target_include_directories(irc
	PUBLIC irc
	PUBLIC .
	PRIVATE ../thirdparty
	PRIVATE ../src # TODO: this must be removed
	PRIVATE ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(irc log)
