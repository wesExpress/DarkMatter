@echo off
SetLocal EnableDelayedExpansion

SET fxc_flags=/FC /Od /Zi

cd ../engine/shaders/hlsl

for %%f in (*.hlsl) do (
	set shader=%%~nxf
	ECHO %shader%
	REM fxc %fxc_flags% /E
)