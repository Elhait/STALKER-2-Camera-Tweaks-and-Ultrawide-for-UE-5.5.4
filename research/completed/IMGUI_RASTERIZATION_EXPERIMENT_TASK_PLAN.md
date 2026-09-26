# ImGui Rasterization Experiment Task Plan

## Objective

Build one isolated experimental ASI with Proggy Vector as the only font and an
in-overlay selector for four ImGui rasterization modes, while retaining the
existing Text size control.

## Established evidence and current state

- The previous Proggy Vector-only ASI showed that using one face removes the
  Latin/Cyrillic style mismatch but does not match the user's ProggyClean
  13 px reference.
- This checkout vendors Dear ImGui 1.91.9b. Its font builder uses
  `stb_truetype`; its DX12 backend uploads the generated RGBA atlas and does not
  add a Windows ClearType stage.
- In this ImGui version, nonzero `OversampleH/V` feed stb_truetype's packer. The
  requested controls therefore represent different atlas rasterization
  configurations. `PixelSnapH=true` is incompatible with useful horizontal
  oversampling (automatic H oversampling is 1 when snapping is enabled).
- The exact rasterization experiment output path was confirmed absent before
  work. Stable output remains untouched.
- Project architecture/safety guidance was read; implementation/testing
  guidance and `docs/code-style.md` are absent in this checkout.

## Approved scope

- Add four experimental rasterization modes:
  - Pixel Crisp: snap=true, H=1, V=1.
  - Smooth 2x: snap=false, H=2, V=2.
  - Smooth H2: snap=false, H=2, V=1.
  - Smooth 3x: snap=false, H=3, V=3.
- Add an experimental-only, non-persisted runtime selector that rebuilds the
  font atlas using the already-existing synchronized/recoverable renderer path.
- Retain the existing runtime Text size setting, font size persistence,
  language selector, and all other behavior.
- Use Proggy Vector as the only font source; no ProggyClean or font merging.
- Gate renderer changes behind `OVERLAY_FONT_RASTERIZATION_EXPERIMENT` and
  compile only a uniquely named experimental ASI.
- Do not edit stable ASI output, production localization/font code, layout,
  camera behavior, or input policy. Do not launch the game. Do not run Git.

## Explicit non-goals

- No changes to the stable UI or shipped settings.
- No new font family, FreeType/DirectWrite backend, localization strings/catalogs,
  text-size range/default, or UI layout scaling.
- No runtime game test; stop after the experimental ASI build.
- No overwriting or deleting any existing experimental artifact.

## Files or areas expected to be touched

- `src/overlay/renderer_runtime.cpp/.hpp`, with every experiment-specific
  addition gated behind the experiment macro.
- `build-artifacts/experimental-font-ab/ProggyVectorRasterization/`:
  font loader, mode API header, resource script and build script.
- `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayRasterization.asi`
- `backlog/TASKLOG.md` and this plan, archived under `research/completed/`.

## Batches and validation

1. Add the mode API and ensure the Proggy Vector loader uses exactly one font
   source with the selected snap/oversampling configuration.
2. Add the experimental selector and mode-change detection behind the build
   macro. Reuse the existing pre-frame GPU wait, atlas rebuild, DX12 resource
   recreation, and rollback logic. Mode selection is in-memory only.
3. Run the repository catalog audit through the experimental build script and
   compile/link the uniquely named ASI. Review compiler result and configured
   artifact path. Do not run the game or any post-build runtime validation.
4. Archive the plan and record the bounded build in the task log; stop.

## Risks and rollback/safe-failure behavior

- Oversampling increases atlas work/memory but only for this temporary test
  artifact. It does not affect shipped code unless the experiment macro is set.
- On a mode-switch rebuild failure, restore the previously active mode and
  atlas through the existing safe rollback path; disable renderer only if
  restoration fails.
- The separate output path must be absent before building; if it exists, stop
  rather than overwrite it.

## Stop conditions and phase gates

- Stop if achieving the selector requires changing stable runtime behavior or
  bypassing renderer rollback/GPU synchronization.
- Stop after successful build, with no game launch or further font tuning.

## Expected final Git review

Git review/state inspection is intentionally omitted at the user's explicit
instruction. Review only the approved gated source and experimental output.
