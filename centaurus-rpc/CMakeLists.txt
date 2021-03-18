cmake_minimum_required(VERSION 3.18)
project(centaurus-rpc)

set(SOURCES
    centaurus-rpc.cpp
)

set(HEADERS
    centaurus-rpc.h
)

find_package(Boost 1.75.0 REQUIRED COMPONENTS
    system thread json
)

add_library(centaurus-rpc ${SOURCES} ${HEADERS})
target_include_directories(centaurus-rpc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(centaurus-rpc PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(centaurus-rpc PRIVATE ${Boost_LIBRARY_DIRS})
target_link_libraries(centaurus-rpc PRIVATE ${Boost_LIBRARIES})

add_executable(centaurus-rpc-test centaurus-rpc-test.cpp)
target_link_libraries(centaurus-rpc-test PRIVATE
    centaurus-rpc
)

target_include_directories(centaurus-rpc-test PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(centaurus-rpc-test PRIVATE ${Boost_LIBRARY_DIRS})
target_link_libraries(centaurus-rpc-test PRIVATE ${Boost_LIBRARIES})
