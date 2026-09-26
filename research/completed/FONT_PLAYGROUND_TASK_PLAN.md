# Experimental Font Playground Task Plan

## Objective

Build one separate experimental ASI with an in-overlay selector for the four
already available font candidates, retaining language and text-size controls,
so the user can compare them in one game session.

## Established evidence and current state

- Existing runtime font-size rebuild is synchronized, recreates the atlas and
  DX12 font texture, and has rollback behavior.
- Local candidates and their licensing materials are already available for
  Proggy Vector, Gohu and Terminus; production currently uses ProggyClean with
  a merged Cyrillic source.
- Dear ImGui 1.91.9b uses stb_truetype in the current build. No FreeType headers
  or library are present in the repository/dependency tree. Dear ImGui's own
  configuration states that FreeType requires separate headers and library.
- The previous isolated rasterization build and stable ASI are not to be
  overwritten or changed.
- Architecture and safety guidance were read. The implementation/testing
  guidance files and `docs/code-style.md` are absent in this checkout.

## Approved scope

- Add a diagnostic Font selector with four candidates:
  1. ProggyClean + Proggy Vector Cyrillic (existing production path).
  2. Proggy Vector only.
  3. Gohu.
  4. Terminus.
- Keep existing Language and Text size controls. Font choice remains in-memory
  only; the existing size setting keeps its current persistence behavior.
- Keep rasterization fixed to stb Pixel Crisp (`PixelSnapH=true`, 1x/1x).
  FreeType is explicitly deferred because it would introduce new dependency and
  build infrastructure.
- Font selection and size changes rebuild the atlas through the current
  synchronized, recoverable renderer path.
- Build a uniquely named separate experimental ASI. Preserve camera,
  localization semantics and stable output.

## Explicit non-goals

- No FreeType integration or new dependency acquisition.
- No stable/release ASI changes, font promotion, text-size range changes,
  layout scaling, localization wording/semantics, camera changes, or settings
  persistence for the diagnostic font selection.
- No game launch, runtime visual validation, or Git operation.
- No overwrite or deletion of existing experimental files/artifacts.

## Files or areas expected to be touched

- `src/overlay/renderer_runtime.cpp/.hpp`, with all playground-specific code
  guarded by a dedicated experimental build flag.
- New isolated loader/resource/build/preflight files under
  `build-artifacts/experimental-font-ab/FontPlayground/`.
- One new uniquely named ASI under `build-artifacts/experimental-font-ab/`.
- `backlog/TASKLOG.md` and this plan, archived under `research/completed/`.

## Batches and validation

1. Add the experimental font-choice state/API and font loaders. Keep one font
   per candidate except the explicitly requested ProggyClean+Cyrillic merge.
   Validate resource/license presence and EN/UK catalog key parity.
2. Add macro-gated UI selection and selection-change detection. Rebuild and
   safely restore the previous font/atlas on failure. Compile stable code path
   only as part of the separate build invocation; do not change its artifact.
3. Build the uniquely named ASI and confirm it exists at the expected path.
   Stop without launching the game or performing visual tuning.
4. Archive this plan and record the bounded build in the task log.

## Risks and rollback/safe-failure behavior

- A missing embedded font/resource fails the candidate atlas build; the prior
  font choice and atlas must be restored via the existing renderer rollback.
- Rebuilding invalidates DX12 device objects, so retain the existing GPU wait
  and renderer resource recreation sequence.
- The build must refuse to overwrite an existing output.
- If a font lacks expected glyphs, record that as a limitation; do not widen the
  scope into font acquisition or family redesign.

## Stop conditions and phase gates

- If a candidate requires new assets, licensing work, or production source
  changes, stop and report rather than expanding scope.
- If FreeType would require new dependency/build infrastructure, omit it.
- Stop after the experimental ASI build; do not launch the game or tune further.

## Expected final Git review

Git state and diffs are intentionally not inspected or changed, per the user's
explicit instruction.
