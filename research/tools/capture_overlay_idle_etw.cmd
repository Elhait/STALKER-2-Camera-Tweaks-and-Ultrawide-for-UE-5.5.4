@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0capture_overlay_idle_etw.ps1" -Mode Capture -Elevate
set "captureExit=%errorlevel%"
echo.
echo Launcher exit code: %captureExit%
echo Do not launch another capture while a recording is active.
pause
exit /b %captureExit%
