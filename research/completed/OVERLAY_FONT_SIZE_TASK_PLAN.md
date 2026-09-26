# Task Plan — Adjustable Overlay Font Size

## Objective

Improve legibility of the bundled Unicode font by raising its default size and
adding a user-adjustable, persistent font-size control in the overlay.

## Established Evidence and Current State

- Runtime screenshots confirmed that both English and Ukrainian remain readable
  functionally but the bundled Noto Sans Mono appearance is substantially worse
  than the prior UI, and the current font is loaded at 13 px.
- The font is embedded in the ASI as an RCDATA resource and covers the glyphs
  required by the current English/Ukrainian catalogs.
- The renderer owns the ImGui context and initializes the font atlas; locale and
  other presentation settings already use `[Overlay]` config and the typed
  runtime-settings API.
- Config persistence uses the established staged write path. The build and test
  entry points are `build-overlay-settings.cmd` and `test.cmd`.
- The user explicitly prohibits Git operations. No Git commands or Git state
  inspection will be used.

## Approved Scope

- Increase the bundled font's baked default from 13 px to 16 px.
- Add a font-size control with a bounded integer range of 12–24 px, default 16,
  persisted as `[Overlay] FontSize=16` and applied live without rebuilding the
  atlas.
- Reuse the existing font atlas and scale its font globally relative to the
  16 px baked size, so changes take effect immediately and layout follows text.
- Validate/default invalid or missing values safely, maintain config template
  synchronization, add deterministic config/runtime/rendering coverage, run the
  full test suite and production overlay build.
- Deliver a fresh ASI for the user's visual check. Do not launch the game.

## Explicit Non-Goals

- Changing font family, language catalogs/translations, glyph sets, theme,
  overlay width/position, camera/runtime behavior, or mouse/input behavior.
- Font-atlas rebuilding, new external assets/dependencies, Auto locale detection,
  or adding locales.
- Any Git operation or automatic game launch/injection.

## Expected Files or Areas

- `src/overlay/localization_font.cpp`, `src/overlay/renderer_runtime.cpp`.
- `src/config/feature_config.hpp/.cpp`, `src/config/config_repository.cpp`,
  `src/config/config_template.cpp`.
- `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`.
- `tests/config/config_persistence_harness.cpp`,
  `tests/config/runtime_settings_api_harness.cpp`,
  `tests/overlay/localization_harness.cpp` and, if needed, a focused renderer
  settings harness/source audit.
- `test.cmd`, `build-overlay-settings.cmd`, `[Overlay]` config template,
  `backlog/TASKLOG.md`, and the production ASI output.
- This plan, archived under `research/completed/` after successful validation.

## Implementation Batches and Validation

### Batch 1 — Config and runtime setting

- Add bounded `OverlayFontSize` config setting, default 16, with `FontSize=16`
  in new/synchronized configuration templates.
- Add typed mutation/snapshot/persistence while keeping the value presentation-
  only and clamping/rejecting out-of-range values safely.
- Validation: config persistence covers default, invalid/missing, boundaries,
  round-trip persistence, and template preservation; runtime API checks mutation
  and snapshot.

### Batch 2 — Font/UI integration

- Bake Noto Sans Mono at 16 px and apply `ImGuiIO::FontGlobalScale` from the
  persisted runtime value relative to 16 px on the render thread.
- Add an immediately adjustable 12–24 px slider in the Overlay section.
- Validation: focused source/resource audit confirms one font atlas/resource
  and bounded dynamic scaling; localization font harness continues to verify
  English and Ukrainian glyph coverage.

### Batch 3 — Full validation and artifact

- Run catalog/source audit, all `test.cmd` harnesses, and
  `build-overlay-settings.cmd`.
- Confirm the resulting ASI exists and is the only intended deployment artifact
  produced for handoff. Do not launch the game.
- Review touched paths against this plan without Git, update the task log, and
  archive this plan.

## Risks and Rollback / Safe-Failure Behavior

- ImGui global font scaling affects text and font-relative widget sizing but
  does not rebuild the atlas; this avoids resource churn and applies changes on
  the next frame. Existing wrapping must remain bounded by the overlay width.
- Missing or invalid config values resolve to 16 px; runtime mutations outside
  12–24 are rejected. Config persistence failures preserve the last-known-good
  file through the existing staged write path.
- Larger text may increase overlay height and cause more wrapped copy. Do not
  widen the window based on content; rely on current wrapping/scroll behavior.
- If tests/build fail, stop and retain the prior ASI; do not launch the game.

## Stop Conditions and Phase Gates

- Stop if scaling changes font glyph coverage, breaks current responsive width
  constraints, or requires font-atlas/resource lifetime changes.
- Stop on any deterministic test failure or production build failure.
- Successful build completes the implementation batch but is not visual runtime
  proof; pause for the user's in-game screenshot/feedback.

## Expected Final Git Review

The user expressly prohibits all Git interaction, so no Git review or Git-state
inspection will be performed. Final review will compare filesystem paths with
the approved scope and report validation limits.
