@echo off
SetLocal EnableDelayedExpansion

if not exist "build" mkdir build

call engine/build_engine.bat
call application/build_app.bat

REM shaders
