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
if not exist "build-artifacts\language-state-watcher" mkdir "build-artifacts\language-state-watcher"
set "OUT=build-artifacts\language-state-watcher"
cl /nologo /std:c++latest /EHsc /W4 /utf-8 /I src\diagnostics tests\diagnostics\language_state_transition_harness.cpp src\diagnostics\language_state_transition.cpp /Fe:%OUT%\language_state_transition_harness.exe
if errorlevel 1 goto :fail
%OUT%\language_state_transition_harness.exe
if errorlevel 1 goto :fail
cl /nologo /LD /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /I src\diagnostics src\diagnostics\language_state_watcher.cpp src\diagnostics\language_state_transition.cpp /link user32.lib /OUT:%OUT%\STALKER2LanguageStateWatcherDiagnostic.asi
if errorlevel 1 goto :fail
popd
exit /b 0

:fail
set "CODE=%errorlevel%"
popd
if "%CODE%"=="0" set "CODE=1"
exit /b %CODE%
