@echo off
call "E:\Work\Visual Studio Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
"E:\Work\Visual Studio Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build-artifacts\cm\test-suite --target runtime_authority_cutover_harness --parallel 1
exit /b %errorlevel%
