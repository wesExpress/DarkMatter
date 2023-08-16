#! /bin/bash

set echo on
output="app"

vulkan=0
debug=1
simd_256=1
phys_simd=1
phys_multi_th=0

SRC_DIR=$PWD

mkdir -p build
cd build

c_files="$SRC_DIR/main.c $SRC_DIR/app.c $SRC_DIR/render_pass.c $SRC_DIR/debug_render_pass.c"
dm_files="$SRC_DIR/dm_impl.c $SRC_DIR/dm_platform_linux.c ${SRC_DIR}/dm_physics.c"

if ((vulkan)); then
	dm_files="$dm_files $SRC_DIR/dm_renderer_vulkan.c"
	defines="-DDM_VULKAN"
else
	dm_files="$dm_files $SRC_DIR/dm_renderer_opengl.c $SRC_DIR/lib/glad/src/glad.c"
	defines="-DDM_OPENGL"
fi

compiler_flags="-g -MD -std=gnu99 -fPIC -Wall -Wuninitialized -Wno-missing-braces -fno-stack-protector"

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
	defines="$defines -DDM_DEBUG"
	#compiler_flags="$compiler_flags -O0 -fsanitize=address -fno-omit-frame-pointer"
else
	compiler_flags="$compiler_flags -O2"
fi

include_flags="-I$SRC_DIR/ -I$SRC_DIR/lib"
if ((vulkan)); then
	include_flags="$include_flags -I$VULKAN_SDK/Include"
else
	include_flags="$include_flags -I$SRC_DIR/lib/glad/include"
fi

linker_flags="-lX11 -lX11-xcb -lxcb -lxkbcommon -L/usr/X11R6/lib -lm "
if ((vulkan)); then
	linker_flags="$linker_flags -L$VULKAN_SDK/Lib -lvulkan"
else
	linker_flags="$linker_flags -lGL"
fi

echo "Building $output..."
gcc $compiler_flags $c_files $dm_files -o $output $defines $include_flags $linker_flags

cd ..

# move assets
mkdir -p build/assets/shaders

for file in *.glsl; do
	if((vulkan)); then
		root=${file%.*}
		shader_type=${root: -5}
		output=$root.spv
		echo "Compiling shader: $file"
		if [[ "$shader_type" == "pixel" ]]; then
			shader_flags=-fshader-stage=frag
		else
			shader_flags=-fshader-stage=vert
		fi
		$VULKAN_SDK/bin/glslc $shader_flags $file -o $output
		mv $output build/assets/shaders
	else
		echo $file
		cp $file build/assets/shaders
	fi
done

mkdir -p build/assets/textures
cp "default_texture.png" build/assets/textures