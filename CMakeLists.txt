cmake_minimum_required( VERSION 3.16 )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)

set(CMAKE_BUILD_TYPE Debug)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

project(Master)

set(DM_RENDER_OPENGL on)
set(DM_BUILD_DEBUG on)

if(DM_BUILD_DEBUG)
    add_definitions(-DDM_DEBUG)
else()
    add_definitions(-DDM_RELEASE)
endif()

add_subdirectory( engine )
add_subdirectory( application )