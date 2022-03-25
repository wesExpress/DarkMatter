@echo off
SetLocal EnableDelayedExpansion

REM set "opengl=true"

cd engine/src

SET c_filenames=
FOR /R %%f in (*.c) do (
	SET c_filenames=!c_filenames! %%f
	REM ECHO %%f
)

cd ../..

IF defined opengl (
	SET extern_files=..\engine\lib\stb_image\src\stb_image.c ..\engine\lib\mt19937\src\mt19937.c ..\engine\lib\mt19937\src\mt19937_64.c ..\engine\lib\glad\src\glad.c ..\engine\lib\glad\src\glad_wgl.c

	SET include_flags=/I..\engine\src /I..\engine\lib\stb_image\include /I..\engine\lib\mt19937\include /I..\engine\lib\glad\include

	SET linker_flags="/link user32.lib gdi32.lib Opengl32.lib"

	SET defines=/DDM_DEBUG /DDM_EXPORT /DDM_OPENGL
) ELSE (
	SET extern_files=..\engine\lib\stb_image\src\stb_image.c ..\engine\lib\mt19937\src\mt19937.c ..\engine\lib\mt19937\src\mt19937_64.c

	SET include_flags=/I..\engine\src /I..\engine\lib\stb_image\include /I..\engine\lib\mt19937\include

	SET linker_flags="/link user32.lib gdi32.lib d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib"
	
	SET defines=/DDM_DEBUG /DDM_EXPORT
)

SET compiler_flags=/W2 /Zi
SET assembly=DarkMatter

REM echo %include_flags%

cd bin
ECHO Building %assembly%...
cl %compiler_flags% %defines% /FC /LD %include_flags% %c_filenames% %extern_files% /Fe%assembly%.dll %linker_flags%

REM Shaders
IF defined opengl (
	if not exist "shaders\glsl\" mkdir shaders\glsl
	xcopy ..\engine\shaders\glsl\"." shaders\glsl /Y
) ELSE (
	if not exist "shaders\hlsl" mkdir shaders\hlsl
	SET fxc_flags=/Fc /Od /Zi

	ECHO Compiling shader: object_vertex.hlsl
	fxc %fxc_flags% /E v_main /T vs_5_0 ../engine/shaders/hlsl/object_vertex.hlsl /Fo shaders/hlsl/object_vertex.fxc

	ECHO Compiling shader: object_pixel.hlsl
	fxc %fxc_flags% /E p_main /T ps_5_0 ../engine/shaders/hlsl/object_pixel.hlsl /Fo shaders/hlsl/object_pixel.fxc

	ECHO Compiling shader: light_src_vertex.hlsl
	fxc %fxc_flags% /E v_main /T vs_5_0 ../engine/shaders/hlsl/light_src_vertex.hlsl /Fo shaders/hlsl/light_src_vertex.fxc

	ECHO Compiling shader: light_src_pixel.hlsl
	fxc %fxc_flags% /E p_main /T ps_5_0 ../engine/shaders/hlsl/light_src_pixel.hlsl /Fo shaders/hlsl/light_src_pixel.fxc
)