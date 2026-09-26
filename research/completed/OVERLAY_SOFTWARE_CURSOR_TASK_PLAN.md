# Overlay Software Cursor Task Plan

## Objective

Make the overlay cursor visible during gameplay even when the game continuously
hides the native Windows cursor, without changing the already-validated
mouse-only input capture behavior.

## Established Evidence / Current State

- The latest runtime session confirms that overlay mouse input no longer moves
  the gameplay camera.
- The latest STALKER2CameraTweaksOverlayDiscovery.log repeatedly reports
  OVERLAY_CURSOR_VISIBILITY_RETRY_LIMIT while the overlay is visible; the
  native cursor therefore remains hidden despite repeated ShowCursor(TRUE).
- The ImGui Win32 backend supports its own rendered mouse cursor through
  ImGuiIO::MouseDrawCursor; mouse movement messages already reach the backend
  while the overlay is visible.
- The preceding fix for startup-disabled Gameplay and mouse-only raw-input
  capture is runtime-confirmed by the user and is out of scope here.

## Approved Scope

- Replace repeated native cursor visibility forcing with ImGui's software-drawn
  cursor while the overlay is visible.
- Ensure the software cursor draw flag is updated on the renderer/present thread
  to avoid adding cross-thread ImGui access from the window procedure.
- Preserve keyboard passthrough, raw mouse interception, Insert toggle, and
  hidden-at-startup behavior.
- Remove retry-limit logging and the ShowCursor counter adjustments if no
  longer needed.

## Explicit Non-Goals

- No changes to camera/FOV, Gameplay.Enabled or runtime settings behavior.
- No changes to mouse/raw-input capture policy or key bindings.
- No game/window clipping or cursor-position manipulation.
- No D3D12 lifecycle, rendering-resource or resize changes.
- No game launch, commit, release, or unrelated cleanup.

## Expected Files / Areas

- src/overlay/discovery_runtime.cpp
- src/overlay/renderer_runtime.cpp and possibly its header if needed
- Existing overlay input/render harnesses only if a meaningful deterministic
  invariant can be tested without introducing a fake ImGui runtime
- This plan, archived under research/completed/ after validation

## Batches

1. Remove native ShowCursor counter manipulation and retry logging.
2. Set ImGui software cursor mode on the render/present thread according to
   overlay visibility; preserve existing window-message behavior.
3. Run relevant harnesses, test.cmd, build-overlay-settings.cmd, and
   git diff --check; archive this plan and perform read-only Git review.

## Validation

- Existing overlay input/render harnesses and full test.cmd.
- build-overlay-settings.cmd.
- git diff --check and scoped diff/status review.
- Runtime: not performed by Codex; user will verify cursor visibility in
  gameplay and confirm mouse camera movement remains blocked.

## Risks and Rollback / Safe-Failure

- If ImGui does not receive usable mouse coordinates, its software cursor may
  not track the pointer; retain existing mouse event delivery and report this
  as requiring runtime validation rather than restoring counter manipulation.
- Software cursor rendering must remain limited to visible overlay frames.
- Rollback is limited to the cursor presentation changes; raw-input capture and
  camera behavior remain untouched.

## Stop Conditions / Phase Gates

- Stop if the cursor solution requires changing D3D12 submission/resize logic,
  cursor clipping, or gameplay camera behavior.
- Stop after deterministic/build validation and final read-only review. Do not
  launch the game.

## Expected Final Git Review

- Confirm source changes are limited to the cursor presentation path and this
  archived plan.
- Preserve unrelated pre-existing dirty/untracked files.
- Report validation limits and leave runtime behavior explicitly unvalidated.
