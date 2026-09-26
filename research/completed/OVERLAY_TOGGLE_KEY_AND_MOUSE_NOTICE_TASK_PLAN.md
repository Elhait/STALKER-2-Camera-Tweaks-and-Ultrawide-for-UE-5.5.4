# Overlay Toggle Key and Mouse Notice — Task Plan

## Objective

Show a compact informational mouse-capture notice in the overlay, allow rebinding the overlay visibility toggle, and generalize all five overlay/runtime bindings to validated ordinary single keyboard keys with stable persistence and display.

## Established Evidence and Current State

- Overlay visibility was toggled only on `WM_KEYUP/VK_INSERT` in `src/overlay/input_state.cpp`.
- Four runtime hotkeys were limited to F1–F12, A–Z and 0–9 by `config::IsSupportedHotkey`.
- Rebinding ran after the overlay toggle check in the window procedure, so event ordering had to change.
- Runtime config mutations enforced uniqueness only across four runtime bindings and reserved `VK_INSERT`.
- Config defaults and INI persistence covered four hotkeys; missing fields load `FeatureConfig` defaults.
- The UI had a Hotkeys section with an Enabled checkbox and four binding rows; mouse-capture behavior was already implemented.
- Existing working tree contained substantial staged/unstaged user changes; unrelated state was preserved.

## Approved Scope

- Add `OverlayToggle` with default `Insert` and backward-compatible behavior for old INI files.
- Add an Overlay Toggle rebind row, active regardless of `Hotkeys.Enabled`.
- Apply one validated ordinary single-key Win32 VK model to Overlay Toggle, Gameplay Mode, Cinematic Aspect, Cinematic FOV, and Dialogue Zoom.
- Keep all five bindings mutually exclusive. Escape cancels; modifier combinations and mouse buttons remain unsupported.
- Keep capture, identity, display, persistence, and reload coherent, including OEM, navigation, numpad and F13–F24 keys.
- Add an informational notice before Camera Transition with a noninteractive visual keycap showing the current toggle key.
- Update focused tests and configuration/README help text.

## Explicit Non-goals

- Modifier combinations, mouse buttons, camera/settings semantics, input-capture policy, and camera hooks.
- Game launch/injection/runtime testing, Ghidra, packaging/release work, or unrelated UI restyling.
- Reworking other staged/unstaged changes.

## Expected Files / Areas

- `src/config/feature_config.hpp/.cpp`, `src/config/config_repository.cpp`, `src/config/config_template.cpp`
- `src/plugin/runtime_settings.hpp`, relevant mutation/snapshot/persistence paths in `src/plugin/runtime.cpp`
- `src/overlay/input_state.hpp/.cpp`, `src/overlay/discovery_runtime.cpp`, `src/overlay/renderer_runtime.cpp`
- `tests/config/config_persistence_harness.cpp`, `tests/config/runtime_settings_api_harness.cpp`, `tests/overlay/input_state_harness.cpp`, `test.cmd`, and focused `README.md` material
- This plan archived under `research/completed/` and factual `backlog/TASKLOG.md` entry

## Batches and Validation

1. **Key identity/config:** validate ordinary keyboard VKs, preserve legacy symbolic parsing, persist stable `VK_XX` identity, add default and uniqueness across five bindings. Validate config parsing, round-trip, legacy default, invalid keys and duplicate repair.
2. **Input/runtime/UI:** add the Overlay Toggle action and snapshot/mutation/persistence; process capture before toggle dispatch; preserve Escape recovery and consumed key release; render dynamic notice and binding control. Validate input/runtime harnesses and overlay build.
3. **Final review:** run `test.cmd`, overlay build and `git diff --check`; compare changed paths with scope; archive plan and append task log only after success. Do not claim in-game validation.

## Risks and Safe-failure / Rollback

- Exclude mouse, modifier, IME, synthetic and invalid VK identities; invalid config leaves safe defaults.
- Persist VK identity rather than localized display text so reload restores the same key.
- On duplicate config keys, deterministically restore a unique default for the later action and log the repair.
- Display localized key names via Win32, with a stable `VK_XX` fallback.
- Preserve unrelated user changes; if relevant validation fails due to external/pre-existing state, report it without broadening scope.

## Stop Conditions / Phase Gates

- Stop if VK identity/display/persistence cannot be kept coherent or if work requires changing mouse policy, camera behavior, hook ownership, or unrelated state.
- No automatic game/runtime launch.

## Final Git Review

- Read-only status, scoped staged/unstaged diffs, diff summary and recent commit context.
- Compare changed paths with this plan; report completed, remaining, deferred, blocked, and not-runtime-validated separately.
- Record validation and limitations in `backlog/TASKLOG.md` after comparison.
