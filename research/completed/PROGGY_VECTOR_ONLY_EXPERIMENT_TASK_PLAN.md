# Proggy Vector-Only Experimental ASI Task Plan

## Objective

Build one isolated experimental ASI that uses the embedded Proggy Vector TTF
as the sole font source for all overlay text, with no ProggyClean default font
and no merged supplemental fonts. Keep the existing runtime Text size setting
available for user testing across sizes.

## Established evidence and current state

- Stable production uses built-in ProggyClean plus merged Proggy Vector
  Cyrillic and supplemental Segoe UI glyphs.
- The exact Proggy Vector Regular TTF and its full license are already present
  under `build-artifacts/experimental-font-ab/ProggyVector/`.
- Existing experimental ASIs and variant directories are preserved.
- A unique output path
  `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayIntegrationProggyVectorOnly.asi`
  was confirmed absent before work.
- Architecture and safety-invariant guidance were read. The referenced
  `docs/code-style.md` and `docs/assistant/{implementation,testing}-guidelines.md`
  do not exist in this checkout.

## Approved scope

- Add experimental-only font-loader source, resource script, and build script
  under `build-artifacts/experimental-font-ab/ProggyVectorOnly/`.
- Load the embedded Proggy Vector TTF as the base/only `ImFont` source using
  selected runtime pixel size and the same existing font size setting/API.
- Include English, Ukrainian Cyrillic, and needed UI punctuation/symbol ranges
  in that one font source; do not merge additional fonts.
- Embed the same font and full license resources in the experimental ASI.
- Build exactly one uniquely named experimental ASI. Do not edit production
  sources/resources, stable ASI, game installation, localization contents,
  layout, or camera behavior.
- Do not run the game. Do not run Git commands or inspect Git state.

## Explicit non-goals

- No changes to the stable font loader, stable ASI, font-size behavior/range,
  runtime slider, layout, localization, camera semantics, or input behavior.
- No additional font candidates or size-specific presets.
- No cleanup or replacement of prior experimental outputs.
- No validation after the build beyond capturing the command result and output
  path; stop at the requested build boundary.

## Files or areas expected to be touched

- `build-artifacts/experimental-font-ab/ProggyVectorOnly/localization_font.cpp`
- `build-artifacts/experimental-font-ab/ProggyVectorOnly/localization_resources.rc`
- `build-artifacts/experimental-font-ab/ProggyVectorOnly/build.cmd`
- `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayIntegrationProggyVectorOnly.asi`
- This plan, archived to `research/completed/` after the bounded build.
- `backlog/TASKLOG.md` for the completed experimental build record.

## Batches and validation

1. Implement isolated loader/resources/build script. Statically confirm this
   loader has one AddFontFromMemoryTTF call, uses MergeMode=false, and does not
   call AddFontDefault or any other font-loading function.
2. Compile the variant with the repository's existing Windows C++ toolchain and
   link to the unique output path. Confirm build success and the artifact exists
   at that path. Do not launch or inject it.
3. Archive this plan, add a factual task-log entry, and stop.

## Risks and rollback/safe-failure behavior

- Proggy Vector may lack glyphs outside its verified English/Ukrainian test
  coverage. Keep ranges bounded to ASCII/Latin-1, required Cyrillic and UI
  punctuation/symbol blocks; visual test remains with the user.
- A unique output avoids overwriting stable or prior experimental ASIs. If the
  compile/link fails, preserve all existing artifacts and report the failure.

## Stop conditions and phase gates

- Stop if build changes would require touching stable production source or
  output, or if the unique output path unexpectedly exists.
- After the experimental ASI build succeeds, archive the plan/log the factual
  result and stop. No game launch, runtime test or follow-up edits.

## Expected final Git review

Git review/state inspection is intentionally omitted at the user's explicit
instruction. Review only the approved experimental paths and build result.
