cmake_minimum_required(VERSION 3.18)
project(centaurus)

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(BOOST_INCLUDEDIR "C:\\Boost\\include\\boost-1_75")
set(BOOST_LIBRARYDIR "C:\\Boost\\lib")
set(BOOST_ROOT "C:\\Boost\\include\\boost-1_75\\boost")

add_definitions( -DBOOST_ALL_NO_LIB )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE EQUAL Release)
    if(MSVC)
        add_compile_options(/O2)
        add_link_options(/O2)
    else()
        add_compile_options(-O3)
        add_link_options(-O3)
    endif()
endif()

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
    add_compile_options(/utf-8)
    add_compile_options(/wd26812 /wd6258 /wd28251)
endif()

add_subdirectory(json)
add_subdirectory(win32ctrl)
add_subdirectory(blowfish)
add_subdirectory(cronos)

add_subdirectory(centaurus)
add_subdirectory(centaurus-ex)

#add_subdirectory(centaurus-rpc)

#install(TARGETS worker win32ctrl autoreader
#    RUNTIME DESTINATION ${CMAKE_BUILD_DIRECTORY}
#)
