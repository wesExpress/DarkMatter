cmake_minimum_required( VERSION 3.16 )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)

set(CMAKE_BUILD_TYPE Debug)

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

project( CEngine )

if(UNIX OR APPLE)
    add_subdirectory(lib/glfw)
    add_subdirectory(lib/glad)
endif()

add_definitions(-DDM_DEBUG)

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
    src/platform/dm_platform_win32.c

    src/rendering/dm_renderer.c
    src/rendering/dm_command_buffer.c
    src/rendering/dm_camera.c

    src/rendering/opengl/dm_opengl_renderer.c
    src/rendering/opengl/dm_opengl_buffer.c
    src/rendering/opengl/dm_opengl_shader.c
    src/rendering/opengl/dm_opengl_enum_conversion.c

    src/rendering/directx/dm_directx_renderer.c
    src/rendering/directx/dm_directx_device.c
    src/rendering/directx/dm_directx_swapchain.c
    src/rendering/directx/dm_directx_rendertarget.c
    src/rendering/directx/dm_directx_depth_stencil.c
    src/rendering/directx/dm_directx_enum_conversion.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src PRIVATE lib/glfw/include lib/glad/include)
if(UNIX OR APPLE)
    target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} GLAD)
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} 
    d3d11.lib
	dxgi.lib
	dxguid.lib
	d3dcompiler.lib)
endif()

if(BUILD_SHARED_LIBS)
    add_definitions(-DDMEXPORT)
endif()

# move shaders to binary dir for now
add_custom_command(
    TARGET CEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_BINARY_DIR}/shaders
)