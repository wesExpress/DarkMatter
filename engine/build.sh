#!/bin/bash

set echo on

mkdir -p bin
mkdir -p bin/shaders/metal

cd engine

c_files=$(find src -type f -name "*.c")
objc_files=$(find src -type f -name "*.m")

# externals
external_files="lib/stb_image/src/stb_image.c lib/mt19937/src/mt19937.c lib/mt19937/src/mt19937_64.c"

echo "C Files: " $c_files
echo "ObjC Files: " $objc_files

output="DarkMatter"

compiler_flags="-g -shared -fdiagnostics-absolute-paths -fdeclspec -fPIC -Wall -Wno-missing-braces"
include_flags="-Isrc -Ilib/stb_image/include -Ilib/mt19937/include"
linker_flags="-lobjc -framework QuartzCore -framework Cocoa -framework CoreFoundation -framework Metal"
defines="-DDM_DEBUG"

echo "Building $output..."

clang $c_files $objc_files $external_files $compiler_flags -o ../bin/$output.so $defines $include_flags $linker_flags

# shaders
echo "Compiling object shader..."
xcrun -sdk macosx metal shaders/metal/object.metal -c -o ../bin/shaders/metal/object.air
xcrun -sdk macosx metallib ../bin/shaders/metal/object.air -o ../bin/shaders/metal/object.metallib

echo "Compiling light source shader..."
xcrun -sdk macosx metal shaders/metal/light_src.metal -c -o ../bin/shaders/metal/light_src.air
xcrun -sdk macosx metallib ../bin/shaders/metal/light_src.air -o ../bin/shaders/metal/light_src.metallib

