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
rem CMake detects the localized MSVC /showIncludes prefix during configure.
rem Keep the console code page UTF-8 so the generated Ninja rule matches CL output.
chcp 65001 >nul

set "VSROOT=%VSINSTALLDIR%"
set "CMAKE_DIR=%VSROOT%Common7\IDE\CommonExtensions\Microsoft\CMake\cmake\bin"
set "NINJA_DIR=%VSROOT%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
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

set "PROFILE=production"
if /I "%CAMERA_TWEAKS_BUILD_PROFILE%"=="diagnostic" set "PROFILE=diagnostic"
set "TIMELINE=OFF"
if /I "%CAMERA_TWEAKS_STARTUP_TIMELINE%"=="1" set "TIMELINE=ON"
set "DISCOVERY=OFF"
if /I "%PROFILE%"=="diagnostic" if /I "%DIALOGUE_DISCOVERY_DIAGNOSTIC%"=="1" set "DISCOVERY=ON"
if defined CAMERA_TWEAKS_CMAKE_BUILD_ROOT (
    set "BUILD_DIR=%CAMERA_TWEAKS_CMAKE_BUILD_ROOT%\%PROFILE%-t%TIMELINE%-d%DISCOVERY%"
) else (
    set "BUILD_DIR=build-artifacts\cm\candidate\%PROFILE%-t%TIMELINE%-d%DISCOVERY%"
)

"%CMAKE_DIR%\cmake.exe" -S . -B "%BUILD_DIR%" -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA_DIR%\ninja.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DCAMERA_TWEAKS_PROFILE=%PROFILE% -DCAMERA_TWEAKS_STARTUP_TIMELINE=%TIMELINE% -DCAMERA_TWEAKS_DIALOGUE_DISCOVERY=%DISCOVERY%
if errorlevel 1 goto :failed
"%CMAKE_DIR%\cmake.exe" --build "%BUILD_DIR%" --target STALKER2CameraTweaks
set "BUILD_RESULT=%errorlevel%"
goto :done

:failed
set "BUILD_RESULT=%errorlevel%"
if "%BUILD_RESULT%"=="0" set "BUILD_RESULT=1"
:done
popd
exit /b %BUILD_RESULT%
