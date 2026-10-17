@echo off
setlocal EnableExtensions
set "VSDEVCMD="
pushd "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not "%errorlevel%"=="0" exit /b 1
for /f "delims=" %%I in ('vswhere.exe -latest -products * -version "[17.14,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat') do set "VSDEVCMD=%%I"
popd
if not defined VSDEVCMD exit /b 1
call "%VSDEVCMD%" -arch=x64 -host_arch=x64
if not "%errorlevel%"=="0" exit /b 1
chcp 65001 >nul
cl.exe @"%~1"
if not "%errorlevel%"=="0" exit /b %errorlevel%
link.exe @"%~2"
exit /b %errorlevel%
