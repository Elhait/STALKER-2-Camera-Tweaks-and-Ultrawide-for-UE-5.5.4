@echo off
setlocal
pushd "%~dp0..\..\.."
if errorlevel 1 exit /b 1
call "E:\Work\Visual Studio Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 goto :fail
chcp 65001 >nul
set "CMAKE_DIR=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
set "NINJA_DIR=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "BUILD_DIR=build-artifacts\cm\u2-provider-dialogue-endpoint-fix\production"
"%CMAKE_DIR%\cmake.exe" -S . -B "%BUILD_DIR%" -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA_DIR%\ninja.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DCAMERA_TWEAKS_PROFILE=production -DCAMERA_TWEAKS_STARTUP_TIMELINE=OFF -DCAMERA_TWEAKS_DIALOGUE_DISCOVERY=OFF
if errorlevel 1 goto :fail
"%CMAKE_DIR%\cmake.exe" --build "%BUILD_DIR%" --target STALKER2CameraTweaks
if errorlevel 1 goto :fail
exit /b 0
:fail
set "RESULT=%errorlevel%"
if "%RESULT%"=="0" set "RESULT=1"
exit /b %RESULT%

