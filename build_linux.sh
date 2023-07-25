#! /bin/bash

set echo on
output="app"

debug=1
simd_256=1
phys_simd=1
phys_multi_th=0

SRC_DIR=$PWD

mkdir -p build
cd build

c_files="$SRC_DIR/main.c $SRC_DIR/app.c $SRC_DIR/render_pass.c"
external_files="$external_files $SRC_DIR/lib/glad/src/glad.c"

compiler_flags="-g -fPIC -MD -std=gnu99 -fdiagnostics-absolute-paths -fPIC -Wall -Wno-missing-braces"
defines="-DDM_OPENGL"

if ((simd_256)); then
	defines="$defines -DDM_SIMD_256"
	compiler_flags="$compiler_flags -msse4.2 -mavx2 -mfma"
else
	compiler_flags="$compiler_flags -msse4.1"
fi

if ((phys_simd)); then
	defines="$defines -DDM_PHYSICS_SIMD"
fi

if ((phys_multi_th)); then
	defines="$defines -DDM_PHYSICS_MULTI_TH"
fi

if ((debug)); then
	defines="-DDM_DEBUG $defines"
	compiler_flags="-O0 $compiler_flags"
else
	compiler_flags="-O2 $compiler_flags"
fi

include_flags="-I$SRC_DIR/ -I$SRC_DIR/lib -I$SRC_DIR/lib/glad/include"

linker_flags="-g -lX11 -lX11-xcb -lxcb -lxkbcommon -lGL -L/usr/X11R6/lib -lm -ldl -pthread"

echo "Building $output..."
clang $c_files $compiler_flags $external_files -o $output $defines $include_flags $linker_flags

cd ..

# move assets
mkdir -p build/assets/shaders
cp test_vertex.glsl build/assets/shaders
cp test_pixel.glsl  build/assets/shaders
