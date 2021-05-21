cmake_minimum_required(VERSION 3.18)
project(centaurus-dbms)

set(HEADERS
    centaurus_dbms.h
    cronos_api.h
    api.h
    bank.h
    fetch.h
    export.h
)

set(PRECOMPILE_HEADERS
    centaurus_dbms_pch.h
)

set(SOURCES
    centaurus_dbms.cpp
    cronos_api.cpp
    api.cpp
    bank.cpp
    fetch.cpp
    export.cpp
)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
else()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

add_library(centaurus-dbms SHARED ${SOURCES} ${HEADERS})
target_precompile_headers(centaurus-dbms PRIVATE ${PRECOMPILE_HEADERS})

find_package(Boost 1.75.0 REQUIRED COMPONENTS
    system filesystem thread
)

target_compile_definitions(centaurus-dbms PRIVATE CENTAURUS_DBMS)
target_include_directories(centaurus-dbms PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(centaurus-dbms PRIVATE ${Boost_LIBRARY_DIRS})
target_link_libraries(centaurus-dbms PRIVATE ${Boost_LIBRARIES})
target_link_libraries(centaurus-dbms PRIVATE centaurus-api cronos win32ctrl json)

target_include_directories(centaurus-dbms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})