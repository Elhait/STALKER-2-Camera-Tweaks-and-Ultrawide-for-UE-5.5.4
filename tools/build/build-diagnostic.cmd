@echo off
setlocal
set "ROOT=%~dp0..\.."
pushd "%ROOT%"
if errorlevel 1 exit /b 1
set "CAMERA_TWEAKS_BUILD_PROFILE=diagnostic"
call "%ROOT%\build.cmd"
set "BUILD_RESULT=%errorlevel%"
popd
exit /b %BUILD_RESULT%
