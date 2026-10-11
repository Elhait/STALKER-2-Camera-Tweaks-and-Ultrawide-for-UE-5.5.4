@echo off
setlocal
set "FAIL_CODE="
set "ROOT=%~dp0"
pushd "%ROOT%"
if not "%errorlevel%"=="0" exit /b 1

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

PowerShell -NoProfile -ExecutionPolicy Bypass -File tests\runner\test_cmd_audit.ps1 -RunnerPath test.cmd -RegistryPath tests\test_harnesses.json
if not "%errorlevel%"=="0" goto :fail

"%CMAKE_DIR%\cmake.exe" -S . -B "%BUILD_DIR%" -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA_DIR%\ninja.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DCAMERA_TWEAKS_PROFILE=production -DCAMERA_TWEAKS_STARTUP_TIMELINE=OFF -DCAMERA_TWEAKS_DIALOGUE_DISCOVERY=OFF
if not "%errorlevel%"=="0" goto :fail

"%CMAKE_DIR%\cmake.exe" --build "%BUILD_DIR%" --target test_harnesses --parallel
if not "%errorlevel%"=="0" goto :fail

PowerShell -NoProfile -ExecutionPolicy Bypass -File tests\runner\test_cmd_exit_code_contract.ps1 -ProbePath build-artifacts\tests\test_cmd_exit_status_probe.exe
if not "%errorlevel%"=="0" goto :fail

PowerShell -NoProfile -ExecutionPolicy Bypass -File tests\runner\test_harness_runner.ps1 -RegistryPath tests\test_harnesses.json
if not "%errorlevel%"=="0" goto :fail

popd
exit /b 0

:fail
if not defined FAIL_CODE set "FAIL_CODE=%errorlevel%"
if "%FAIL_CODE%"=="0" set "FAIL_CODE=1"
echo test.cmd failed with exit code %FAIL_CODE%.>&2
popd
exit /b %FAIL_CODE%
