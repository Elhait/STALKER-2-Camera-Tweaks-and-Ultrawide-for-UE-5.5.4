# Overlay display and Camera State layout repair

- Objective: fix misleading Win32 key names, ensure the mouse-capture notice fits the overlay, and repair Camera State label/value layout.
- Evidence/current state: screenshots showed Insert as `Num 0`, Delete as `Num Del`; notice on one line; expanded Camera State labels were separated from or appeared without values. Existing display used `GetKeyNameTextW` without extended scan-code semantics for navigation keys; Camera State fields manually positioned values with `SameLine` and wrapping.
- Scope: adjust hotkey display names and overlay text/layout only; preserve key identity, persistence, capture policy, camera/runtime semantics and other settings.
- Non-goals: Git commands/state operations, hook/camera behavior, input policy changes, unrelated redesign, game launch.
- Files: `src/config/feature_config.cpp`, `src/overlay/renderer_runtime.cpp`, `tests/config/config_persistence_harness.cpp`.
- Batches: (1) stable semantic labels for navigation keys with Insert/Delete/Home tests; (2) split/wrap the mouse notice and render Camera State values in bounded label/value tables; (3) run `test.cmd` and overlay build, inspect only scoped files.
- Risks/safe behavior: display-only change; stable VK persistence is unchanged. Other key names retain Win32/fallback naming. Tables constrain long text to available column width and do not change parent sizing.
- Stop conditions: no camera snapshot semantics or input capture changes; no game launch.
- Final review: inspect changed source/test content and validation output only. Do not run Git commands or change Git state.
