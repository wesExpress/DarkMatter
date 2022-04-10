#!/bin/bash

set echo on
output="DarkMatter"

SRC_DIR=$PWD

mkdir -p build/engine

cd build/engine

c_files=$(find $SRC_DIR/engine/src -type f -name "*.c")

external_files="$SRC_DIR/engine/lib/stb_image/src/stb_image.c $SRC_DIR/engine/lib/mt19937/src/mt19937.c $SRC_DIR/engine/lib/mt19937/src/mt19937_64.c $SRC_DIR/engine/lib/glad/src/glad.c"

compiler_flags="-g -shared -std=gnu99 -fdiagnostics-absolute-paths -fdeclspec -fPIC -Wall -Wno-missing-braces"

defines="-DDM_DEBUG -DDM_EXPORT -DDM_OPENGL"

include_flags="-I$SRC_DIR/engine/src -I$SRC_DIR/engine/lib/stb_image/include -I$SRC_DIR/engine/lib/mt19937/include -I$SRC_DIR/engine/lib/glad/include"

linker_flags="-g -lX11 -lX11-xcb -lxkbcommon -lGL -L/usr/X11R6/lib -lm"

echo "Building $output..."
clang $c_files $objc_files $external_files $compiler_flags -o lib$output.so $defines $include_flags $linker_flags