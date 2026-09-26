# Task Plan — ProggyClean Cyrillic Companion Runtime A/B

## Objective

Produce two separately named experimental overlay ASIs for the user's visual
runtime comparison: ProggyClean + GohuFont Unicode Cyrillic, and ProggyClean +
Terminus Cyrillic. Do not select a winner or change the production font choice.

## Established Evidence and Current State

- The user rates ProggyClean Latin 10/10 and the current merged Cyrillic 5/10;
  scaling up worsens the visual mismatch.
- `src/overlay/localization_font.cpp` currently adds ImGui's default font, then
  merges Segoe UI at 13 px with `MergeMode`, `PixelSnapH`, and H/V oversampling 1.
- The existing UI's persisted default font-size value is 16, which maps to a
  global scale of 1.0. This experiment must not alter or use that global scale
  to tune either candidate.
- Terminus 4.49.1 upstream `ter-u14n.bdf` has 8x14 cells and code points for
  `А-Я а-я І і Ї ї Є є Ґ ґ`; its OFL 1.1 permits embedding. Exact source and
  license links are in the prior research report and upstream web evidence.
- GohuFont Unicode 14 upstream BDF coverage and WTFPL v2 were verified in the
  prior research report. The official page marks contributed TTF conversions
  unsupported; this task uses the upstream contributor's Unicode 14 TTF only
  as an explicitly identified runtime A/B conversion, not as a claim that its
  STB outline render is identical to the native BDF bitmap.
- Git is expressly prohibited by the user. No Git command/state inspection.

## Approved Scope

- Verify the exact chosen Terminus and Gohu resource files, provenance,
  licensing, file integrity and all required Cyrillic code points before build.
- Build two experimental variants with the same checked-in ImGui version,
  ProggyClean default font, production overlay sources, localization resources,
  compile flags, and user-facing/runtime behavior.
- Preserve ASCII/Latin from ProggyClean. Merge only the chosen supplementary
  Cyrillic range; do not let either candidate replace Latin glyphs.
- Keep the ProggyClean source/raster settings unchanged. Use both candidate
  Unicode TTFs at 14 px through the same ImGui STB loader and merge policy
  (`PixelSnapH=true`, oversampling H/V=1, same glyph offset). Do not use
  `FontGlobalScale` to tune. Terminus's upstream warning about antialiased
  outline rendering and Gohu's unsupported converted TTF must be called out in
  the handoff: this compares the two concrete ImGui-compatible TTF resources.
- Name the outputs unambiguously, e.g.
  `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayIntegrationGohu.asi`
  and `...Terminus.asi`.
- Run only the validation needed to verify source/catalog contracts and both
  experimental builds. Do not launch the game; the user performs the runtime
  A/B and supplies screenshots/results.

## Explicit Non-Goals

- No change to production font choice, production ASI, runtime font setting,
  localization/catalog content, camera/settings semantics, or input behavior.
- No installation of fonts, font sidecars, external localization files, or
  additional distributable artifacts beyond the two explicitly requested test
  ASIs and their research provenance/license notes.
- No game launch, injection, production build/output replacement, or Git
  commands/state inspection.
- No size sweep: only the 14 px candidate comparison in this batch.

## Expected Files or Areas

- Candidate inputs and license notices only beneath
  `build-artifacts/experimental-font-ab/` (ignored/generated experiment area).
- Experiment-only build source/configuration beneath that same directory; do
  not edit `src/overlay/localization_font.cpp` or the production build scripts.
- Two named ASI outputs beneath the experiment directory.
- A concise result note under `research/reports/` and this plan archived under
  `research/completed/` only if both artifacts build and the scoped filesystem
  review passes; otherwise archive under `research/deferred/` with blocker.

## Batches and Validation

1. **Candidate gate:** obtain the exact official Gohu Unicode 14 and Terminus
   4.49.1 14 px resources; verify checksums/provenance, licenses and required
   code points, including `А-Я а-я І і Ї ї Є є Ґ ґ`. Stop before source/build
   work if a candidate file or license/coverage is uncertain.
2. **Isolated build path:** prepare build-artifact-only experiment inputs and
   the smallest isolated source/config needed for each variant. Verify the
   Latin/default glyph source remains ImGui ProggyClean and supplementary
   glyph ranges exclude Latin/ASCII. Verify no production source/build-script
   edit is needed.
3. **Build:** compile each variant with otherwise matching overlay and
   localization sources/flags into the two distinct ASI paths. Do not overwrite
   any existing root ASI. Check both output files, timestamps/sizes and embedded
   resource inventory; record the compiler result for each candidate.
4. **Review and handoff:** audit only filesystem paths against this plan,
   preserve source/license provenance beside experiment artifacts, archive the
   plan and report exact artifact paths. Stop; user runs visual comparison.

## Risks and Safe Failure

- Gohu's upstream source is BDF and its contributor TTF conversion is marked
  unsupported. The requested A/B may proceed only with the exact identified
  conversion, clearly reported; it must not be presented as a verdict on the
  native BDF rendering.
- Terminus TTF may render outlines with antialiasing unlike its bitmaps. This
  test is a matched ImGui TTF-loader comparison, not a raw bitmap renderer
  comparison; do not claim otherwise.
- If common rasterization settings cannot be used without individually tuning a
  candidate, preserve the baseline, document the mismatch, and stop before
  producing a misleading A/B pair.
- Never overwrite production or existing root ASIs. Any candidate failure
  leaves the previous artifact(s) intact and is reported as incomplete.

## Stop Conditions and Final Review

- Stop if exact source/coverage/license checks fail, if an isolated experiment
  requires production-path edits, if Latin source/raster settings change, or if
  either build fails.
- Do not launch the game or claim visual success. Only the user's runtime
  screenshots establish which candidate looks better in context.
- Final review is filesystem/path-only; no Git interaction of any kind.
