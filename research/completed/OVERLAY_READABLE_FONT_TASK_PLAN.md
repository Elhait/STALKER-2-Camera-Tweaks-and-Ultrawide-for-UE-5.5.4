# Task Plan — Replace the Hard-to-Read Overlay Font

## Objective

Replace the poorly readable bundled Noto Sans Mono face with a familiar,
proportional Windows UI font while preserving English/Ukrainian glyph coverage,
the adjustable text-size control, and the one-ASI user installation contract.

## Established Evidence and Current State

- The user's runtime screenshot confirms both English and Ukrainian are
  functional, but the bundled Noto Sans Mono face is difficult to read even at
  a larger selected size.
- The only font asset in the repository is `NotoSansMono-Regular.ttf`; it is
  currently embedded as a resource and loaded at a 16 px base size.
- The current Windows environment has both `C:\Windows\Fonts\segoeui.ttf` and
  `arial.ttf`. The game/mod is Windows-only; using an OS-installed font adds no
  mod sidecar or distribution file.
- The overlay already has a persistent 12–24 px font-size setting, and the
  renderer owns the ImGui font atlas. English/Ukrainian catalogs remain embedded
  in the ASI.
- The user prohibits all Git operations. No Git commands or state inspection
  will be used.

## Approved Scope

- Load Segoe UI from the standard Windows Fonts directory, with Arial as a
  built-in Windows fallback, using the existing Latin/Cyrillic glyph ranges.
- Remove the font RCDATA reference from the ASI resource script; do not delete
  the existing source font asset or license file.
- Preserve default 16 px size and the persistent 12–24 px user control.
- Extend deterministic font tests/audits for Windows font loading and Latin plus
  Ukrainian glyph coverage; run the full tests and overlay build.
- Produce a fresh single ASI and stop for the user's visual check. Do not launch
  the game.

## Explicit Non-Goals

- Adding a font file to the mod distribution, changing the translations or
  camera/runtime behavior, redesigning the overlay, or changing font-size
  persistence/range.
- Deleting old font assets, adding new languages, or launching/injecting the
  game.
- Any Git operation.

## Expected Files or Areas

- `src/overlay/localization_font.hpp/.cpp`, `renderer_runtime.cpp`,
  `localization_resources.rc`, and `localization_resource_ids.h`.
- `tests/overlay/localization_harness.cpp`,
  `tests/runner/localization_catalog_audit.ps1`.
- `test.cmd`, `build-overlay-settings.cmd`, output ASI, `backlog/TASKLOG.md`.
- This plan, archived under `research/completed/` after validation.

## Batches and Validation

1. Replace resource-based font loading with OS font-path resolution (Segoe UI,
   then Arial), preserving the existing glyph ranges and configured pixel size.
   Verify system paths, initialization success, and glyph atlas construction.
2. Update resource/source audit and full deterministic tests; confirm no font
   sidecar is in the ASI resource script and both locale catalogs remain
   embedded.
3. Build the production overlay ASI, verify its path and timestamp, update the
   task log, and archive this plan. Do not launch the game.

## Risks and Safe Failure

- Windows font files are expected to exist on supported Windows installations.
  Try Segoe UI first and Arial second; if neither loads, fail font initialization
  clearly rather than silently reverting to the unreadable font.
- `AddFontFromFileTTF` loads the OS font during atlas construction; ImGui owns
  the parsed atlas data afterward. No font file is shipped beside the ASI.
- Keep the existing 16 px baked default and scale factor contract. No runtime
  slider semantics change.
- Stop on glyph/test/build failure. Keep the prior ASI untouched until the new
  build succeeds.

## Stop Conditions and Final Review

- Stop if either stock font cannot provide required glyphs, if the resource
  removal affects catalog embedding, or if tests/build fail.
- Review affected filesystem paths against this plan without Git. Report that
  the final appearance still needs user runtime confirmation.

## Git Review

The user expressly prohibits Git interaction. No Git review or Git-state
inspection will be performed.
