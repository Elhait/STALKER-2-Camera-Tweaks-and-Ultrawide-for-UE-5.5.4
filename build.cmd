@echo off
setlocal EnableExtensions
pushd "%~dp0"
if errorlevel 1 exit /b 1

rem Each option set gets a distinct CMake/Ninja binary tree. The legacy
rem CAMERA_TWEAKS_OBJECT_DIR now names the parent of that isolated tree.
set "PROFILE=production"
if /I "%CAMERA_TWEAKS_BUILD_PROFILE%"=="diagnostic" set "PROFILE=diagnostic"
set "TIMELINE=OFF"
if /I "%CAMERA_TWEAKS_STARTUP_TIMELINE%"=="1" set "TIMELINE=ON"
set "DISCOVERY=OFF"
if /I "%PROFILE%"=="diagnostic" if /I "%DIALOGUE_DISCOVERY_DIAGNOSTIC%"=="1" set "DISCOVERY=ON"

set "CMAKE_BUILD_ROOT=build-artifacts\cm"
if defined CAMERA_TWEAKS_OBJECT_DIR set "CMAKE_BUILD_ROOT=%CAMERA_TWEAKS_OBJECT_DIR%\cmake"
set "CAMERA_TWEAKS_CMAKE_BUILD_ROOT=%CMAKE_BUILD_ROOT%"
call "%~dp0tools\build\build-cmake-candidate.cmd"
set "BUILD_RESULT=%errorlevel%"
if not "%BUILD_RESULT%"=="0" goto :done

set "OUTPUT_NAME=STALKER2CameraTweaks.asi"
set "INTERNAL_NAME=STALKER2CameraTweaks.asi"
if /I "%PROFILE%"=="diagnostic" (
    set "OUTPUT_NAME=STALKER2CameraTweaksDiagnostic.asi"
    set "INTERNAL_NAME=STALKER2CameraTweaksDiagnostic.asi"
)
if defined CAMERA_TWEAKS_OUTPUT_NAME set "OUTPUT_NAME=%CAMERA_TWEAKS_OUTPUT_NAME%"
set "INTERNAL_OUTPUT=%CMAKE_BUILD_ROOT%\%PROFILE%-t%TIMELINE%-d%DISCOVERY%\bin\%INTERNAL_NAME%"
if not exist "%INTERNAL_OUTPUT%" (
    echo CMake build succeeded but the expected ASI was not produced: %INTERNAL_OUTPUT%>&2
    set "BUILD_RESULT=1"
    goto :done
)

PowerShell -NoProfile -ExecutionPolicy Bypass -File tools\build\publish_asi.ps1 -Source "%CD%\%INTERNAL_OUTPUT%" -Destination "%OUTPUT_NAME%"
set "BUILD_RESULT=%errorlevel%"

:done
popd
exit /b %BUILD_RESULT%
