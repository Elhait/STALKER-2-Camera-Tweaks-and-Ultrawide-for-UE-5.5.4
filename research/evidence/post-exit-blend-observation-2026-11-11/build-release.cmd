@echo off
setlocal
set "CMAKE_BUILD_PARALLEL_LEVEL=1"
set "CAMERA_TWEAKS_BUILD_PROFILE=%~1"
set "CAMERA_TWEAKS_STARTUP_TIMELINE=0"
set "DIALOGUE_DISCOVERY_DIAGNOSTIC=0"
call tools\build\build-cmake-candidate.cmd
exit /b %errorlevel%
