cmake_minimum_required(VERSION 3.18)
project(cronos)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SOURCES
    croexception.cpp
    croentity.cpp
    crobuffer.cpp
    crodata.cpp
    crotable.cpp
    crostream.cpp
    crosync.cpp
    crofile.cpp
    cronos02.cpp
    cronos_abi.cpp
    croparser.cpp
    croentry.cpp
    croblock.cpp
    crorecord.cpp
    crostru.cpp
    crobank.cpp
    croreader.cpp
    croexport.cpp
)

set(HEADERS
    cronos_format.h
    crotype.h
    croexception.h
    croentity.h
    crobuffer.h
    crodata.h
    crotable.h
    crostream.h
    crosync.h
    crofile.h
    cronos02.h
    crodata.h
    cronos_abi.h
    croparser.h
    crotable.h
    croentry.h
    croblock.h
    crorecord.h
    crostru.h
    crobank.h
    croreader.h
    croexport.h
)

add_library(cronos ${SOURCES} ${HEADERS})
target_include_directories(cronos PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cronos PRIVATE
	blowfish win32ctrl
)

