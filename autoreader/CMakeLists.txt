cmake_minimum_required(VERSION 3.18)
project(autoreader)

add_definitions(-DUNICODE)

set(SOURCES
	main.cpp
	croreader.cpp
)

set(HEADERS
	croreader.h
)

add_executable(autoreader WIN32 ${SOURCES} ${HEADERS})
target_link_libraries(autoreader PRIVATE
	comctl32

	worker
	win32ctrl
	json
)
