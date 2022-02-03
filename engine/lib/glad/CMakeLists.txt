project ( GLAD )

set(SOURCES src/glad.c)

if(WIN32)
	set(SOURCES ${SOURCES} src/glad_wgl.c)
endif()

add_library( ${PROJECT_NAME} SHARED 
	${SOURCES}
)

target_include_directories(GLAD 
	PUBLIC 
		include/ 
)

if(UNIX AND NOT APPLE)
	target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
endif()