# Task Plan — Restore ProggyClean and Merge Cyrillic

## Objective

Restore Dear ImGui's original built-in ProggyClean as the primary UI font and
merge only Cyrillic plus catalog-required non-Latin punctuation from a system
font, then build one runtime A/B test ASI.

## Established Evidence

- Historical localization planning records that the pre-localization renderer
  relied on Dear ImGui's default font.
- The checked-in ImGui v1.91.9b default path embeds ProggyClean at 13 px and, when
  called without an override config, uses `OversampleH=1`, `OversampleV=1`, and
  `PixelSnapH=true`.
- The later bundled Noto Sans Mono and then Segoe UI paths replaced the full
  glyph set and used a different rasterization configuration. The user reports
  both runtime variants are poor and explicitly requests a narrow ProggyClean +
  Cyrillic merge experiment.
- Current system Segoe UI and Arial font files are available; both catalogs are
  embedded and the user requires no font sidecar.
- User prohibits all Git operations. No Git command or state inspection will be
  performed.

## Approved Scope

- Call ImGui `AddFontDefault()` without a custom config to retain its exact
  built-in ProggyClean rasterization settings.
- Merge only Cyrillic ranges and the non-Latin punctuation/symbol ranges required
  by the catalogs from Segoe UI, with `MergeMode=true`, `PixelSnapH=true`, and
  horizontal/vertical oversampling 1.
- Keep the current `[Overlay] FontSize` API/UI and `FontGlobalScale` logic
  unchanged. For this A/B build, the default configured value keeps scale at
  1.0, so the base ProggyClean font is exactly 13 px; do not redesign sizing in
  this batch.
- Remove only the no-longer-used font resource ID/reference; preserve source
  assets. Keep both locale JSON resources embedded.
- Add deterministic assertions for ProggyClean base size, merged glyph coverage,
  and two-source atlas; run `test.cmd` and `build-overlay-settings.cmd`.
- Do not launch the game; provide the ASI for user visual comparison.

## Explicit Non-Goals

- Rebuilding the atlas when the size slider changes or changing its range,
  default, persistence, or UI.
- Camera/runtime semantics, localization copy, font downloads, font files beside
  the ASI, deletion of source font assets, or other UI changes.
- Any Git operation or agent-run game launch/injection.

## Expected Files

- `src/overlay/localization_font.hpp/.cpp`,
  `src/overlay/localization_resource_ids.h`, `renderer_runtime.cpp`.
- `tests/overlay/localization_harness.cpp`,
  `tests/runner/localization_catalog_audit.ps1`, `backlog/TASKLOG.md`.
- Production ASI and this archived plan.

## Validation and Stop Conditions

- Ensure default Latin glyphs remain from the first ProggyClean source and
  required Ukrainian/non-Latin glyphs exist after merge; no Latin range is
  requested from the supplementary TTF.
- Run catalog/resource audit, all deterministic tests, and overlay production
  build. Any atlas/glyph/test/build failure stops delivery.
- Compare affected paths to scope without Git; archive the plan and task-log the
  bounded experiment. The final text appearance remains runtime-unvalidated
  until the user's screenshot.

## Risks and Safe Failure

- Supplementary glyph metrics may not blend perfectly with ProggyClean; the
  purpose of the build is a visual A/B check. Keep its glyph ranges narrow and
  raster settings pixel-snapped/unsampled.
- If a system font is unavailable or the merge fails, fail font initialization
  rather than replacing all Latin glyphs or silently falling back to another
  font.
- The current size scale remains as-is; do not infer that the separate future
  integer atlas-resize design has been validated.

## Git Review

The user prohibits all Git interaction; perform only a read-only filesystem path
review against this plan.
