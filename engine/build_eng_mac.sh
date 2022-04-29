#!/bin/bash

set echo on
output="DarkMatter"

opengl=0

SRC_DIR=$PWD
#echo "$SRC_DIR"

mkdir -p build/engine

cd build/engine/

c_files=$(find $SRC_DIR/engine/src -type f -name "*.c")
objc_files=$(find $SRC_DIR/engine/src -type f -name "*.m")

#echo "C Files: " $c_files
#echo "ObjC Files: " $objc_files

external_files="$SRC_DIR/engine/lib/stb_image/src/stb_image.c $SRC_DIR/engine/lib/mt19937/src/mt19937.c $SRC_DIR/engine/lib/mt19937/src/mt19937_64.c"

if [ "$opengl" -eq 1 ]; then
	external_files="${external_files} $SRC_DIR/engine/lib/glad/src/glad.c"
fi

compiler_flags="-g -dynamiclib -std=gnu99 -fdiagnostics-absolute-paths -fdeclspec -fPIC -Wall -Wno-missing-braces -install_name @rpath/engine/lib$output.dylib"

defines="-DDM_DEBUG -DDM_EXPORT"

include_flags="-I$SRC_DIR/engine/src -I$SRC_DIR/engine/lib/stb_image/include -I$SRC_DIR/engine/lib/mt19937/include
-I$SRC_DIR/engine/lib/cgltf"

linker_flags="-g -framework Cocoa"

if [ "$opengl" -eq 1 ]; then
	defines="${defines} -DDM_OPENGL"
	include_flags="${include_flags} -I$SRC_DIR/engine/lib/glad/include -I$SRC_DIR/engine/lib/glfw/include"
	linker_flags="${linker_flags} -lglfw -framework OpenGl -framework CoreVideo"
else
	linker_flags="${linkerflags} -lobjc -framework QuartzCore -framework CoreFoundation -framework Cocoa -framework Metal"
fi

echo "Building $output..."
clang $c_files $objc_files $external_files $compiler_flags -o lib$output.dylib $defines $include_flags $linker_flags

cd ..
# shaders
if [ "$opengl" -eq 0 ]; then
	mkdir -p ../bin/shaders/metal

	echo "Compiling material shader..."
	xcrun -sdk macosx metal $SRC_DIR/engine/shaders/metal/material.metal -c -o $SRC_DIR/engine/shaders/metal/material.air
	xcrun -sdk macosx metallib $SRC_DIR/engine/shaders/metal/material.air -o $SRC_DIR/engine/shaders/metal/material.metallib

	echo "Compiling material color shader..."
	xcrun -sdk macosx metal $SRC_DIR/engine/shaders/metal/material_color.metal -c -o $SRC_DIR/engine/shaders/metal/material_color.air
	xcrun -sdk macosx metallib $SRC_DIR/engine/shaders/metal/material_color.air -o $SRC_DIR/engine/shaders/metal/material_color.metallib

	echo "Compiling light source shader..."
	xcrun -sdk macosx metal $SRC_DIR/engine/shaders/metal/light_src.metal -c -o $SRC_DIR/engine/shaders/metal/light_src.air
	xcrun -sdk macosx metallib $SRC_DIR/engine/shaders/metal/light_src.air -o $SRC_DIR/engine/shaders/metal/light_src.metallib

	echo "Compiling test shader..."
	xcrun -sdk macosx metal $SRC_DIR/engine/shaders/metal/test.metal -c -o $SRC_DIR/engine/shaders/metal/test.air
	xcrun -sdk macosx metallib $SRC_DIR/engine/shaders/metal/test.air -o $SRC_DIR/engine/shaders/metal/test.metallib
fi
