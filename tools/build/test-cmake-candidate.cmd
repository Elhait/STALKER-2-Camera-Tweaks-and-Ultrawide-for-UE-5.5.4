@echo off
setlocal
pushd "%~dp0..\.."
if errorlevel 1 exit /b 1

set "VSDEVCMD="
if defined VSINSTALLDIR if exist "%VSINSTALLDIR%Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%VSINSTALLDIR%Common7\Tools\VsDevCmd.bat"
if not defined VSDEVCMD if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    pushd "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
    for /f "delims=" %%I in ('vswhere.exe -latest -products * -version "[17.14,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat') do set "VSDEVCMD=%%I"
    popd
)
if not defined VSDEVCMD (
    echo Visual Studio 2022 C++ toolchain not found.>&2
    popd
    exit /b 1
)
call "%VSDEVCMD%" -arch=x64 -host_arch=x64
if errorlevel 1 goto :failed
chcp 65001 >nul

set "VSROOT=%VSINSTALLDIR%"
set "CMAKE_DIR=%VSROOT%Common7\IDE\CommonExtensions\Microsoft\CMake\cmake\bin"
set "NINJA_DIR=%VSROOT%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "BUILD_DIR=build-artifacts\cm\candidate\production-tOFF-dOFF"
if not exist "%CMAKE_DIR%\cmake.exe" (
    echo Visual Studio bundled CMake not found.>&2
    set "BUILD_RESULT=1"
    goto :done
)
if not exist "%NINJA_DIR%\ninja.exe" (
    echo Visual Studio bundled Ninja not found.>&2
    set "BUILD_RESULT=1"
    goto :done
)
set "PATH=%CMAKE_DIR%;%NINJA_DIR%;%PATH%"
"%CMAKE_DIR%\cmake.exe" -S . -B "%BUILD_DIR%" -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA_DIR%\ninja.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DCAMERA_TWEAKS_PROFILE=production -DCAMERA_TWEAKS_STARTUP_TIMELINE=OFF -DCAMERA_TWEAKS_DIALOGUE_DISCOVERY=OFF
if errorlevel 1 goto :failed
"%CMAKE_DIR%\cmake.exe" --build "%BUILD_DIR%" --target runtime_native_adapter_harness n06_m01_zoom_game_derived_harness --parallel
if errorlevel 1 goto :failed
"%CMAKE_DIR%\ctest.exe" --test-dir "%BUILD_DIR%" --output-on-failure
set "BUILD_RESULT=%errorlevel%"
goto :done

:failed
set "BUILD_RESULT=%errorlevel%"
if "%BUILD_RESULT%"=="0" set "BUILD_RESULT=1"
:done
popd
exit /b %BUILD_RESULT%
