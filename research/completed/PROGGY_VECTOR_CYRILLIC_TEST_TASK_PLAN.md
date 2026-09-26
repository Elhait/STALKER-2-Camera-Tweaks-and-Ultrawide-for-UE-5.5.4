# Proggy Vector Cyrillic Companion Test Plan

## Objective

Check whether the official Proggy Vector font contains the required Ukrainian
glyphs and, if it does, build one isolated experimental ASI that keeps the
built-in ProggyClean for Latin while using Proggy Vector only for Cyrillic.

## Established evidence and current state

- User supplied `C:\Users\enton\Downloads\proggyfonts-master.zip`.
- The archive contains `ProggyVector/ProggyVector-Regular.ttf` and
  `ProggyVector/ProggyVector_Licence.txt`.
- The official repository provides the TTF and a license permitting use and
  distribution in software with copyright/license notices retained.
- Existing Gohu and Terminus A/B ASIs are isolated under
  `build-artifacts/experimental-font-ab/`; the user found them readable but
  disliked their visual style.
- Production uses ImGui's built-in ProggyClean. No production change is needed
  for this comparison.

## Approved scope

- Extract only the official Proggy Vector regular TTF and its license into the
  existing experimental font-test area.
- Verify exact glyph coverage for `І Ї Є Ґ і ї є ґ` (and record the complete
  Ukrainian alphabet range check).
- If coverage passes, add an isolated build variant and produce one ASI using
  ProggyClean for Latin and Proggy Vector only for Cyrillic, retaining the
  same rasterization and overlay settings as the previous A/B tests.
- Keep the artifact and any test-only support files under
  `build-artifacts/experimental-font-ab/`.

## Explicit non-goals

- No production source/font/config changes and no replacement of the current
  production ASI.
- No camera/runtime/localization behavior changes.
- No game launch or injected runtime test; user performs the visual test.
- No more font research or extra candidate builds in this batch.
- No Git commands or Git state inspection.

## Expected files/areas

- `build-artifacts/experimental-font-ab/ProggyVector/ProggyVector-Regular.ttf`
- `build-artifacts/experimental-font-ab/ProggyVector/ProggyVector_Licence.txt`
- Test-only resource/source/build-script additions in
  `build-artifacts/experimental-font-ab/`.
- One output:
  `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayIntegrationProggyVector.asi`
- This plan, archived under `research/completed/` after the batch.

## Batches and validation

1. Extract the two named archive entries; validate TTF readability, license
   notice, and glyph coverage. Stop if any required glyph is absent.
2. Add one test-only font resource/build variant without touching production
   font loading. Build the experimental ASI and verify output existence and
   embedded resource identity.
3. Review only the exact changed paths and artifact. Do not inspect or mutate
   Git, per the user's standing instruction.

## Risks and rollback/safe-failure

- Font glyph coverage or resource embedding may fail; do not emit an ASI in
  that case, and report the exact failed check.
- All build/source additions remain in the experimental area and can be
  discarded independently; no production artifact is overwritten.
- Preserve the supplied archive unchanged.

## Stop conditions and phase gates

- Stop before build if extraction, license attribution, or glyph coverage is
  unclear or fails.
- Stop after one successful artifact; do not launch the game or expand the
  candidate set.

## Expected final review

- Confirm the exact experimental ASI path, resource/hash validation, and
  untouched production path.
- State that in-game appearance remains unvalidated until the user tests it.
- Git review is intentionally omitted because the user explicitly prohibited
  all Git interaction.
