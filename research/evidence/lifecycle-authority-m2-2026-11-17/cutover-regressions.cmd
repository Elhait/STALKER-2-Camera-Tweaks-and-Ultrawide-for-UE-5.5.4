@echo off
setlocal
call "E:\Work\Visual Studio Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
set "CMAKE_DIR=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
set "NINJA_DIR=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "BUILD_DIR=build-artifacts\cm\test-suite"
set "TEMP=%CD%\build-artifacts\tests\temp"
set "TMP=%TEMP%"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests\runner\test_cmd_orchestrator.ps1 -RegistryPath tests\test_harnesses.json -NonGraphics
exit /b %errorlevel%
