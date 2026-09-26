# Overlay Content-Driven Column Width Task Plan

## Objective
Replace the font-size-scaled, fixed-ratio overlay geometry with independently content-sized configuration/runtime columns, preserving a viewport-only safety cap and stable window positioning.

## Established evidence and current state
- `renderer_runtime.cpp` scales horizontal ImGui style metrics each frame using font-size / 13.
- `overlay_layout_metrics.cpp` clamps the preferred window width using a 560–720 px reference range, then scales that width by font size; the 720 px cap explains the observed ~680 px content width after window padding.
- The settings/runtime table is explicitly split 45/55 with fixed-width columns.
- Several controls use 72% of available width, creating widths based on the enclosing column rather than their labels/options.
- `AlwaysAutoResize` and per-frame position clamping can make content-width changes appear as window movement.
- The font atlas is rebuilt when the persisted font-size setting changes. Slider edits currently persist on every changed integer value, so dragging can trigger repeated expensive atlas rebuilds.
- Existing deterministic layout tests encode the rejected scale/fixed-split behavior and must be replaced.
- Relevant implementation/testing rules were read from workspace `docs/assistant`; `docs/code-style.md` is absent.

## Approved scope
- Rework only overlay horizontal sizing, column sizing/padding, window position clamping on geometry changes, and font-size slider rebuild cadence as needed to remove repeated atlas work during a drag.
- Allow independent locale-dependent content widths with approximately 5–10 px inner column padding.
- Cap the total window only at the usable viewport width; at that limit, long/dynamic content may wrap.
- Add/update deterministic tests for layout width calculations and run `test.cmd` plus the overlay build.

## Explicit non-goals
- No camera behavior, localization strings/semantics, font selection, rasterizer, default font size, vertical layout redesign, stable/release packaging, or runtime-game testing.
- No Git commands or Git state changes, per user request.
- No content-driven expansion beyond available viewport bounds.

## Expected files/areas
- `src/overlay/renderer_runtime.cpp`
- `src/overlay/overlay_layout_metrics.hpp` and `.cpp`
- `tests/overlay/overlay_layout_metrics_harness.cpp`
- `test.cmd`
- `OVERLAY_CONTENT_WIDTH_TASK_PLAN.md` (archive after completion)
- `backlog/TASKLOG.md` only after relevant validation/build succeeds.

## Batches and validation
1. Replace scaled/fixed preferred geometry with pure helpers for natural column widths, per-column padding, viewport cap and stable clamp; replace corresponding tests. Validate helper harness and full `test.cmd`.
2. Integrate content-based sizing into renderer; remove horizontal scaling and fixed proportional table columns; size controls from intrinsic text/options and avoid repeated font atlas rebuilds while the font-size slider is being dragged. Validate compilation and full `test.cmd`.
3. Build via `build-overlay-settings.cmd`; verify successful output and compare touched areas against this plan. Stop after build; runtime behavior remains unvalidated until user checks in-game.

## Risks and safe failure
- Intrinsic widths can become excessive due to long localized labels or expanded runtime details; cap to viewport usable width and let existing text wrapping handle the constrained case.
- Variable content may fluctuate frame to frame; compute stable content width from static/localized UI strings and selected controls, not continuously changing camera telemetry values. Dynamic runtime text wraps inside its assigned column.
- Deferring font rebuild until slider release means previewed value updates at release rather than rebuilding for every intermediate integer; retain the currently active atlas until the final selected size is committed.
- If sizing produces non-finite/negative/degenerate column widths, clamp safely to zero/available width.

## Stop conditions and phase gates
- Stop if the implementation requires changes to camera/localization semantics, new dependencies, or unrelated layout redesign.
- Stop on failing relevant tests or build; report the exact failure and do not claim runtime validation.
- After successful tests/build, stop without launching the game.

## Final review
- User explicitly forbids Git operations; omit Git status/diff/recent-commit inspection and do not touch Git.
- Review changed file paths and diffs read-only against this plan, record build/test results and runtime-validation limits in the task log, then archive this plan to `research/completed/`.
