cmake_minimum_required(VERSION 3.18)
project(cronos)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    cronos02.cpp
    croexception.cpp
    croentity.cpp
    crobuffer.cpp
    crodata.cpp
    crotable.cpp
    crofile.cpp
)

set(HEADERS
    cronos_format.h
    cronos02.h
    crotype.h
    croexception.h
    croentity.h
    crobuffer.h
    crodata.h
    crotable.h
    crofile.h
)

add_library(cronos ${SOURCES} ${HEADERS})
target_include_directories(cronos PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cronos PRIVATE
	blowfish
)

