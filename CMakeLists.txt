cmake_minimum_required(VERSION 3.24)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(DarkMatter)

set(SOURCES dm.c dm_glfw_window.c)

if(APPLE)
    find_library(APPLE_FWK_COCOA Cocoa REQUIRED)
    find_library(APPLE_FWK_METAL Metal REQUIRED)
    find_library(APPLE_FWK_QUARTZ_CORE QuartzCore REQUIRED)
    find_library(APPLE_FWK_FOUNDATION Foundation REQUIRED)
    find_library(APPLE_FWK_APP_KIT AppKit REQUIRED)

    enable_language(OBJC)
    set(SOURCES ${SOURCES} dm_metal_renderer.m)

    add_definitions(-DDM_METAL)
else()
    find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)

    set(SOURCES ${SOURCES} dm_vulkan_renderer.c vk_mem_alloc.cpp)

    add_definitions(-DDM_VULKAN)
endif()

add_subdirectory(lib/glfw)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC lib lib/glfw/include)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${APPLE_FWK_COCOA} ${APPLE_FWK_METAL} ${APPLE_FWK_QUARTZ_CORE} ${APPLE_FWK_FOUNDATION} ${APPLE_FWK_APP_KIT})
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan Vulkan::volk shaderc_combined SPIRV-Tools SPIRV-Tools-opt glslang)
endif()
