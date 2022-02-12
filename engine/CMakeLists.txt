set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

project( DarkMatter )

if(UNIX AND NOT APPLE)
    set(DM_RENDER_OPENGL on)
endif()

# set up external libraries as needed
# we do not use opengl on mac
if(DM_RENDER_OPENGL)
    if(APPLE)
        set(DM_RENDER_OPENGL off)
    else()
        add_definitions(-DDM_OPENGL)
        add_subdirectory(lib/glad)
    endif()
endif()

if((NOT WIN32) AND (NOT UNIX) AND (NOT APPLE))
    add_subdirectory(lib/glfw)
endif()

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
    set(BUILD_SHARED_LIBS on)
endif()

# source files
# we include opengl and directx without bothering to check which renderer
# because there are checks that block off the entire file if not needed
set(
    SOURCES
    
    src/core/dm_engine.c
    src/core/dm_mem.c
    src/core/dm_logger.c
    src/core/dm_event.c
    src/core/dm_string.c

    src/core/math/dm_math_misc.c
    src/core/math/dm_math_vec2.c
    src/core/math/dm_math_vec3.c
    src/core/math/dm_math_vec4.c
    src/core/math/dm_math_quat.c
    src/core/math/dm_math_mat2.c
    src/core/math/dm_math_mat3.c
    src/core/math/dm_math_mat4.c
    src/core/math/dm_math_mat_transforms.c

    src/input/dm_input.c

    src/platform/dm_platform_glfw.c
    src/platform/dm_platform_win32.c
    src/platform/dm_platform_linux.c

    src/structures/dm_list.c
    src/structures/dm_map.c

    src/rendering/dm_renderer.c
    src/rendering/dm_renderer_api.c
    src/rendering/dm_command_buffer.c
    src/rendering/dm_image.c
    src/rendering/dm_geometry.c
    src/rendering/dm_camera.c
    src/rendering/dm_uniform.c

    src/rendering/opengl/dm_opengl_renderer.c
    src/rendering/opengl/dm_opengl_render_pass.c
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

# objective c files
# can't include these on windows because it has no idea what they are
if(APPLE)
    set(OBJC_SOURCES 
        src/platform/dm_platform_apple.m
        src/rendering/metal/dm_metal_renderer.m
        src/rendering/metal/dm_metal_buffer.m
        src/rendering/metal/dm_metal_texture.m
        src/rendering/metal/dm_metal_shader.m
        src/rendering/metal/dm_metal_view.m
        src/rendering/metal/dm_metal_pipeline.m
        src/rendering/metal/dm_metal_render_pass.m
    )

    set(SOURCES ${SOURCES} ${OBJC_SOURCES})
endif()

# headers
set(PRIVATE_H lib/stb_image/include)
set(PUBLIC_H src include)

if((NOT WIN32) AND (NOT UNIX) AND (NOT APPLE))
    set(PRIVATE_H ${PRIVATE_H} lib/glfw/include)
endif()

if(DM_RENDER_OPENGL)
    set(PRIVATE_H ${PRIVATE_H} lib/glad/include)
endif()

##################
# actual library #
##################
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${SHADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${PUBLIC_H} PRIVATE ${PRIVATE_H})
 
# internal libraries
if(WIN32)
    if(DM_RENDER_OPENGL)
        target_link_libraries(${PROJECT_NAME} GLAD Opengl32.lib)
    else()
        target_link_libraries(${PROJECT_NAME} d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib)
    endif()
elseif(APPLE)
    find_library(CORE_FRAMEWORK CoreFoundation)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(METAL Metal)
    find_library(QUARTZ_CORE QuartzCore)
    target_link_libraries(${PROJECT_NAME} ${CORE_FRAMEWORK} ${COCOA_LIBRARY} ${METAL} ${QUARTZ_CORE})
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig)
    PKG_CHECK_MODULES(PKG_X11_XCB QUIET x11-xcb)

    set(X11_XCB_DEFINITIONS ${PKG_X11_XCB_CFLAGS})

    find_path(X11_XCB_INCLUDE_DIR NAMES X11/Xlib-xcb.h HINTS ${PKG_X11_XCB_INCLUDE_DIRS})
    find_library(X11_XCB_LIBRARIES NAMES X11-xcb HINTS ${PKG_X11_XCB_LIBRARY_DIRS})
    
    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(X11_XCB DEFAULT_MSG X11_XCB_LIBRARIES X11_XCB_INCLUDE_DIR)

    mark_as_advanced(X11_XCB_INCLUDE_DIR X11_XCB_LIBRARIES)

    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${X11_LIBRARIES} ${X11_XCB_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIRS} ${X11_XCB_INCLUDE_DIR})

    target_link_libraries(${PROJECT_NAME} m)

    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
    include_directories(${OPENGL_INCLUDE_DIR})

    target_link_libraries(${PROJECT_NAME} GLAD)
else()
    target_link_libraries(${PROJECT_NAME} glfw GLAD)
endif()

if(BUILD_SHARED_LIBS)
    add_definitions(-DDM_EXPORT)
endif()

# shader
if(DM_RENDER_OPENGL)
    set(VERTEX_SHADERS
        shaders/glsl/object_vertex.glsl
        shaders/glsl/light_src_vertex.glsl
    )
    set(PIXEL_SHADERS
        shaders/glsl/object_pixel.glsl
        shaders/glsl/light_src_pixel.glsl
    )

    set(SHADERS ${VERTEX_SHADERS} ${PIXEL_SHADERS})
elseif(WIN32)
    set(VERTEX_SHADERS
        shaders/hlsl/object_vertex.hlsl
        shaders/hlsl/light_src_vertex.hlsl
    )
    set(PIXEL_SHADERS
        shaders/hlsl/object_pixel.hlsl
        shaders/hlsl/light_src_pixel.hlsl
    )    

    set(SHADERS ${VERTEX_SHADERS} ${PIXEL_SHADERS})
elseif(APPLE)
    set(SHADERS
        shaders/metal/object.metal
    )
endif()

######################
# shader compilation #
######################

# HLSL
if(WIN32 AND NOT DM_RENDER_OPENGL)
    find_program(FXC fxc DOC "fx shader compiler")
    if(NOT FXC)
        message(FATAL_ERROR "FXC not found!")
    endif()
    set(FXC_FLAGS "/Fc" "/Od" "/Zi")
    
    set(current_output_path ${CMAKE_BINARY_DIR}/shaders/hlsl)

    function(compile_hlsl shader shader_entry shader_version)
        get_filename_component(file_we ${shader} NAME_WE)
        set(shader_out ${current_output_path}/${file_we}.fxc)
        add_custom_command(
            OUTPUT ${shader_out}
            COMMAND ${FXC} ${FXC_FLAGS} /E ${shader_entry} /T ${shader_version} ${CMAKE_CURRENT_SOURCE_DIR}/${shader} /Fo ${shader_out} 
            MAIN_DEPENDENCY ${shader}
            COMMENT "Compiling ${shader}..."
            VERBATIM
        )
        add_custom_target(fx_${file_we} DEPENDS ${shader_out})
        add_dependencies(${PROJECT_NAME} fx_${file_we})
    endfunction()

    # vertex shaders
    foreach(shader ${VERTEX_SHADERS})
        compile_hlsl(${shader} v_main vs_5_0)
    endforeach()

    # pixel shaders
    foreach(shader ${PIXEL_SHADERS})
        compile_hlsl(${shader} p_main ps_5_0)
    endforeach()
    
# metal shaders
elseif(APPLE)    
    set(current_output_path ${CMAKE_BINARY_DIR}/shaders/metal)
    
    foreach(shader ${SHADERS})
        get_filename_component(file_we ${shader} NAME_WE)

        set(current_output_path ${CMAKE_CURRENT_SOURCE_DIR}/shaders/metal)
        set(shader_intermediate ${current_output_path}/${file_we}.air)
        set(shader_out ${current_output_path}/${file_we}.metallib)

        add_custom_command(
            OUTPUT ${shader_intermediate}
            COMMAND xcrun -sdk macosx metal ${CMAKE_CURRENT_SOURCE_DIR}/${shader} -c -o ${shader_intermediate}
            MAIN_DEPENDENCY ${shader}
            COMMENT "Compiling intermediate metal shader: ${shader}..."
            VERBATIM
        )
        add_custom_target(metal_first DEPENDS ${shader_intermediate})
        add_dependencies(${PROJECT_NAME} metal_first)

        add_custom_command(
            OUTPUT ${shader_out}
            COMMAND xcrun -sdk macosx metallib ${shader_intermediate} -o ${shader_out}
            MAIN_DEPENDENCY ${shader_intermediate}
            COMMENT "Compiling metal shader lib: ${shader_intermediate}"
            VERBATIM
        )
        add_custom_target(metal_second DEPENDS ${shader_out})
        add_dependencies(${PROJECT_NAME} metal_second)

    endforeach()
endif()

# move shaders to binary dir for now
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Moving shaders..."
)

if(BUILD_SHARED_LIBS)
    add_definitions(-DDM_EXPORT)
endif()