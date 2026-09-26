# Task Plan — ProggyClean-Compatible Cyrillic Font Research

## Objective

Identify and visually compare a small set of legally embeddable Cyrillic font
candidates that pair more naturally with the existing ProggyClean UI font.

## Established Evidence

- User rates ProggyClean Latin rendering 10/10 and the current merged Cyrillic
  rendering 5/10; increasing the font size worsens the mismatch.
- The current production font path uses ImGui's built-in ProggyClean plus a
  system Segoe UI TTF merge for Cyrillic and supplemental punctuation.
- A bundled Noto Sans Mono TTF and its SIL OFL 1.1 notice remain in the source
  tree from an earlier iteration; it is not accepted visually.
- User authorizes research and side-by-side specimens only; no production change.

## Approved Scope

- Inspect available bundled/system font files and source licenses.
- Research a few pixel/bitmap-oriented Cyrillic-capable candidates, including
  any credible ProggyClean Cyrillic expansion, using authoritative upstream
  sources for glyph coverage and license.
- Produce a disposable diagnostic specimen with identical sample text, pixel
  sizes, and raster settings for candidate comparison, if the local font tools
  can do so without altering production assets.
- Report candidate provenance, Ukrainian glyph coverage, license, visual
  observations, and unresolved limitations.

## Explicit Non-Goals

- No production source/config/resource/build changes and no ASI output.
- No font-size slider or rasterization behavior changes.
- No downloading into the project, committing/bundling fonts, or changing
  localization/camera behavior.
- No game launch, injection, or Git commands/inspection.

## Expected Areas / Artifacts

- Read-only inspection of `assets/fonts`, current `localization_font.cpp`, and
  relevant vendored ImGui font APIs.
- Any generated specimen is temporary/outside production source and clearly
  labeled as research output.
- This plan is archived under `research/deferred/` after the candidate survey;
  the required same-ImGui specimen is pending a suitable disposable rendering
  path and is not represented as complete.

## Batches and Validation

1. Inventory existing font assets, installed rendering tools, glyph coverage,
   and current ImGui rasterization path. Validate by recording exact names and
   glyph coverage for `І Ї Є Ґ і ї є ґ`.
2. Search authoritative upstream projects for 3–5 suitable candidates and
   confirm licensing/glyph coverage from their own documentation/font metadata.
3. Render a controlled specimen at 13, 14, and 16 px with the same sample
   phrases and pixel-snap/oversampling settings; verify output visually and
   compare dimensions/metrics, without modifying production.
4. Record candidate evidence and limitations. If an equivalent same-ImGui
   specimen cannot be produced safely with available tooling, archive the
   survey as deferred and report that specimen as the remaining step.

## Risks and Safe Failure

- TTF-to-pixel rasterization may not match a hand-authored bitmap font; report
  renderer/settings with each sample and avoid claiming equivalence from family
  names alone.
- A candidate's broad Cyrillic block may omit Ukrainian-specific letters; treat
  actual glyph checks as a hard gate.
- Font licenses vary; exclude candidates whose embedding terms are unclear or
  incompatible. If no faithful specimen method is available locally, report
  that instead of changing production tooling.

## Stop Conditions and Final Review

- Stop before any source/resource/config/ASI modification.
- Stop after candidate evidence and specimen are ready; user selects whether to
  proceed with a production integration task.
- Final review is filesystem/path-only; user prohibits all Git interaction.
