cmake_minimum_required(VERSION 3.18)
project(centaurus)

set(HEADERS
    centaurus.h
    centaurus_api.h
    centaurus_bank.h
    centaurus_task.h
    centaurus_worker.h
    centaurus_export.h
    centaurus_loader.h
    centaurus_job.h
    centaurus_scheduler.h
)

set(SOURCES
    centaurus.cpp
    centaurus_api.cpp
    centaurus_bank.cpp
    centaurus_task.cpp
    centaurus_worker.cpp
    centaurus_export.cpp
    centaurus_loader.cpp
    centaurus_job.cpp
    centaurus_scheduler.cpp
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
