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

if not exist "build-artifacts\dcomp-prototype" mkdir "build-artifacts\dcomp-prototype"
cl /nologo /LD /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /DNDEBUG /Fo:build-artifacts\dcomp-prototype\ research\prototypes\dcomp_presenter\dcomp_presenter_prototype.cpp /link user32.lib d3d11.lib dxgi.lib d2d1.lib dwrite.lib dcomp.lib /OUT:"build-artifacts\dcomp-prototype\STALKER2CameraTweaks.asi"
set "BUILD_RESULT=%errorlevel%"
popd
exit /b %BUILD_RESULT%

:failed
set "BUILD_RESULT=%errorlevel%"
popd
exit /b %BUILD_RESULT%
