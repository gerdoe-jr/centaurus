cmake_minimum_required(VERSION 3.18)
project(cronos_test)

add_executable(cronos_test test.cpp)
target_link_libraries(cronos_test PRIVATE
	cronos
    win32ctrl
)

install(TARGETS cronos_test
	RUNTIME DESTINATION ${CMAKE_BUILD_DIRECTORY}
)
