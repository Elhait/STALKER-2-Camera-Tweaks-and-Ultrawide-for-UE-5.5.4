@echo off
rem Compatibility entry point: the production build now includes the complete overlay.
setlocal
set "ROOT=%~dp0..\.."
call "%ROOT%\build.cmd" %*
exit /b %errorlevel%
