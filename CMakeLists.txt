cmake_minimum_required( VERSION 3.16 )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)

set(CMAKE_BUILD_TYPE Debug)

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

project( CEngine )

if(WIN32)
    set(DM_WIN_OPENGL off)
    if(DM_WIN_OPENGL)
       add_definitions(-DDM_WIN_OPENGL) 
    endif()
endif()

if(UNIX OR APPLE OR DM_WIN_OPENGL)
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
    src/rendering/directx/dm_directx_buffer.c
    src/rendering/directx/dm_directx_shader.c
    src/rendering/directx/dm_directx_enum_conversion.c
)

if(WIN32 AND NOT DM_WIN_OPENGL)
    set(VERTEX_SHADERS
        shaders/hlsl/object_vertex.hlsl
    )
    set(PIXEL_SHADERS
        shaders/hlsl/object_pixel.hlsl
    )
else()
    set(VERTEX_SHADERS
        shaders/glsl/object_vertex.glsl
    )
    set(PIXEL_SHADERS
        shaders/glsl/object_pixel.glsl
    )
endif()

set(SHADERS ${VERTEX_SHADERS} ${PIXEL_SHADERS})

add_executable(${PROJECT_NAME} ${SOURCES} ${SHADERS})

target_include_directories(${PROJECT_NAME} PUBLIC src PRIVATE lib/glfw/include lib/glad/include)
if(UNIX OR APPLE OR DM_WIN_OPENGL)
    target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} GLAD)
elseif(WIN32 AND NOT DM_WIN_OPENGL)
    target_link_libraries(${PROJECT_NAME} 
    d3d11.lib
	dxgi.lib
	dxguid.lib
	d3dcompiler.lib)
endif()

if(BUILD_SHARED_LIBS)
    add_definitions(-DDMEXPORT)
endif()

# compile shaders for directx
if(WIN32 AND NOT DM_WIN_OPENGL)
    find_program(FXC fxc DOC "fx shader compiler")
    if(NOT FXC)
        message(FATAL_ERROR "FXC not found!")
    endif()
    set(FXC_FLAGS "/Fc" "/Od" "/Zi")
    
    set(current_output_path ${CMAKE_BINARY_DIR}/shaders/hlsl/)

    function(compile_hlsl shaders shader_entry shader_version)
        foreach(shader ${shaders})
            get_filename_component(file_we ${shader} NAME_WE)
            set(shader_out ${current_output_path}/${file_we}.fxc)
        
            add_custom_command(
                OUTPUT ${shader_out}
                COMMAND ${FXC} ${FXC_FLAGS} /E ${shader_entry} /T ${shader_version} ${CMAKE_CURRENT_SOURCE_DIR}/${shader} /Fo ${shader_out} 
                MAIN_DEPENDENCY ${shader}
                COMMENT "Compiling ${shader}..."
                VERBATIM
            )
            add_custom_target(fx_${file_we} ALL DEPENDS ${SHADER_OUT})
        endforeach()
    endfunction()

    # vertex shaders
    compile_hlsl(${VERTEX_SHADERS} v_main vs_5_0)

    # pixel shaders
    compile_hlsl(${PIXEL_SHADERS} p_main ps_5_0)
endif()

# move shaders to binary dir for now
add_custom_command(
    TARGET CEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_BINARY_DIR}/shaders
)