# Overlay Toggle grouping

- Objective: visually distinguish the always-active Overlay Toggle binding from optional runtime hotkeys.
- Evidence: the UI rendered `Overlay Toggle` after the `Hotkeys` Enabled checkbox, visually associating it with that checkbox although runtime handling is independent.
- Scope: move the toggle binding into its own `Overlay` section before `Hotkeys`; clarify in the tooltip that it is always active regardless of Hotkeys.Enabled.
- Non-goals: change bindings, input handling, runtime logic, mouse capture, or other layout.
- File changed: `src/overlay/renderer_runtime.cpp`.
- Implementation: separated the Overlay section and clarified both tooltips; verified the overlay settings build.
- Validation: `build-overlay-settings.cmd` passed. No game launch.
- Risk/safe behavior: presentation only; same action, key and callback retained.
- Stop conditions: no runtime behavior or settings model changes were needed.
- Final review: source scope and build output reviewed; no Git commands or Git-state operations performed.
