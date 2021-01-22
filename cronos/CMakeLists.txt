cmake_minimum_required(VERSION 3.18)
project(cronos)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
	cronos.cpp
    cronos02.cpp
)

set(HEADERS
	cronos.h
    cronos02.h
)

add_library(cronos ${SOURCES} ${HEADERS})
target_include_directories(cronos PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cronos PRIVATE
	blowfish
)

