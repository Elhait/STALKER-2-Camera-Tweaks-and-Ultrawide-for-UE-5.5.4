# Overlay Cursor Restore Task Plan

## Objective

Restore the native game/system cursor when the overlay closes and ensure a
visible fallback during final game-window destruction.

## Established Evidence / Current State

- The user confirms ImGui's software cursor is visible and mouse movement no
  longer rotates the camera while the overlay is open.
- After closing the game, the native cursor remains hidden until the game
  finishes closing.
- The latest overlay log records repeated VISIBLE/HIDDEN toggles and ends in
  HIDDEN; it has no cursor-restoration or window-destruction event.
- Source intentionally calls SetCursor(nullptr) while the overlay is visible,
  but the hide path does not ask the original game window procedure to restore
  its cursor shape, and WM_NCDESTROY has no cursor fallback.

## Approved Scope

- On overlay hide, forward one synthetic WM_SETCURSOR event to the original game
  window procedure so the game reselects its expected cursor shape/state.
- On WM_NCDESTROY, restore a standard arrow cursor after forwarding destruction
  to the original procedure, then clear the subclass bookkeeping.
- Keep mouse/raw-input capture, ImGui software cursor rendering, and gameplay
  camera behavior unchanged.

## Explicit Non-Goals

- No ShowCursor counter manipulation or ClipCursor/SetCursorPos changes.
- No camera/FOV/runtime settings changes.
- No D3D12 render, resize, queue or resource lifecycle changes.
- No game launch, commit, release or unrelated cleanup.

## Expected Files / Areas

- src/overlay/discovery_runtime.cpp
- This plan, archived under research/completed/ after validation

## Batches

1. Restore the native cursor through the original game's WM_SETCURSOR handler
   when the overlay is toggled off.
2. Add a final WM_NCDESTROY cursor fallback and clear subclass state safely.
3. Run full test.cmd, build-overlay-settings.cmd, git diff --check, archive
   this plan and perform a read-only scoped Git review.

## Validation

- Full deterministic test.cmd.
- build-overlay-settings.cmd.
- git diff --check and scoped source/status review.
- Runtime is not performed by Codex; user will verify cursor return immediately
  after hiding the overlay and during game shutdown.

## Risks and Rollback / Safe-Failure

- A synthetic WM_SETCURSOR message could be interpreted differently by the game;
  use the original window procedure directly with standard client-area parameters
  and avoid recursive SendMessage dispatch.
- WM_NCDESTROY must call the original procedure exactly once before clearing the
  saved WNDPROC/window state.
- Rollback is limited to the cursor restoration and subclass teardown changes.

## Stop Conditions / Phase Gates

- Stop if reliable cursor restoration requires changing game input registration,
  global cursor counters or camera behavior.
- Stop after deterministic/build validation and final review. Do not launch the
  game.

## Expected Final Git Review

- Confirm only the cursor/window-procedure lifecycle path and archived plan are
  attributable to this batch.
- Preserve unrelated pre-existing dirty/untracked files.
- Report tests/build results and explicitly state runtime remains pending.
