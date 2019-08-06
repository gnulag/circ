set(LOG_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/log.c
	${CMAKE_CURRENT_SOURCE_DIR}/log.h
)
set(LOG_SOURCES ${LOG_SOURCES} PARENT_SCOPE)

add_library(log ${LOG_SOURCES})

target_include_directories(log PUBLIC ..)
