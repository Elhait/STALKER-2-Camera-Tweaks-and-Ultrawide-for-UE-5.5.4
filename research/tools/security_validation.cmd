@echo off
setlocal
cd /d "%~dp0..\.."
call "E:\Work\Visual Studio Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
chcp 65001 >nul
set "CMAKE_DIR=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
set "NINJA_DIR=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "PATH=%CMAKE_DIR%;%NINJA_DIR%;%PATH%"
set "CMAKE_BUILD_PARALLEL_LEVEL=1"
if /I "%~1"=="Regression" goto regression
if /I "%~1"=="Release" goto release
if /I not "%~1"=="FocusedBuild" exit /b 2
"%CMAKE_DIR%\cmake.exe" -S . -B build-artifacts/cm/test-suite -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA_DIR%\ninja.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DCAMERA_TWEAKS_PROFILE=production -DCAMERA_TWEAKS_STARTUP_TIMELINE=OFF -DCAMERA_TWEAKS_DIALOGUE_DISCOVERY=OFF
if errorlevel 1 exit /b 1
"%NINJA_DIR%\ninja.exe" -C build-artifacts/cm/test-suite -j 1 worker_lifecycle_harness config_persistence_harness localization_harness startup_journal_harness runtime_native_adapter_harness
exit /b %errorlevel%

:regression
call test.cmd -NonGraphics
exit /b %errorlevel%

:release
set "CAMERA_TWEAKS_CMAKE_BUILD_ROOT=build-artifacts\security-hardening\release"
call tools\build\build-cmake-candidate.cmd
exit /b %errorlevel%
