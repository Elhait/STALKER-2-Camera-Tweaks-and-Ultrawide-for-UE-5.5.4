# Stable Font Default Task Plan

## Objective

Set the default overlay font size to 15 px for fresh/default configuration
paths while keeping the accepted ProggyClean + Proggy Vector Cyrillic font,
preserving the existing user's saved size, and leaving font-choice diagnostic
code available but absent from the stable UI.

## Established evidence and current state

- Production `Renderer::BuildImGui` loads `AddProggyCleanWithCyrillic`, and the
  production RCDATA resource embeds Proggy Vector. This is the combination the
  user selected from the playground.
- The Font selector code is compiled only under
  `OVERLAY_FONT_PLAYGROUND_EXPERIMENT`; the normal stable build does not expose
  it. The experiment sources remain available.
- Current fresh/default size is 16 in `OverlayFontSizeDefault`, generated INI,
  managed template, and its persistence harness expectation.
- Preserve existing persisted user values; only missing/default values change.

## Approved scope

- Change the default overlay text size from 16 px to 15 px across config
  defaults/templates and the directly affected test expectation.
- Run the relevant repository tests and rebuild the stable overlay ASI with
  `build-overlay-settings.cmd`.
- Keep the selected production font, font selection code (experimental-only),
  font-size range, UI layout, language/localization behavior and camera code
  unchanged.

## Explicit non-goals

- Do not alter or persist font-choice selection.
- Do not change existing users' saved `FontSize` values.
- Do not modify camera, localization semantics, glyph ranges, rasterizer,
  layout, or the experimental playground implementation.
- Do not launch the game or perform runtime visual validation.
- Do not inspect or modify Git state.

## Files or areas expected to be touched

- `src/config/feature_config.hpp`
- `src/config/config_template.cpp`
- `src/config/config_repository.cpp`
- `tests/config/config_persistence_harness.cpp`
- Stable build output `STALKER2CameraTweaksOverlayIntegration.asi`
- `backlog/TASKLOG.md` and this plan, archived under `research/completed/`.

## Batches and validation

1. Update default values/comments and the focused persisted-template test.
   Run `test.cmd` and localization audit via the stable build.
2. Build the stable overlay ASI with `build-overlay-settings.cmd`. Verify the
   artifact path and successful build. Do not launch the game.
3. Archive this plan and record the results. Do not inspect Git.

## Risks and rollback/safe-failure behavior

- Existing persisted font sizes must remain untouched. Only absent/invalid
  values should resolve to 15.
- If tests reveal that the default is used for an unrelated contract, stop and
  revise scope rather than changing the supported size range or migration
  behavior.
- Build output is the stable overlay frontend artifact; build failure must be
  reported without claiming delivery.

## Stop conditions and phase gates

- Stop if production font choice or visible selector requires additional
  changes; source inspection shows both are already in the requested state.
- Stop after tests and successful stable overlay build. No game launch.

## Expected final Git review

Git state and diffs are intentionally not inspected or changed, per the user's
explicit instruction.
