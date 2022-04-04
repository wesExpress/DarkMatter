@echo off
SetLocal EnableDelayedExpansion

set SRC_DIR=%cd%

cd application/src

SET c_filenames=
FOR /R %%f in (*.c) do (
	SET c_filenames=!c_filenames! %%f
	REM ECHO %%f
)

cd ../..

SET assembly=DarkMatterApp
SET linker_flags=/linkDarkMatter.lib
SET compiler_flags=/W2 /Zi
SET include_flags=/I..\engine\include /I..\engine\src

REM echo %include_flags%

cd build
ECHO Building %assembly%...
cl %compiler_flags% /FC %include_flags% %c_filenames% /Fe%assembly% %linker_flags%