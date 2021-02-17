cmake_minimum_required(VERSION 3.18)
project(centaurus-rpc)

set(SOURCES
    centaurus-rpc.cpp
)

set(HEADERS
    centaurus-rpc.h
)

add_library(centaurus-rpc ${SOURCES} ${HEADERS})
target_include_directories(centaurus-rpc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Boost 1.75.0 COMPONENTS system)
if(Boost_FOUND)
    target_include_directories(centaurus-rpc PUBLIC
        ${Boost_INCLUDE_DIRS})
    target_link_libraries(centaurus-rpc PUBLIC
        ${Boost_LIBRARY_DIRS})
else()
    message("Boost NOT FOUND")
endif()

add_executable(centaurus-rpc-test centaurus-rpc-test.cpp)
target_link_libraries(centaurus-rpc-test PRIVATE
    centaurus-rpc
)