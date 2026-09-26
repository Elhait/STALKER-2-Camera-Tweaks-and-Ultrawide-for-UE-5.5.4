# Overlay Settings Frontend — Implementation Report

## Scope

This batch converted the runtime-validated ImGui overlay POC into an
experimental settings frontend. F9–F12 were not migrated or otherwise changed
by this batch.

The normal production ASI remains separate from the overlay. Because the
production `RuntimeState` is private to its translation unit, the frontend is
built as `STALKER2CameraTweaksOverlayIntegration.asi`, combining the existing
production runtime and overlay in one module. This avoids an unsafe cross-DLL
state bridge and avoids a duplicate settings owner.

## Implemented

- Added a typed `RuntimeSettingsSnapshot` read boundary.
- Added typed mutation and persistence callbacks for:
  - Gameplay enabled/mode;
  - Cinematic aspect policy/FOV mode;
  - Dialogue zoom policy.
- Reused the existing configuration persistence path.
- Replaced the overlay test window with Gameplay, Cinematics and Dialogue
  controls plus read-only viewport aspect display.
- UI selections are read back from the shared effective snapshot rather than
  maintained as overlay-owned authoritative state.
- Invalid/unavailable mutations are rejected without affecting the renderer.
- Added `build-overlay-settings.cmd` for the combined experimental artifact.
- Kept `build-overlay-poc.cmd` as the standalone rendering POC path.

## Validation

```yaml
runtime_settings_api: IMPLEMENTED
overlay_settings_frontend: IMPLEMENTED
production_runtime_instance_shared: COMBINED_BUILD_REQUIRED
persistence: IMPLEMENTED
hotkeys_modified: NO
camera_semantics_changed: NO_INTENDED
deterministic_tests: PASS
test_cmd: PASS (36/36)
production_build: PASS
diagnostic_build: PASS
standalone_overlay_poc_build: PASS
combined_overlay_settings_build: PASS
git_diff_check: PASS (line-ending warnings only)
runtime: NOT_PERFORMED
combined_runtime_test_ready: YES
```

The required runtime validation remains one combined session using the
experimental integration artifact. It should verify initial values, each
control, persistence, reopen behavior and resize while the overlay is visible.
