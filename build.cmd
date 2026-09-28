@echo off
setlocal
pushd "%~dp0"
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
if errorlevel 1 goto :build_failed
rem MSVC 17.14 exposes the required C++23 feature set through /std:c++latest.
set "OBJECT_DIR=build-artifacts\obj"
if defined CAMERA_TWEAKS_OBJECT_DIR set "OBJECT_DIR=%CAMERA_TWEAKS_OBJECT_DIR%"
if not exist "%OBJECT_DIR%" mkdir "%OBJECT_DIR%"
set "DIALOGUE_DIAGNOSTIC_DEFINE="
set "DIALOGUE_DISCOVERY_DEFINE="
set "ZOOM_TRANSITION_DEFINE="
set "HORPLUS_FOV_STATE_DEFINE="
set "DIALOGUE_RECOVERY_ENDPOINT_DEFINE="
set "CAMERA_STATE_SNAPSHOT_DEFINE="
set "SUPPORTED_DIAGNOSTICS_DEFINE="
set "PRESENT_FAILURE_BREAK_DEFINE="
set "STARTUP_JOURNAL_DEFINE=/DOVERLAY_STARTUP_JOURNAL"
set "STARTUP_TIMELINE_DEFINE="
rem The lightweight milestone journal is production-default. The forensic
rem trace remains opt-in and replaces it because both target the same log file.
if /I "%CAMERA_TWEAKS_STARTUP_TIMELINE%"=="1" (
    set "STARTUP_JOURNAL_DEFINE="
    set "STARTUP_TIMELINE_DEFINE=/DOVERLAY_STARTUP_TIMELINE"
)
if /I "%CAMERA_TWEAKS_BREAK_ON_PRESENT_FAILURE%"=="1" set "PRESENT_FAILURE_BREAK_DEFINE=/DOVERLAY_BREAK_ON_PRESENT_FAILURE"
set "DIALOGUE_OUTPUT=STALKER2CameraTweaks.asi"
if /I "%CAMERA_TWEAKS_BUILD_PROFILE%"=="diagnostic" (
    set "SUPPORTED_DIAGNOSTICS_DEFINE=/DZOOM_TRANSITION_DIAGNOSTIC /DHORPLUS_FOV_STATE_DIAGNOSTIC /DCAMERA_STATE_SNAPSHOT_DIAGNOSTIC /DHORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC /DMATCHGAMEPLAY_DIAGNOSTIC"
    if /I "%DIALOGUE_DIAGNOSTIC%"=="1" set "DIALOGUE_DIAGNOSTIC_DEFINE=/DDIALOGUE_BOUNDARY_DIAGNOSTIC"
    if /I "%DIALOGUE_DISCOVERY_DIAGNOSTIC%"=="1" set "DIALOGUE_DISCOVERY_DEFINE=/DDIALOGUE_DISCOVERY_DIAGNOSTIC"
    if /I "%ZOOM_TRANSITION_DIAGNOSTIC%"=="1" set "ZOOM_TRANSITION_DEFINE=/DZOOM_TRANSITION_DIAGNOSTIC"
    if /I "%HORPLUS_FOV_STATE_DIAGNOSTIC%"=="1" set "HORPLUS_FOV_STATE_DEFINE=/DHORPLUS_FOV_STATE_DIAGNOSTIC"
    if /I "%DIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC%"=="1" set "DIALOGUE_RECOVERY_ENDPOINT_DEFINE=/DDIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC"
    if /I "%CAMERA_STATE_SNAPSHOT_DIAGNOSTIC%"=="1" set "CAMERA_STATE_SNAPSHOT_DEFINE=/DCAMERA_STATE_SNAPSHOT_DIAGNOSTIC"
    set "DIALOGUE_OUTPUT=STALKER2CameraTweaksDiagnostic.asi"
)
if defined CAMERA_TWEAKS_OUTPUT_NAME set "DIALOGUE_OUTPUT=%CAMERA_TWEAKS_OUTPUT_NAME%"
if not exist "build-artifacts\overlay" mkdir "build-artifacts\overlay"
PowerShell -NoProfile -ExecutionPolicy Bypass -File tests\runner\localization_catalog_audit.ps1
if errorlevel 1 goto :build_failed
PowerShell -NoProfile -ExecutionPolicy Bypass -File tools\generate_ini_documentation.ps1
if errorlevel 1 goto :build_failed
set "INCLUDE=%CD%\build-artifacts\generated;%INCLUDE%"
rc /nologo /fo build-artifacts\overlay\localization_resources.res src\overlay\localization_resources.rc
if errorlevel 1 goto :build_failed
set "OVERLAY_DEFINE=/DOVERLAY_PRODUCTION /DOVERLAY_SETTINGS_FRONTEND /DOVERLAY_COMBINED %STARTUP_JOURNAL_DEFINE% %STARTUP_TIMELINE_DEFINE%"
cl /nologo /LD /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /DNDEBUG %OVERLAY_DEFINE% %SUPPORTED_DIAGNOSTICS_DEFINE% %DIALOGUE_DIAGNOSTIC_DEFINE% %DIALOGUE_DISCOVERY_DEFINE% %ZOOM_TRANSITION_DEFINE% %HORPLUS_FOV_STATE_DEFINE% %DIALOGUE_RECOVERY_ENDPOINT_DEFINE% %CAMERA_STATE_SNAPSHOT_DEFINE% %PRESENT_FAILURE_BREAK_DEFINE% /Fo%OBJECT_DIR%\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui /Iexternal\imgui\backends ^
 src\plugin\runtime.cpp src\plugin\worker_lifecycle.cpp src\plugin\feature_status.cpp src\plugin\dll_entry.cpp src\config\feature_config.cpp src\config\config_repository.cpp src\config\config_template.cpp src\diagnostics\diagnostic_runtime.cpp src\diagnostics\performance_telemetry.cpp src\hooks\signature_scanner.cpp src\hooks\instruction_validator.cpp src\hooks\hook_set.cpp src\gameplay\gameplay_state.cpp src\gameplay\gameplay_camera.cpp src\gameplay\aspect_policy.cpp src\gameplay\horplus_gameplay.cpp src\cinematics\cinematic_fov.cpp src\cinematics\cinematic_selection.cpp src\cinematics\cinematic_aspect.cpp src\cinematics\cinematic_initialization.cpp src\dialogue\dialogue_fov.cpp src\dialogue\dialogue_state.cpp src\camera\camera_state_snapshot.cpp src\camera\fov_observation.cpp src\camera\gameplay_baseline.cpp src\camera\gameplay_aspect_restoration.cpp src\camera\presentation_state.cpp src\camera\horplus.cpp src\platform\win32\memory.cpp src\platform\win32\sha256.cpp src\platform\win32\window.cpp src\platform\win32\viewport.cpp ^
 src\overlay\camera_integration.cpp src\overlay\feature_presentation.cpp src\overlay\camera_state_view.cpp src\overlay\selector_documentation_view.cpp src\overlay\overlay_layout_metrics.cpp src\overlay\placement_config.cpp src\overlay\localization_catalog.cpp src\overlay\localization_formatter.cpp src\overlay\localization_validator.cpp src\overlay\localization_manager.cpp src\overlay\localization_font.cpp src\overlay\localization_keys.cpp src\overlay\game_language_reader.cpp src\overlay\composition_runtime.cpp src\overlay\composition_presenter_state.cpp src\overlay\input_state.cpp src\overlay\renderer_state.cpp src\overlay\renderer_runtime.cpp src\overlay\imgui_d3d11_renderer.cpp ^
 external\safetyhook\safetyhook.cpp external\safetyhook\Zydis.c external\imgui\imgui.cpp external\imgui\imgui_draw.cpp external\imgui\imgui_tables.cpp external\imgui\imgui_widgets.cpp external\imgui\backends\imgui_impl_win32.cpp ^
 /link user32.lib psapi.lib bcrypt.lib dxgi.lib d3d11.lib dcomp.lib d3dcompiler.lib build-artifacts\overlay\localization_resources.res /OUT:"%DIALOGUE_OUTPUT%"
set "BUILD_RESULT=%errorlevel%"
popd
exit /b %BUILD_RESULT%

:build_failed
set "BUILD_RESULT=%errorlevel%"
popd
exit /b %BUILD_RESULT%
