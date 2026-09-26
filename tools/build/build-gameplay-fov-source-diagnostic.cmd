@echo off
setlocal
set "ROOT=%~dp0..\.."
pushd "%ROOT%"
if errorlevel 1 exit /b 1
set "CAMERA_TWEAKS_BUILD_PROFILE="
set "CAMERA_TWEAKS_OUTPUT_NAME=build-artifacts\test-asi\fov-source-diagnostic\STALKER2CameraTweaks.asi"
set "CAMERA_TWEAKS_OBJECT_DIR=build-artifacts\obj-gameplay-fov-source-diagnostic"
if not exist "build-artifacts\test-asi\fov-source-diagnostic" mkdir "build-artifacts\test-asi\fov-source-diagnostic"
if errorlevel 1 (
    popd
    exit /b 1
)
call "%ROOT%\build.cmd"
set "BUILD_RESULT=%errorlevel%"
popd
exit /b %BUILD_RESULT%
