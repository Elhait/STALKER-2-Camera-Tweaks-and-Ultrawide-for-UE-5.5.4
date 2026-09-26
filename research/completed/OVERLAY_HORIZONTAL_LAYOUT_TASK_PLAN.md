# Overlay Horizontal Layout Task Plan

## Objective

Make overlay width, columns and text-dependent horizontal spacing respond to
the active integer font size, while preserving the 13 px reference geometry,
clamping to the viewport, and preventing content-driven window growth.

## Established evidence and current state

- Main window preferred width is currently clamped from `viewportWidth * 0.32`
  to 560-720 px and then constrained to one fixed width. It does not account
  for the active font size.
- The settings/runtime table is always split 45/55 of current content width;
  several label/control offsets and horizontal style paddings remain fixed.
- Window position is clamped after `Begin`, but currently only against viewport
  edges and not safe margins. This per-frame clamp can also handle width changes.
- Main window currently uses `AlwaysAutoResize` and camera-state content is
  rendered in the right column. The new preferred width must depend only on
  viewport and font-size inputs, never content measurement.
- `activeFontSizePixels_` is updated during synchronized atlas rebuild before
  the next ImGui frame, so deriving layout metrics from it can take effect in
  that same frame.
- Architecture and safety guidance were read. Implementation/testing guidance
  and `docs/code-style.md` are absent in this checkout.

## Approved scope

- Add pure, deterministic horizontal layout metric functions with a 13 px
  reference scale.
- Scale preferred width, safe-margin-aware width clamp, 45/55 columns with
  scaled minimum widths, horizontal style padding/spacing, and fixed label/button
  horizontal offsets.
- Clamp/re-clamp the window position with viewport safe margins after Begin;
  use nonnegative widths for narrow viewports.
- Add deterministic tests for 13/15/18/24 px, narrow viewport clamp, monotonic
  width growth, nonnegative columns, unchanged 13 px reference geometry, and
  independence from content width.
- Run `test.cmd` and rebuild the stable overlay ASI with
  `build-overlay-settings.cmd`; stop without launching the game.
- Do not inspect or modify Git.

## Explicit non-goals

- No font/rasterizer selection, text-size range/default, localization, camera,
  semantic UI, or vertical layout changes.
- No layout width derived from Camera State or other dynamic content.
- No user-configurable layout ratio/margins and no viewport-independent
  semantic values scaled by font size.
- No game launch or runtime visual claim.

## Files or areas expected to be touched

- `src/overlay/renderer_runtime.cpp`
- New `src/overlay/overlay_layout_metrics.hpp/.cpp`
- New `tests/overlay/overlay_layout_metrics_harness.cpp`
- `test.cmd`, `build-overlay-settings.cmd`
- `backlog/TASKLOG.md` and this plan, archived under `research/completed/`.

## Batches and validation

1. Implement pure layout formulas and deterministic harness. Verify reference,
   scale monotonicity, viewport clamps, safe position clamps and narrow columns.
2. Integrate metrics at render time using `activeFontSizePixels_`; horizontal
   style metrics are scaled from an unscaled baseline (not cumulatively), and
   fixed label/control offsets use the same factor. Existing content wrapping
   stays available; tooltip wrap remains tied to current font size.
3. Run `test.cmd`, then `build-overlay-settings.cmd`; review outputs and ensure
   only the approved files/artifact are changed. No game launch.
4. Archive the plan and record the completed build/test batch.

## Risks and rollback/safe-failure behavior

- A narrow viewport may not fit both scaled minimum columns. In that case split
  available width proportionally without negative widths; the viewport clamp
  wins over preferred width.
- ImGui style changes can accumulate if scaled repeatedly. Preserve a single
  unscaled horizontal baseline and derive every frame's values from it.
- Keep safe margins fixed in viewport units; do not multiply them by font size.
- If reference geometry changes at 13 px on a standard viewport, correct the
  formula before building the stable artifact.

## Stop conditions and phase gates

- Stop if matching the contract requires content measurement to drive window
  size or changes to unrelated camera/localization semantics.
- Stop after the full deterministic suite and successful stable overlay build.

## Expected final Git review

Git status/diff/history inspection is intentionally omitted, per the user's
explicit instruction.
