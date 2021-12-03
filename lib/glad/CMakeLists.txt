project ( GLAD )

add_library( ${PROJECT_NAME} STATIC src/glad.c )

target_include_directories(GLAD 
	PUBLIC 
		include/ 
)