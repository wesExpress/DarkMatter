cmake_minimum_required( VERSION 3.16 )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)

set(CMAKE_BUILD_TYPE Debug)

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

project( CEngine )

add_subdirectory(lib/glfw)
add_subdirectory(lib/glad)

add_definitions(-DDM_OPENGL -DDM_DEBUG)

set(BUILD_SHARED_LIBS TRUE)

set(
    SOURCES
    
    src/dm_engine_entry.c
    src/dm_engine.c
    src/dm_mem.c
    src/dm_logger.c
    src/dm_event.c
    src/dm_camera_controller.c

    src/input/dm_input.c

    src/platform/dm_platform_glfw.c

    src/rendering/dm_renderer.c
    src/rendering/dm_camera.c
    src/rendering/opengl/dm_opengl_renderer.c
    src/rendering/opengl/dm_opengl_shader.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src PRIVATE lib/glfw/include lib/glad/include)
target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} GLAD)

if(BUILD_SHARED_LIBS)
    add_definitions(-DDMEXPORT)
endif()

file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})