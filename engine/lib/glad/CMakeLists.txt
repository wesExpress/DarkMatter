project ( GLAD )

add_library( ${PROJECT_NAME} SHARED src/glad.c )

target_include_directories(GLAD 
	PUBLIC 
		include/ 
)