cmake_minimum_required(VERSION 3.18)
project(centaurus)

set(HEADERS
    centaurus.h
    api.h
    bank.h
    task.h
    worker.h
    loader.h
    fetch.h
    export.h
    job.h
    scheduler.h
)

set(SOURCES
    centaurus.cpp
    api.cpp
    bank.cpp
    task.cpp
    worker.cpp
    loader.cpp
    fetch.cpp
    export.cpp
    job.cpp
    scheduler.cpp
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

target_include_directories(centaurus PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(centaurus PRIVATE ${Boost_LIBRARY_DIRS})
target_link_libraries(centaurus PRIVATE ${Boost_LIBRARIES})

target_compile_definitions(centaurus PRIVATE CENTAURUS_INTERNAL)
target_link_libraries(centaurus PRIVATE cronos win32ctrl json)

target_include_directories(centaurus PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
