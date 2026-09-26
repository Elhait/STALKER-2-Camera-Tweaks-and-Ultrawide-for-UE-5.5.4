# Task Plan: Runtime Enable from Disabled Config and Gameplay Mouse Capture

## Objective

Repair two runtime-confirmed overlay issues:

1. Allow `Gameplay.Enabled` to be enabled from the overlay when configuration
   starts with Gameplay disabled.
2. While the overlay is visible, show/use the mouse cursor and prevent both
   ordinary mouse messages and raw mouse input from reaching the game, while
   forwarding ordinary keyboard input to the game. Keep the overlay hidden at
   process startup and retain Insert as its toggle.

## Established Evidence and Current State

- Production log starts with `Gameplay.Enabled=0` and reports the Gameplay hook
  was bypassed. Each checkbox click is rejected with
  `reason=gameplay_hook_unavailable`.
- Initialization currently installs the camera-writer hook only when Gameplay
  correction is configured on.
- The overlay window procedure captures standard mouse messages but does not
  classify `WM_INPUT`, handle cursor visibility, or override `WM_SETCURSOR`.
- In gameplay, the game uses mouse input that continues to rotate the camera
  while the overlay is visible; its cursor is hidden.
- Keyboard passthrough and hidden-at-startup behavior were implemented in the
  preceding batch and must remain intact.

## Approved Scope

- Install the validated camera-writer hook as an observation/recovery capability
  even when the initial `Gameplay.Enabled` value is false; keep the correction
  disabled until the user enables it.
- Keep feature status and intervention state distinct: disabled config remains
  disabled, while an available observer capability allows runtime enabling and
  dependent lifecycle observation.
- Classify `WM_INPUT` by raw-input type and capture only mouse input while the
  overlay is visible; perform default cleanup for captured mouse events without
  forwarding them to the game's window procedure. Raw keyboard input passes through.
- Ensure a visible system cursor while the overlay is open, prevent the game's
  `WM_SETCURSOR` handling from hiding/replacing it, and balance cursor visibility
  adjustments when the overlay closes.
- Preserve forwarding of non-Insert keyboard messages to the game.
- Extend deterministic coverage for `WM_INPUT`, cursor/input policy where
  feasible, and runtime capability/disabled-start behavior.

## Explicit Non-Goals

- No Gameplay mode, FOV math, AspectRecalculation, Dialogue, Cinematic, or
  camera-state behavior changes beyond making the existing validated observer
  hook available for runtime activation.
- No key capture, keyboard blocking, remapping, or change to the Insert binding.
- No permanent game input registration changes, injected game API, or global
  input hooks outside the game's existing HWND procedure.
- No changes to D3D12 queue/render/resize lifecycle.
- No game launch, commit, release, or unrelated cleanup.

## Expected Files / Areas

- `src/plugin/runtime.cpp`
- `src/overlay/input_state.hpp`
- `src/overlay/input_state.cpp`
- `src/overlay/discovery_runtime.cpp`
- `tests/overlay/input_state_harness.cpp`
- `tests/feature_status/feature_status_harness.cpp` if a pure capability-policy
  test is needed
- This plan, to be archived under `research/completed/` after validation.

## Batches

1. Separate Gameplay correction enablement from camera-writer observer-hook
   availability; support initial disabled configuration without applying the
   correction.
2. Add visible cursor lifecycle and capture foreground raw mouse input while
   preserving keyboard passthrough and startup-hidden behavior.
3. Add/update deterministic tests; run focused/full harnesses, overlay settings
   build, and diff validation.
4. Archive this plan and perform final read-only Git/diff review.

## Validation

- Focused Gameplay capability and overlay input harnesses.
- `test.cmd`
- `build-overlay-settings.cmd`
- `git diff --check`
- Read-only review of changed paths and preserved pre-existing user work.
- Runtime: not performed; user will validate the produced overlay build.

## Risks and Rollback / Safe-Failure

- Risk: observer-only camera hook accidentally applies correction while
  disabled. Mitigation: keep `runtimeGameplayEnabled=false` authoritative and
  verify the callback's disabled path passes camera writes through unchanged.
- Risk: capturing all `WM_INPUT` would also block raw keyboard input, while
  dropping captured mouse input without cleanup could leak raw-input state.
  Mitigation: inspect only `RAWINPUTHEADER`, capture `RIM_TYPEMOUSE`,
  forward other/unknown raw-input types, and call the default procedure for the
  captured mouse event without invoking the game's custom procedure.
- Risk: unbalanced `ShowCursor` calls affect gameplay cursor visibility after
  closing the overlay. Mitigation: track only visibility increments made by the
  overlay and undo those exact increments on hide; avoid cursor clipping changes.
- Rollback: revert only the scoped observer-gating/input/cursor changes; no
  persistent camera or configuration state is modified by this patch.

## Stop Conditions and Phase Gates

- Stop if observer-only installation cannot be made distinct from correction
  intervention without changing camera behavior.
- Stop if raw-input cleanup or cursor balancing requires broad input interception
  beyond the game's window procedure.
- Stop after deterministic/build validation and final review. Do not launch the
  game.

## Expected Final Git Review

- Confirm only planned runtime/input/test files and the archived plan are
  attributable to this batch.
- Preserve all pre-existing dirty and untracked user work.
- Report tests/build/diff results, note runtime was not performed, and identify
  any remaining cursor or runtime integration uncertainty.
