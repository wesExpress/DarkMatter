project(Application)

set(SOURCES
	src/entry.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} CEngine)