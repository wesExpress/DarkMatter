cmake_minimum_required( VERSION 3.16 )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)

set(CMAKE_BUILD_TYPE Debug)

project( CEngine )

set(
    SOURCES
    main.c
)

add_executable(${PROJECT_NAME} ${SOURCES})