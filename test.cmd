@echo off
setlocal
set "ROOT=%~dp0"
pushd "%ROOT%"
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
if errorlevel 1 goto :fail
if not exist "build-artifacts\tests" mkdir "build-artifacts\tests"
PowerShell -NoProfile -ExecutionPolicy Bypass -File tests\runner\test_cmd_audit.ps1 -RunnerPath test.cmd
if errorlevel 1 goto :fail
PowerShell -NoProfile -ExecutionPolicy Bypass -File tests\runner\localization_catalog_audit.ps1
if errorlevel 1 goto :fail
PowerShell -NoProfile -ExecutionPolicy Bypass -File tools\generate_ini_documentation.ps1
if errorlevel 1 goto :fail
set "INCLUDE=%CD%\build-artifacts\generated;%INCLUDE%"
rc /nologo /fo build-artifacts\tests\localization_resources.res src\overlay\localization_resources.rc
if errorlevel 1 goto :fail
set "COMMON=/nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui"
cl %COMMON% tests\overlay\dx12_backend_safety_harness.cpp external\imgui\imgui.cpp external\imgui\imgui_draw.cpp external\imgui\imgui_tables.cpp external\imgui\imgui_widgets.cpp /link user32.lib d3d12.lib d3dcompiler.lib /OUT:build-artifacts\tests\dx12_backend_safety_harness.exe
if errorlevel 1 goto :fail

cl %COMMON% tests\lifecycle\worker_lifecycle_harness.cpp src\plugin\worker_lifecycle.cpp /link user32.lib /OUT:build-artifacts\tests\worker_lifecycle_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\feature_status\feature_status_harness.cpp src\plugin\feature_status.cpp /link /OUT:build-artifacts\tests\feature_status_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\plugin\optional_overlay_startup_harness.cpp /link /OUT:build-artifacts\tests\optional_overlay_startup_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\startup_timeline_harness.cpp /link psapi.lib /OUT:build-artifacts\tests\startup_timeline_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% /DOVERLAY_STARTUP_JOURNAL tests\diagnostics\startup_journal_harness.cpp /link /OUT:build-artifacts\tests\startup_journal_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\config\config_persistence_harness.cpp src\config\feature_config.cpp src\config\config_repository.cpp src\config\config_template.cpp /link user32.lib /OUT:build-artifacts\tests\config_persistence_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\config\gameplay_mode_harness.cpp src\config\feature_config.cpp /link user32.lib /OUT:build-artifacts\tests\gameplay_mode_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\config\runtime_settings_api_harness.cpp src\config\feature_config.cpp /link user32.lib /OUT:build-artifacts\tests\runtime_settings_api_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\gameplay\aspect_policy_harness.cpp src\gameplay\aspect_policy.cpp /link /OUT:build-artifacts\tests\aspect_policy_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\gameplay\horplus_gameplay_harness.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\aspect_policy.cpp src\cinematics\cinematic_fov.cpp src\camera\horplus.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\horplus_gameplay_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\gameplay\zoom_transition_harness.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\aspect_policy.cpp src\cinematics\cinematic_fov.cpp src\camera\horplus.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\zoom_transition_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\gameplay\gameplay_mode_transition_harness.cpp src\gameplay\gameplay_state.cpp src\camera\presentation_state.cpp /link /OUT:build-artifacts\tests\gameplay_mode_transition_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\gameplay\gameplay_camera_safety_harness.cpp src\gameplay\gameplay_camera.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c /link /OUT:build-artifacts\tests\gameplay_camera_safety_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\platform\memory_harness.cpp src\platform\win32\memory.cpp /link /OUT:build-artifacts\tests\memory_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\platform\viewport_harness.cpp src\platform\win32\viewport.cpp src\platform\win32\window.cpp /link user32.lib /OUT:build-artifacts\tests\viewport_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% /DSIGNATURE_SCANNER_TEST tests\platform\signature_scanner_harness.cpp src\hooks\signature_scanner.cpp /link /OUT:build-artifacts\tests\signature_scanner_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\platform\instruction_validator_harness.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c /link /OUT:build-artifacts\tests\instruction_validator_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\cinematics\cinematic_initialization_harness.cpp src\cinematics\cinematic_initialization.cpp src\plugin\feature_status.cpp /link /OUT:build-artifacts\tests\cinematic_initialization_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\cinematics\coordinator_recovery_harness.cpp src\gameplay\gameplay_state.cpp src\camera\presentation_state.cpp /link /OUT:build-artifacts\tests\coordinator_recovery_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\cinematics\horplus_post_exit_recovery_harness.cpp src\gameplay\gameplay_state.cpp src\camera\presentation_state.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\aspect_policy.cpp src\camera\horplus.cpp src\camera\gameplay_baseline.cpp src\camera\fov_observation.cpp src\cinematics\cinematic_fov.cpp src\dialogue\dialogue_state.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\horplus_post_exit_recovery_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\cinematics\cinematic_selection_harness.cpp src\cinematics\cinematic_selection.cpp /link /OUT:build-artifacts\tests\cinematic_selection_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\dialogue\post_cinematic_exclusion_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\post_cinematic_exclusion_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\dialogue\recovery_rearm_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\recovery_rearm_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\dialogue\candidate_hardening_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\candidate_hardening_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\dialogue\policy_snapshot_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\policy_snapshot_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\camera\camera_state_snapshot_harness.cpp src\camera\camera_state_snapshot.cpp /link /OUT:build-artifacts\tests\camera_state_snapshot_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\camera\fov_observation_harness.cpp src\camera\fov_observation.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\fov_observation_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\camera\gameplay_baseline_harness.cpp src\camera\fov_observation.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\gameplay_baseline_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\camera\gameplay_aspect_restoration_harness.cpp src\camera\fov_observation.cpp src\camera\gameplay_aspect_restoration.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\gameplay_aspect_restoration_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\cinematics\cinematic_fov_harness.cpp src\cinematics\cinematic_fov.cpp src\camera\horplus.cpp src\camera\fov_observation.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\cinematic_fov_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\cinematics\cinematic_aspect_harness.cpp src\cinematics\cinematic_aspect.cpp src\hooks\signature_scanner.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c /link /OUT:build-artifacts\tests\cinematic_aspect_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\dialogue\dialogue_fov_harness.cpp src\dialogue\dialogue_fov.cpp /link /OUT:build-artifacts\tests\dialogue_fov_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\diagnostics\diagnostics_gate_harness.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\diagnostics_gate_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\diagnostics\performance_telemetry_harness.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\performance_telemetry_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\diagnostics\language_state_transition_harness.cpp src\diagnostics\language_state_transition.cpp /link /OUT:build-artifacts\tests\language_state_transition_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\diagnostics\matchgameplay_prediction_harness.cpp /link /OUT:build-artifacts\tests\matchgameplay_prediction_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\regression\runtime_evidence_replay_harness.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\aspect_policy.cpp src\gameplay\gameplay_state.cpp src\camera\presentation_state.cpp src\cinematics\cinematic_fov.cpp src\camera\horplus.cpp src\camera\fov_observation.cpp src\camera\gameplay_baseline.cpp src\camera\gameplay_aspect_restoration.cpp src\dialogue\dialogue_state.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\runtime_evidence_replay_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\gameplay\gameplay_camera_resolver_harness.cpp src\gameplay\gameplay_camera.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c /link /OUT:build-artifacts\tests\gameplay_camera_resolver_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\overlay_lifecycle_harness.cpp src\overlay\overlay_lifecycle.cpp /link /OUT:build-artifacts\tests\overlay_lifecycle_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\discovery_evidence_harness.cpp src\overlay\discovery_evidence.cpp /link /OUT:build-artifacts\tests\discovery_evidence_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\dxgi_sdk_extent_harness.cpp /link /OUT:build-artifacts\tests\dxgi_sdk_extent_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\renderer_wait_policy_harness.cpp /link /OUT:build-artifacts\tests\renderer_wait_policy_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% /DOVERLAY_SETTINGS_FRONTEND /DOVERLAY_RENDERER_LIFETIME_TEST tests\overlay\renderer_lifetime_harness.cpp src\overlay\overlay_lifecycle.cpp src\overlay\discovery_evidence.cpp external\safetyhook\safetyhook.cpp external\safetyhook\Zydis.c src\overlay\renderer_runtime.cpp src\overlay\renderer_state.cpp src\overlay\input_state.cpp src\overlay\placement_config.cpp src\overlay\overlay_layout_metrics.cpp src\overlay\camera_integration.cpp src\overlay\feature_presentation.cpp src\overlay\camera_state_view.cpp src\overlay\selector_documentation_view.cpp src\overlay\localization_catalog.cpp src\overlay\localization_formatter.cpp src\overlay\localization_validator.cpp src\overlay\localization_manager.cpp src\overlay\localization_keys.cpp src\overlay\localization_font.cpp src\overlay\game_language_reader.cpp src\config\feature_config.cpp src\config\config_repository.cpp src\config\config_template.cpp src\diagnostics\diagnostic_runtime.cpp src\diagnostics\performance_telemetry.cpp external\imgui\imgui.cpp external\imgui\imgui_draw.cpp external\imgui\imgui_tables.cpp external\imgui\imgui_widgets.cpp external\imgui\backends\imgui_impl_dx12.cpp external\imgui\backends\imgui_impl_win32.cpp src\camera\camera_state_snapshot.cpp /link user32.lib psapi.lib dxgi.lib d3d12.lib d3dcompiler.lib build-artifacts\tests\localization_resources.res /OUT:build-artifacts\tests\renderer_lifetime_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\dxgi_callback_harness.cpp src\overlay\overlay_lifecycle.cpp src\overlay\discovery_evidence.cpp src\diagnostics\diagnostic_runtime.cpp external\safetyhook\safetyhook.cpp external\safetyhook\Zydis.c /link user32.lib dxgi.lib d3d12.lib /OUT:build-artifacts\tests\dxgi_callback_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\renderer_state_harness.cpp src\overlay\renderer_state.cpp src\overlay\discovery_evidence.cpp /link /OUT:build-artifacts\tests\renderer_state_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\overlay_layout_metrics_harness.cpp src\overlay\overlay_layout_metrics.cpp /link /OUT:build-artifacts\tests\overlay_layout_metrics_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\selector_tooltip_state_harness.cpp /link /OUT:build-artifacts\tests\selector_tooltip_state_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\input_state_harness.cpp src\overlay\input_state.cpp src\config\feature_config.cpp /link user32.lib /OUT:build-artifacts\tests\input_state_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\camera_integration_harness.cpp src\overlay\camera_integration.cpp /link /OUT:build-artifacts\tests\camera_integration_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\placement_config_harness.cpp src\overlay\placement_config.cpp src\config\config_repository.cpp src\config\feature_config.cpp src\config\config_template.cpp /link user32.lib /OUT:build-artifacts\tests\placement_config_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\feature_presentation_harness.cpp src\overlay\feature_presentation.cpp src\config\feature_config.cpp /link user32.lib /OUT:build-artifacts\tests\feature_presentation_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\semantic_snapshot_harness.cpp /link /OUT:build-artifacts\tests\semantic_snapshot_harness.exe
if errorlevel 1 goto :fail
cl %COMMON% tests\overlay\localization_harness.cpp src\overlay\localization_catalog.cpp src\overlay\localization_formatter.cpp src\overlay\localization_validator.cpp src\overlay\localization_manager.cpp src\overlay\localization_font.cpp src\overlay\localization_keys.cpp external\imgui\imgui.cpp external\imgui\imgui_draw.cpp external\imgui\imgui_tables.cpp external\imgui\imgui_widgets.cpp /link build-artifacts\tests\localization_resources.res /OUT:build-artifacts\tests\localization_harness.exe
if errorlevel 1 goto :fail

build-artifacts\tests\worker_lifecycle_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\feature_status_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\optional_overlay_startup_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\startup_timeline_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\startup_journal_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\config_persistence_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\gameplay_mode_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\runtime_settings_api_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\aspect_policy_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\horplus_gameplay_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\zoom_transition_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\gameplay_mode_transition_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\gameplay_camera_safety_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\memory_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\viewport_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\signature_scanner_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\instruction_validator_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\cinematic_initialization_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\coordinator_recovery_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\horplus_post_exit_recovery_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\cinematic_selection_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\post_cinematic_exclusion_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\recovery_rearm_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\candidate_hardening_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\policy_snapshot_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\camera_state_snapshot_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\fov_observation_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\gameplay_baseline_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\gameplay_aspect_restoration_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\cinematic_fov_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\cinematic_aspect_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\dialogue_fov_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\diagnostics_gate_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\performance_telemetry_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\language_state_transition_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\matchgameplay_prediction_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\runtime_evidence_replay_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\gameplay_camera_resolver_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\overlay_lifecycle_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\discovery_evidence_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\dxgi_sdk_extent_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\renderer_wait_policy_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\dx12_backend_safety_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\renderer_lifetime_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\dxgi_callback_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\renderer_state_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\overlay_layout_metrics_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\selector_tooltip_state_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\input_state_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\camera_integration_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\placement_config_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\feature_presentation_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\semantic_snapshot_harness.exe
if errorlevel 1 goto :fail
build-artifacts\tests\localization_harness.exe
if errorlevel 1 goto :fail

popd
exit /b 0

:fail
set "CODE=%errorlevel%"
popd
if "%CODE%"=="0" set "CODE=1"
exit /b %CODE%
