#!/bin/bash

set echo on

opengl=1

mkdir -p bin

cd engine

c_files=$(find src -type f -name "*.c")
objc_files=$(find src -type f -name "*.m")

# externals
if [ "$opengl" -eq 1 ]; then
	external_files="lib/stb_image/src/stb_image.c lib/mt19937/src/mt19937.c lib/mt19937/src/mt19937_64.c lib/glad/src/glad.c"
else
	external_files="lib/stb_image/src/stb_image.c lib/mt19937/src/mt19937.c lib/mt19937/src/mt19937_64.c"
fi

#echo "C Files: " $c_files
#echo "ObjC Files: " $objc_files

output="DarkMatter"

compiler_flags="-g -dynamiclib -std=gnu99 -fdiagnostics-absolute-paths -fdeclspec -fPIC -Wall -Wno-missing-braces"

if [ "$opengl" -eq 1 ]; then
	defines="-DDM_DEBUG -DDM_EXPORT -DDM_OPENGL"
	include_flags="-Isrc -Ilib/stb_image/include -Ilib/mt19937/include -Ilib/glad/include -Ilib/glfw/include"
	linker_flags="-lglfw -framework Cocoa -framework OpenGL -framework CoreVideo"
else
	linker_flags="-lobjc -framework QuartzCore -framework Cocoa -framework CoreFoundation -framework Metal"
	include_flags="-Isrc -Ilib/stb_image/include -Ilib/mt19937/include"
	defines="-DDM_DEBUG -DDM_EXPORT"
fi

echo "Building $output..."

clang $c_files $objc_files $external_files $compiler_flags -o ../bin/$output.dylib $defines $include_flags $linker_flags

# shaders
if [ "$opengl" -eq 1 ]; then
	mkdir -p ../bin/shaders/glsl

	cp shaders/glsl/object_vertex.glsl ../bin/shaders/glsl/
	cp shaders/glsl/object_pixel.glsl ../bin/shaders/glsl/
	cp shaders/glsl/light_src_vertex.glsl ../bin/shaders/glsl/
	cp shaders/glsl/light_src_pixel.glsl ../bin/shaders/glsl/
else
	mkdir -p ../bin/shaders/metal

	echo "Compiling object shader..."
	xcrun -sdk macosx metal shaders/metal/object.metal -c -o ../bin/shaders/metal/object.air
	xcrun -sdk macosx metallib ../bin/shaders/metal/object.air -o ../bin/shaders/metal/object.metallib

	echo "Compiling light source shader..."
	xcrun -sdk macosx metal shaders/metal/light_src.metal -c -o ../bin/shaders/metal/light_src.air
	xcrun -sdk macosx metallib ../bin/shaders/metal/light_src.air -o ../bin/shaders/metal/light_src.metallib
fi
