cmake_minimum_required(VERSION 3.18)
project(json)

set(SOURCES
	json_file.cpp
)

set(HEADERS
	json.hpp
	json_file.h
)

add_library(json STATIC ${SOURCES} ${HEADERS})
target_include_directories(json PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)