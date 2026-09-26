# Task Plan: Overlay Mouse-Only Capture and Hidden Startup

## Objective

Make the overlay start hidden. While visible, route mouse interaction to the
overlay and keep ordinary keyboard input flowing to the game, with Insert
remaining the overlay visibility toggle.

## Established Evidence and Current State

- `InputState` currently starts visible and classifies both mouse and keyboard
  messages for capture.
- `Renderer` also defaults to visible.
- The window procedure sends messages to ImGui and suppresses messages selected
  by `InputState::ShouldCapture`.
- The existing overlay input harness checks the current defaults and routing.
- The overlay settings controls are mouse-operated; game keyboard control should
  remain available while the overlay is open.

## Approved Scope

- Start both input state and renderer visibility as hidden.
- While visible, capture mouse-button, mouse-move, wheel, and X-button messages
  for overlay use; always forward ordinary keyboard messages to the game.
- Continue consuming Insert key-up as the overlay toggle in either visibility
  state.
- Update deterministic input-state harness coverage for startup, mouse capture,
  keyboard passthrough, and repeated toggle behavior.

## Explicit Non-Goals

- No changes to camera/gameplay/cinematic/dialogue behavior or Runtime Settings
  API.
- No changes to D3D12 rendering, resize synchronization, cursor policy, or input
  capture outside the overlay window procedure.
- No change to Insert binding or persistence.
- No game launch, commit, release, or unrelated cleanup.

## Expected Files / Areas

- `src/overlay/input_state.hpp`
- `src/overlay/input_state.cpp`
- `src/overlay/discovery_runtime.cpp`
- `src/overlay/renderer_runtime.hpp`
- `tests/overlay/input_state_harness.cpp`
- This plan, to be archived under `research/completed/` after validation.

## Batches

1. Change input policy to visible-only mouse capture and hidden initial state;
   synchronize renderer visibility with the input state after initialization.
2. Update harness expectations and validate the focused harness, full test suite,
   and overlay settings build.
3. Archive this plan and perform a read-only final Git/diff review.

## Validation

- Focused overlay input-state harness.
- `test.cmd`
- `build-overlay-settings.cmd`
- `git diff --check`
- Read-only review for scope and preservation of pre-existing working-tree edits.
- Runtime: not performed.

## Risks and Rollback / Safe-Failure

- Risk: keyboard messages could be swallowed by ImGui or an unrelated input
  policy. Mitigation: the window procedure must always forward non-Insert
  keyboard messages to the original game procedure, regardless of ImGui capture
  flags.
- Risk: renderer and input visibility could start out of sync. Mitigation:
  initialize both hidden and set renderer visibility from `InputState` after
  successful renderer initialization.
- Rollback: revert only the scoped input-state, visibility, and harness changes;
  no persistent game or camera state is altered.

## Stop Conditions and Phase Gates

- Stop if mouse-only routing requires modifying game-level input hooks or cursor
  capture outside the overlay window procedure.
- Stop after deterministic/build validation and final review. Do not launch the
  game.

## Expected Final Git Review

- Confirm only the planned overlay input/visibility files and archived plan are
  attributable to this task.
- Preserve all pre-existing dirty and untracked user work.
- Report validation results and state clearly that runtime was not performed.
