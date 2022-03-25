@echo off
SetLocal EnableDelayedExpansion

if not exist "bin" mkdir bin

call engine/build_engine.bat
call application/build_app.bat

REM shaders
