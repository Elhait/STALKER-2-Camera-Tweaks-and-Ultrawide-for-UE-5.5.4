# Overlay Cursor Visibility Gating Task Plan

## Objective

Enforce the lifecycle invariant that overlay cursor takeover and cursor-rendering
work occur only while the overlay is open, with one-shot restoration only when
closing the overlay or destroying the game window while the overlay is open.

## Established Evidence / Current State

- User confirmed the ImGui software cursor works and raw mouse capture prevents
  gameplay camera movement.
- User requires all overlay/cursor behavior to be strictly gated by overlay
  visibility.
- Current Renderer::Render updates ImGui MouseDrawCursor on every Present,
  including hidden frames.
- Current WM_DESTROY/WM_NCDESTROY path unconditionally sets the arrow cursor,
  including when the overlay has remained hidden.
- Overlay visibility is already atomic in Renderer; WndProc and Present are the
  existing visibility boundaries.

## Approved Scope

- Make hidden Renderer::Render return without touching ImGui cursor state.
- Enable ImGui software cursor only on visible render frames.
- Restore the original game's cursor once when the user closes the overlay.
- On window destruction, perform cursor restoration only if the overlay was
  still open; continue clearing subclass bookkeeping at WM_NCDESTROY.
- Preserve current mouse-only raw-input capture, keyboard passthrough, camera
  behavior, and Insert toggle.

## Explicit Non-Goals

- No per-frame cursor forcing, ShowCursor counter calls, ClipCursor, or
  SetCursorPos.
- No camera/FOV/runtime settings changes.
- No D3D12 resource, queue, resize, or synchronization changes.
- No game launch, commit, release, or unrelated cleanup.

## Expected Files / Areas

- src/overlay/discovery_runtime.cpp
- src/overlay/renderer_runtime.cpp
- This plan, archived under research/completed/ after validation

## Batches

1. Gate software cursor state changes behind visible rendering.
2. Gate teardown cursor restoration on the overlay-open state and retain
   one-shot restoration on explicit close.
3. Run test.cmd, build-overlay-settings.cmd, git diff --check, archive the plan,
   and perform a read-only scoped Git review.

## Validation

- Full deterministic test.cmd.
- build-overlay-settings.cmd.
- git diff --check and source/status review.
- Runtime remains for the user: show/hide cursor behavior, cursor state while
  overlay is hidden, and exit with overlay both open and closed.

## Risks and Rollback / Safe-Failure

- Avoid reading or mutating ImGui context from the WndProc thread; cursor draw
  state remains set only on the render thread while the overlay is visible.
- Restore the game's cursor only at the visibility-close edge, not continuously.
- Rollback is limited to visibility-gating changes in these two source files.

## Stop Conditions / Phase Gates

- Stop if the change requires global cursor counter manipulation, input-hook
  expansion, or camera behavior changes.
- Stop after deterministic/build validation and read-only review. Do not launch
  the game.

## Expected Final Git Review

- Confirm source changes are limited to cursor visibility gating and this plan.
- Preserve unrelated pre-existing dirty/untracked work.
- Report validation and leave the requested runtime matrix pending.
