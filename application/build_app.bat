@echo off
SetLocal EnableDelayedExpansion

cd application/src

SET c_filenames=
FOR /R %%f in (*.c) do (
	SET c_filenames=!c_filenames! %%f
	REM ECHO %%f
)

cd ../..

SET assembly=DarkMatterApp
SET linker_flags=/linkDarkMatter.lib
SET compiler_flags="/W2"
SET include_flags=/I..\engine\include /I..\engine\src

REM echo %include_flags%

cd bin
ECHO Building %assembly%...
cl %compiler_flags% /FC %include_flags% %c_filenames% /Fe%assembly% %linker_flags%