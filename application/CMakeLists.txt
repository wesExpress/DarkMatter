project(Application)

set(SOURCES
	src/entry.c
	src/application.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} CEngine)

# move assets to binary dir
add_custom_command(
    TARGET Application POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/application/assets
            ${CMAKE_CURRENT_BINARY_DIR}/application/assets
)