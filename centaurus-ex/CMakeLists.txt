cmake_minimum_required(VERSION 3.18)
project(centaurus-ex)

set(SOURCES
    centaurus-ex.cpp
)

add_executable(centaurus-exe ${SOURCES})

target_link_libraries(centaurus-exe
    PRIVATE centaurus win32ctrl
)
