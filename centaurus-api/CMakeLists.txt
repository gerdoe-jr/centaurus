cmake_minimum_required(VERSION 3.18)
project(centaurus-api)

include_directories(centaurus)

set(SOURCES
	centaurus_api.cpp
    task.cpp
	worker.cpp
	job.cpp
	scheduler.cpp
	logger.cpp
)

set(PRECOMPILE_HEADERS
	centaurus_api_pch.h
)

set(HEADERS
	centaurus_api.h
    task.h
	worker.h
	job.h
	scheduler.h
	logger.h
)

find_package(Boost 1.75.0 REQUIRED COMPONENTS
    system filesystem thread
)

if (MSVC)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
endif()

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
link_libraries(${Boost_LIBRARIES})

add_library(centaurus-api SHARED ${SOURCES} ${HEADERS})
target_precompile_headers(centaurus-api PRIVATE ${PRECOMPILE_HEADERS})
target_link_libraries(centaurus-api PRIVATE cronos win32ctrl json)
target_compile_definitions(centaurus-api PRIVATE CENTAURUS_INTERNAL)
target_include_directories(centaurus-api PRIVATE centaurus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
