@echo off
setlocal
set "ROOT=%~dp0..\.."
pushd "%ROOT%"
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
set "COMMON=/nologo /LD /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /DOVERLAY_RENDERING_POC /Iexternal\safetyhook /Iexternal\imgui /Iexternal\imgui\backends"
cl %COMMON% /Fobuild-artifacts\overlay\ src\overlay\discovery_runtime.cpp src\overlay\input_state.cpp src\overlay\overlay_lifecycle.cpp src\overlay\discovery_evidence.cpp src\overlay\renderer_state.cpp src\overlay\renderer_runtime.cpp external\safetyhook\safetyhook.cpp external\safetyhook\Zydis.c external\imgui\imgui.cpp external\imgui\imgui_draw.cpp external\imgui\imgui_tables.cpp external\imgui\imgui_widgets.cpp external\imgui\backends\imgui_impl_dx12.cpp external\imgui\backends\imgui_impl_win32.cpp /link user32.lib dxgi.lib d3d12.lib d3dcompiler.lib /OUT:STALKER2CameraTweaksOverlayPOC.asi
if errorlevel 1 goto :fail
popd
exit /b 0

:fail
set "CODE=%errorlevel%"
popd
if "%CODE%"=="0" set "CODE=1"
exit /b %CODE%
