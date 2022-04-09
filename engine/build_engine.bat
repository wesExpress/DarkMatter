@echo off
SetLocal EnableDelayedExpansion

set SRC_DIR=%cd%

set "opengl=true"

cd engine/src

SET c_filenames=
FOR /R %%f in (*.c) do (
	SET c_filenames=!c_filenames! %%f
	REM ECHO %%f
)

cd ../..

	SET extern_files=%SRC_DIR%\engine\lib\stb_image\src\stb_image.c %SRC_DIR%\engine\lib\mt19937\src\mt19937.c %SRC_DIR%\engine\lib\mt19937\src\mt19937_64.c 
	SET include_flags=/I%SRC_DIR%\engine\src /I%SRC_DIR%\engine\lib\stb_image\include /I%SRC_DIR%\engine\lib\mt19937\include 
	SET linker_flags=/link user32.lib gdi32.lib
	SET defines=/DDM_DEBUG /DDM_EXPORT

IF defined opengl (
	SET extern_files=%extern_files% %SRC_DIR%\engine\lib\glad\src\glad.c %SRC_DIR%\engine\lib\glad\src\glad_wgl.c 
	SET include_flags=%include_flags% /I%SRC_DIR%\engine\lib\glad\include
	SET linker_flags=%linker_flags% Opengl32.lib
	SET defines=%defines% /DDM_OPENGL
) ELSE (
	SET linker_flags=%linker_flags% d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib
)

SET compiler_flags=/W2 /Zi
SET assembly=DarkMatter

REM echo %include_flags%

if not exist "build/engine" mkdir build/engine
cd build
ECHO Building %assembly%...
cl %compiler_flags% %defines% /FC /LD %include_flags% %c_filenames% %extern_files% /Fe%assembly%.dll %linker_flags%

REM Shaders
IF not defined opengl (
	SET fxc_flags=/Fc /Od /Zi

	ECHO Compiling shader: material_vertex.hlsl
	fxc %fxc_flags% /E v_main /T vs_5_0 %SRC_DIR%/engine/shaders/hlsl/material_vertex.hlsl /Fo %SRC_DIR%/engine/shaders/hlsl/material_vertex.fxc

	ECHO Compiling shader: material_pixel.hlsl
	fxc %fxc_flags% /E p_main /T ps_5_0 %SRC_DIR%/engine/shaders/hlsl/material_pixel.hlsl /Fo %SRC_DIR%/engine/shaders/hlsl/material_pixel.fxc

	ECHO Compiling shader: material_color_vertex.hlsl
	fxc %fxc_flags% /E v_main /T vs_5_0 %SRC_DIR%/engine/shaders/hlsl/material_color_vertex.hlsl /Fo %SRC_DIR%/engine/shaders/hlsl/material_color_vertex.fxc

	ECHO Compiling shader: material_color_pixel.hlsl
	fxc %fxc_flags% /E p_main /T ps_5_0 %SRC_DIR%/engine/shaders/hlsl/material_color_pixel.hlsl /Fo %SRC_DIR%/engine/shaders/hlsl/material_color_pixel.fxc

	ECHO Compiling shader: light_src_vertex.hlsl
	fxc %fxc_flags% /E v_main /T vs_5_0 %SRC_DIR%/engine/shaders/hlsl/light_src_vertex.hlsl /Fo %SRC_DIR%/engine/shaders/hlsl/light_src_vertex.fxc

	ECHO Compiling shader: light_src_pixel.hlsl
	fxc %fxc_flags% /E p_main /T ps_5_0 %SRC_DIR%/engine/shaders/hlsl/light_src_pixel.hlsl /Fo %SRC_DIR%/engine/shaders/hlsl/light_src_pixel.fxc
)