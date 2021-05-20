cmake_minimum_required(VERSION 3.18)
project(centaurus-rpc)

link_libraries(centaurus)
include_directories(centaurus)

set(SOURCES
    rpc.cpp
)

set(HEADERS
    rpc.h
)

find_package(Boost 1.75.0 REQUIRED COMPONENTS
    system filesystem thread
)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
link_libraries(${Boost_LIBRARIES})

add_library(centaurus-rpc SHARED ${SOURCES} ${HEADERS})
target_compile_definitions(centaurus-rpc PRIVATE CENTAURUS_INTERNAL)
target_include_directories(centaurus-rpc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

#add_executable(centaurus-rpc-test centaurus-rpc-test.cpp)
include_directories(centaurus-rpc)
link_libraries(centaurus-rpc)

add_executable(centaurus-rpc-ws rpc-ws.cpp)
add_executable(centaurus-rpc-http-sync rpc-http-sync.cpp)
