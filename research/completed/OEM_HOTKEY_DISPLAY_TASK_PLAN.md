# OEM hotkey display repair

- Objective: show readable symbols for accepted OEM punctuation key bindings.
- Evidence: user runtime screenshots confirm `[ ] ; ' , .` bind and work, but the overlay shows `?`. `HotkeyName` used `GetKeyNameTextW` after scan-code conversion; keyboard-layout OEM symbol resolution was missing/inadequate.
- Scope: modify display-name resolution and focused deterministic config tests only. Keep VK identity, key capture, persistence format, action routing, and modifier exclusions unchanged.
- Non-goals: enabling Ctrl/Shift, changing key whitelist/capture, camera/UI layout changes, or runtime game test.
- Files: `src/config/feature_config.cpp`, `tests/config/config_persistence_harness.cpp`.
- Implementation: resolve standard OEM punctuation through the active Windows keyboard layout, then use readable fallbacks where mapping is absent/invalid.
- Validation: `test.cmd` passed all 39 harnesses, including OEM identity → persistence → reload → display checks; `build-overlay-settings.cmd` passed.
- Risks/safe failure: use fallback symbols if layout mapping yields no usable character; canonical VK identity remains unchanged.
- Stop conditions: no capture/persistence change; no game launch.
- Final review: touched-source scope and command results reviewed. No Git commands or Git-state operations performed.
