@echo off
SetLocal EnableDelayedExpansion

set SRC_DIR=%cd%

cd build

if not exist "shaders" mkdir shaders
if not exist "assets" mkdir assets

xcopy %SRC_DIR%\engine\shaders\ shaders /S /Y
xcopy %SRC_DIR%\application\assets\ assets /S /Y