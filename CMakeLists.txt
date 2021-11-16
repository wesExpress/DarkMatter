cmake_minimum_required( VERSION 3.16 )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)

set(CMAKE_BUILD_TYPE Debug)

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

add_subdirectory(lib/glfw)

project( CEngine )

set(
    SOURCES
    
    src/main.c
    src/engine.c

    src/platform/platform_glfw.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src PRIVATE lib/glfw/include)
target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES})