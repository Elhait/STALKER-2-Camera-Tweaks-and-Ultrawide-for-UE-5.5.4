@echo off
setlocal
call "E:\Work\Visual Studio Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
set "CMAKE_BUILD_PARALLEL_LEVEL=1"
"E:\Work\Visual Studio Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build-artifacts\cm\test-suite --target camera_writer_identity_harness runtime_native_adapter_harness --parallel 1
exit /b %errorlevel%
