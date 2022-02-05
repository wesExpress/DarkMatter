project ( GLAD )

set(SOURCES src/glad.c)

if(WIN32)
	set(SOURCES ${SOURCES} src/glad_wgl.c)
endif()

if(UNIX)
	add_library( ${PROJECT_NAME} SHARED 
		${SOURCES}
	)
else()
	add_library( ${PROJECT_NAME} STATIC 
		${SOURCES}
	)
endif()

target_include_directories(GLAD 
	PUBLIC 
		include/ 
)

if(UNIX AND NOT APPLE)
	target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
endif()