cmake_minimum_required(VERSION 3.18)
project(cronos)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

add_compile_options(-O3)
add_link_options(-O3)

add_subdirectory(worker)
add_subdirectory(win32ctrl)
add_subdirectory(json)

add_subdirectory(autoreader)

add_subdirectory(blowfish)
add_subdirectory(cronos)
add_subdirectory(cronos_test)

install(TARGETS worker win32ctrl autoreader
    RUNTIME DESTINATION ${CMAKE_BUILD_DIRECTORY}
)
