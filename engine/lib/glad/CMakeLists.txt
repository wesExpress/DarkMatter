project ( GLAD )

set(SOURCES src/glad.c)

if(WIN32)
	set(SOURCES ${SOURCES} src/glad_wgl.c)
endif()

add_library( ${PROJECT_NAME} STATIC 
	${SOURCES}
)

target_include_directories(GLAD 
	PUBLIC 
		include/ 
)