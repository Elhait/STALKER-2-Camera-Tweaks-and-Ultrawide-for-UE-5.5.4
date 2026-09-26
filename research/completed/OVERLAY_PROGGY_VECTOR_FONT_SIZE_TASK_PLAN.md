# Overlay Proggy Vector and Raster-Size Task Plan

## Objective

Promote the user-approved ProggyClean + Proggy Vector Cyrillic font pairing to
the stable single-file ASI and replace fractional `FontGlobalScale` resizing
with integer-size font-atlas rasterization and safe DX12 font-texture refresh.

## Established evidence and current state

- Production `localization_font.cpp` currently uses ImGui's built-in
  ProggyClean plus Windows Segoe UI for Cyrillic and supplemental punctuation.
- The renderer applies `overlayFontSize / 16` through `ImGuiIO::FontGlobalScale`
  on every frame and immediately when the slider changes.
- The official `ProggyVector-Regular.ttf` and its license were supplied by the
  user, extracted under the experimental artifact area, and validated for all
  66 Ukrainian uppercase/lowercase letters.
- One experimental Proggy Vector ASI built successfully; its embedded font and
  exact license resources were SHA-256 verified. The user accepted the font
  family for stable use and requests a better size mechanism.
- Dear ImGui `AddFontDefault` accepts an explicit `SizePixels`; the DX12 backend
  exposes invalidate/recreate device-object functions. The renderer already
  waits for in-flight GPU work and recreates these objects during swap-chain
  resize.
- The repo has architecture and safety docs, but the referenced
  `docs/assistant/implementation-guidelines.md` and
  `docs/assistant/testing-guidelines.md` are absent.

## Approved scope

- Move/copy the exact supplied Proggy Vector regular font and license into the
  source asset tree and embed both resources in the production ASI.
- Keep built-in ProggyClean for Latin. Use Proggy Vector only for the Cyrillic
  ranges needed by the Ukrainian catalog; keep the existing Segoe UI merge for
  supplemental punctuation/arrows, rasterized at the selected integer size.
- Initialize ProggyClean, Proggy Vector, and supplemental glyphs at the selected
  integer pixel size, with pixel snapping and 1x oversampling.
- Rebuild the atlas only when the requested size changes. Wait for queued GPU
  work, invalidate DX12 device objects, rebuild the CPU atlas and recreate its
  texture before drawing the next frame.
- On rebuild failure, restore the last known-good font size and texture; disable
  the renderer only if recovery also fails.
- Preserve current size bounds/default, config format, slider semantics,
  language/catalog content, input and all camera/runtime behavior.
- Add deterministic atlas and catalog-glyph coverage checks at representative
  sizes, then run `test.cmd` and `build-overlay-settings.cmd`.
- Do not launch the game. Do not run or inspect Git.

## Explicit non-goals

- No new font-family alternatives, localization strings/languages, font-size
  range/default changes, UI redesign, camera behavior or hotkey changes.
- No deletion of the previous experimental artifacts or unrelated Noto files.
- No Git commands, Git state inspection, game launch or injection test.

## Files/areas expected to change

- `assets/fonts/ProggyVector/ProggyVector-Regular.ttf`
- `assets/fonts/ProggyVector/ProggyVector_Licence.txt`
- `src/overlay/localization_resource_ids.h`
- `src/overlay/localization_resources.rc`
- `src/overlay/localization_font.hpp/.cpp`
- `src/overlay/renderer_runtime.hpp/.cpp`
- `tests/overlay/localization_harness.cpp`
- `tests/runner/localization_catalog_audit.ps1`
- `THIRD_PARTY_NOTICES.md`
- Generated test/build outputs under ignored `build-artifacts/` and the stable
  `STALKER2CameraTweaksOverlayIntegration.asi` output.
- This plan, archived in `research/completed/` after the bounded batch.

## Batches and validation

1. Install the supplied font/license in source assets and embed resources;
   verify their resource bytes match the supplied files and audit the license
   attribution.
2. Parameterize font-atlas creation by integer pixel size and add deterministic
   checks for the merged source sizes, render flags, Ukrainian glyphs, and all
   catalog characters at 12, 16, 20, and 24 px.
3. Replace global scaling with a one-time-per-change renderer refresh guarded by
   GPU synchronization and rollback to the previous font on failure. Run the
   focused harness and catalog audit.
4. Run `test.cmd`, build the stable ASI with `build-overlay-settings.cmd`, and
   verify embedded font/license resources plus the exact generated output path.
5. Review touched paths and report completed, remaining, deferred, blocked, and
   not-runtime-validated status. Omit Git review because the user expressly
   prohibited Git interaction.

## Risks and rollback/safe-failure behavior

- Atlas recreation is synchronized to the render thread and occurs before the
  next ImGui frame. The renderer waits for the DX12 queue before freeing the
  old font texture/atlas references.
- If the requested atlas or DX12 objects cannot be recreated, rebuild and
  restore the last known-good size; if that also fails, fail closed by disabling
  the overlay renderer, not by drawing with stale texture data.
- The font remains an embedded resource, so deployment remains one ASI; its
  supplied license text is embedded and also recorded in
  `THIRD_PARTY_NOTICES.md`.
- The prior stable ASI is only replaced by the explicitly requested successful
  stable build. No runtime injection is performed by this task.

## Stop conditions and phase gates

- Stop before production build if required catalog glyph coverage fails or the
  supplied license/resource attribution cannot be preserved.
- Stop if atlas rollback behavior cannot be made safe or compile/test evidence
  contradicts the accepted renderer ownership model.
- After build/resource verification, stop and hand the ASI to the user; wait for
  runtime visual feedback before making further typography changes.

## Expected final Git review

- Git review/state inspection is intentionally not performed at the user's
  standing instruction. Review exact filesystem paths, generated resource
  hashes and build output instead.
