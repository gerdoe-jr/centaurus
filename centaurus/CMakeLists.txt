cmake_minimum_required(VERSION 3.18)
project(centaurus)

set(HEADERS
    centaurus.h
)

set(SOURCES
    centaurus.cpp
)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 0)
else()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

add_library(centaurus SHARED ${SOURCES} ${HEADERS})

find_package(Boost 1.75.0 REQUIRED COMPONENTS
    system filesystem thread
)

target_compile_definitions(centaurus PRIVATE CENTAURUS_DLL)
target_include_directories(centaurus PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(centaurus PRIVATE ${Boost_LIBRARY_DIRS})
target_link_libraries(centaurus PRIVATE ${Boost_LIBRARIES})
target_link_libraries(centaurus PRIVATE centaurus-api centaurus-dbms)

target_include_directories(centaurus PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
