@echo off
setlocal
set "FAIL_CODE="
set "TEST_RESULT=0"
set "PUSHD_DONE=0"
set "ROOT=%~dp0"
pushd "%ROOT%"
if errorlevel 1 (
    set "TEST_RESULT=1"
    goto :finish
)
set "PUSHD_DONE=1"

set "VSDEVCMD="
if defined VSINSTALLDIR if exist "%VSINSTALLDIR%Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%VSINSTALLDIR%Common7\Tools\VsDevCmd.bat"
if not defined VSDEVCMD if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    pushd "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
    for /f "delims=" %%I in ('vswhere.exe -latest -products * -version "[17.14,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat') do set "VSDEVCMD=%%I"
    popd
)
if not defined VSDEVCMD (
    echo Visual Studio 2022 C++ toolchain not found.>&2
    set "FAIL_CODE=1"
    goto :fail
)

call "%VSDEVCMD%" -arch=x64 -host_arch=x64
if not "%errorlevel%"=="0" goto :fail
chcp 65001 >nul

set "VSROOT=%VSINSTALLDIR%"
set "CMAKE_DIR=%VSROOT%Common7\IDE\CommonExtensions\Microsoft\CMake\cmake\bin"
set "NINJA_DIR=%VSROOT%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "BUILD_DIR=build-artifacts\cm\test-suite"
if not exist "%CMAKE_DIR%\cmake.exe" (
    echo Visual Studio bundled CMake not found.>&2
    set "FAIL_CODE=1"
    goto :fail
)
if not exist "%NINJA_DIR%\ninja.exe" (
    echo Visual Studio bundled Ninja not found.>&2
    set "FAIL_CODE=1"
    goto :fail
)
set "PATH=%CMAKE_DIR%;%NINJA_DIR%;%PATH%"

if not exist "build-artifacts\tests\temp" mkdir "build-artifacts\tests\temp"
if not "%errorlevel%"=="0" goto :fail
set "TEMP=%CD%\build-artifacts\tests\temp"
set "TMP=%TEMP%"

PowerShell -NoProfile -ExecutionPolicy Bypass -File tests\runner\test_cmd_orchestrator.ps1 -RegistryPath tests\test_harnesses.json %*
if not "%errorlevel%"=="0" goto :fail

goto :finish

:fail
if not defined FAIL_CODE set "FAIL_CODE=%errorlevel%"
if "%FAIL_CODE%"=="0" set "FAIL_CODE=1"
echo test.cmd failed with exit code %FAIL_CODE%.>&2
set "TEST_RESULT=%FAIL_CODE%"

:finish
if "%PUSHD_DONE%"=="1" popd
if "%TEST_RESULT%"=="0" (
    echo Test run finished with no failures. Review PASS and SKIP totals above.
) else (
    echo Tests failed with exit code %TEST_RESULT%.
)
echo Press any key to close this window...
pause >nul
exit /b %TEST_RESULT%
