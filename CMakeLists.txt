cmake_minimum_required(VERSION 3.18)
project(centaurus)

set(CENTAURUS centaurus CACHE STRING "Centaurus API")
set(CENTAURUS_EXE centaurus-run CACHE STRING "Centaurus UI/CLI")
set(CENTAURUS_RPC centaurus-rpc CACHE STRING "Centaurus RPC")

set(CENTAURUS_LIBS json win32ctrl blowfish cronos)
set(CENTAURUS_TARGETS ${CENTAURUS} ${CENTAURUS_RPC} ${CENTAURUS_EXE})
set(CENTAURUS_ALL ${CENTAURUS_LIBS} ${CENTAURUS_TARGETS})

set(CENTAURUS_DEBUG FALSE)
set(CENTAURUS_RELEASE FALSE)

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

add_definitions(-DBOOST_ALL_NO_LIB)
set(CMAKE_MODULE_PATH CMAKE_MODULE_PATH BOOST_ROOT "${PROJECT_SOURCE_DIR}/boost-libs")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CENTAURUS_DEBUG TRUE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CENTAURUS_RELEASE TRUE)
endif()

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_compile_options(/utf-8)
    add_compile_options(/wd26812 /wd6258 /wd28251)
endif()

if(${CENTAURUS_RELEASE})
    if(MSVC)
        add_compile_options(/O2)
        add_link_options(/O2)
    else()
        add_compile_options(-O3)
        add_link_options(-O3)
    endif()
endif()

foreach(target ${CENTAURUS_ALL})
    add_subdirectory(${target})
    if(${CENTAURUS_DEBUG})
        target_compile_definitions(${target} PRIVATE CENTAURUS_DEBUG)
    elseif(${CENTAURUS_RELEASE})
        target_compile_definitions(${target} PRIVATE CENTAURUS_RELEASE)
    endif()
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}

        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}
    )
endforeach()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
