@echo off
SetLocal EnableDelayedExpansion

SET SRC_DIR=%cd%

set /A vulkan=0
SET /A debug=1
SET /A simd_256=1
SET /A phys_simd=1
SET /A phys_multi_th=0

SET c_filenames=%SRC_DIR%\main.c %SRC_DIR%\app.c %SRC_DIR%\render_pass.c
SET linker_flags=/link user32.lib gdi32.lib
SET include_flags=/I%SRC_DIR%\lib
SET compiler_flags=/arch:AVX2 /Wall /WL /TC /std:c99 /RTCsu

IF /I "%simd_256%" EQU "1" (
	SET defines="/DDM_SIMD_256"
)

IF /I "%phys_simd%" EQU "1" (
	SET defines=%defines% /DDM_PHYSICS_SIMD
)

IF /I "%phys_multi_th%" EQU "1" (
	SET defines=%defines% /DDM_PHYSICS_MULTI_TH
)

IF /I "%debug%" EQU "1" (
	SET defines=%defines% /DDM_DEBUG
	SET compiler_flags=/W2 /Z7 /Od /Ob0
) ELSE (
	SET defines=%defines% /DDM_RELEASE
	SET compiler_flags=/Zi /O2 /Ob2 /DEBUG
)

IF /I "%vulkan%" EQU "1" (
	SET include_flags=%include_flags% /I%VULKAN_SDK%\Include
	SET linker_flags=%linker_flags% /LIBPATH:%VULKAN_SDK%\Lib vulkan-1.lib
	SET defines=%defines% /DDM_VULKAN
) ELSE (
	SET c_filenames=!c_filenames:%cd%\lib\glad\src\glad.c=!
	SET c_filenames=!c_filenames:%cd%\lib\glad\src\glad_wgl.c=!
	SET linker_flags=%linker_flags% d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib
)

SET assembly=app

if not exist "build" mkdir build
cd build
ECHO Building %assembly%...
cl %compiler_flags% %defines% /FC %include_flags% %c_filenames% /Fe%assembly% %linker_flags%

cd ..
IF NOT EXIST "build\assets\shaders" mkdir build\assets\shaders

IF /I "%vulkan%" EQU "1" (
	FOR /R %%f IN (*.glsl) DO (
		SET fname=%%f
		SET root=!fname:~0,-5!
		SET output=!root!.spv
		SET shader_type=!root:~-5!

		ECHO Compiling shader: !fname!
		IF /I "!shader_type!" EQU "pixel" (
			SET shader_flags=-fshader-stage=frag
		) ELSE (
			SET shader_flags=-fshader-stage=vert
		)

		%VULKAN_SDK%\bin\glslc !shader_flags! !fname! -o !output!
		MOVE !output! build/assets/shaders
	)
) ELSE (
	FOR /R %%f IN (*.hlsl) DO (
		SET fname=%%f
		SET root=!fname:~0,-5!
		SET output=!root!.fxc
		SET shader_type=!root:~-5!

		ECHO Compiling shader: !fname!
		IF /I "!shader_type!" EQU "pixel" (
			SET shader_flags=/E p_main /T ps_5_0
		) ELSE (
			SET shader_flags=/E v_main /T vs_5_0
		)
		ECHO !shader_flags!

		fxc %fxc_flags% !shader_flags! !fname! /Fo !output!

		MOVE !output! build/assets/shaders
	)
)