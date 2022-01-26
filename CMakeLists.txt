cmake_minimum_required( VERSION 3.16 )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)

set(CMAKE_BUILD_TYPE Debug)

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

project( CEngine )

set(DM_RENDER_OPENGL on)
set(DM_BUILD_DEBUG on)

if(WIN32)
    if(DM_RENDER_OPENGL)
       add_definitions(-DDM_WIN_OPENGL) 
    endif()
endif()

if(APPLE)
    if(DM_RENDER_OPENGL)
        add_definitions(-DDM_APPLE_OPENGL)
    endif()
endif()

if(DM_RENDER_OPENGL)
    add_subdirectory(lib/glfw)
    add_subdirectory(lib/glad)
endif()

if(DM_BUILD_DEBUG)
    add_definitions(-DDM_DEBUG)
else()
    add_definitions(-DDM_RELEASE)
endif()

set(BUILD_SHARED_LIBS TRUE)

set(
    SOURCES
    
    src/core/dm_engine_entry.c
    src/core/dm_engine.c
    src/core/dm_mem.c
    src/core/dm_logger.c
    src/core/dm_event.c
    src/core/dm_string.c

    src/core/dm_camera_controller.c

    src/input/dm_input.c

    src/platform/dm_platform_glfw.c
    src/platform/dm_platform_win32.c

    src/structures/dm_list.c
    src/structures/dm_map.c

    src/rendering/dm_renderer.c
    src/rendering/dm_command_buffer.c
    src/rendering/dm_texture.c
    src/rendering/dm_camera.c

    src/rendering/opengl/dm_opengl_renderer.c
    src/rendering/opengl/dm_opengl_buffer.c
    src/rendering/opengl/dm_opengl_shader.c
    src/rendering/opengl/dm_opengl_enum_conversion.c
    src/rendering/opengl/dm_opengl_texture.c

    src/rendering/directx/dm_directx_renderer.c
    src/rendering/directx/dm_directx_device.c
    src/rendering/directx/dm_directx_swapchain.c
    src/rendering/directx/dm_directx_rendertarget.c
    src/rendering/directx/dm_directx_depth_stencil.c
    src/rendering/directx/dm_directx_buffer.c
    src/rendering/directx/dm_directx_shader.c
    src/rendering/directx/dm_directx_texture.c
    src/rendering/directx/dm_directx_enum_conversion.c

    # stb image
    lib/stb_image/include/stb_image/stb_image.c
)

# objective c files if using metal
if(APPLE AND NOT DM_RENDER_OPENGL)
    set(SOURCES ${SOURCES} 
        src/platform/dm_platform_apple.m

        src/rendering/metal/dm_metal_renderer.m
    )
endif()

if(WIN32 AND NOT DM_RENDER_OPENGL)
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

set(INCLUDES PUBLIC src PRIVATE lib/stb_image/include)

if(DM_RENDER_OPENGL)
    set(INCLUDES ${INCLUDES} PRIVATE lib/glfw/include lib/glad/include)
endif()

target_include_directories(${PROJECT_NAME} ${INCLUDES})

if(DM_RENDER_OPENGL)
    target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} GLAD)
elseif(WIN32 AND NOT DM_RENDER_OPENGL)
    target_link_libraries(${PROJECT_NAME} d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib)
elseif(APPLE AND NOT DM_RENDER_OPENGL)
    find_library(CORE_FRAMEWORK CoreFoundation)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(METAL Metal)
    find_library(QUARTZ_CORE QuartzCore)
    target_link_libraries(${PROJECT_NAME} ${CORE_FRAMEWORK} ${COCOA_LIBRARY} ${METAL} ${QUARTZ_CORE})
endif()

if(BUILD_SHARED_LIBS)
    add_definitions(-DDMEXPORT)
endif()

# compile shaders for directx
if(WIN32 AND NOT DM_RENDER_OPENGL)
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

# move assets to binary dir
add_custom_command(
    TARGET CEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_CURRENT_BINARY_DIR}/assets
)