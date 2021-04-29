cmake_minimum_required(VERSION 3.18)
project(centaurus-run)

set(SOURCES
    centaurus-run.cpp
)

add_executable(centaurus-run ${SOURCES})

set_target_properties(centaurus-run PROPERTIES
    OUTPUT_NAME "centaurus-exe"
)

target_link_libraries(centaurus-run
    PRIVATE centaurus win32ctrl
)
