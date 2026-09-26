@echo off
setlocal
set "ROOT=%~dp0..\.."
pushd "%ROOT%"
if errorlevel 1 exit /b 1
set "VSDEVCMD="
if defined VSINSTALLDIR if exist "%VSINSTALLDIR%Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%VSINSTALLDIR%Common7\Tools\VsDevCmd.bat"
if not defined VSDEVCMD if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    pushd "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
    for /f "delims=" %%I in ('vswhere.exe -latest -products * -version "[17.14,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat') do set "VSDEVCMD=%%I"
    popd
)
if not defined VSDEVCMD (
    echo Visual Studio 2022 C++ toolchain not found.&gt;&amp;2
    popd
    exit /b 1
)
call "%VSDEVCMD%" -arch=x64 -host_arch=x64
if errorlevel 1 goto :fail
if not exist "build-artifacts\overlay" mkdir "build-artifacts\overlay"
cl /nologo /LD /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\overlay\ /Iexternal\safetyhook src\overlay\discovery_runtime.cpp src\overlay\overlay_lifecycle.cpp src\overlay\discovery_evidence.cpp external\safetyhook\safetyhook.cpp external\safetyhook\Zydis.c /link user32.lib dxgi.lib d3d12.lib /OUT:STALKER2CameraTweaksOverlayDiscovery.asi
set "BUILD_RESULT=%errorlevel%"
popd
exit /b %BUILD_RESULT%

:fail
set "BUILD_RESULT=%errorlevel%"
popd
exit /b %BUILD_RESULT%
