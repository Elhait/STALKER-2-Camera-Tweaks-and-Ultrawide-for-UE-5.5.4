# Overlay Font Rebuild Parity Task Plan

## Objective

Determine whether a cold-start atlas at size X and a runtime-style atlas rebuild
to size X produce equivalent font state and pixels. Fix only a reproducible
startup-versus-rebuild discrepancy and rebuild the stable ASI if a fix is
needed.

## Established evidence and current state

- The user's runtime report says text is clearest at the size used to initialize
  the overlay and worsens after a runtime size change.
- Startup and runtime both call `AddProggyCleanWithCyrillic(fontSizePixels)`.
  This function applies the same ProggyClean default-font settings, embedded
  Proggy Vector Cyrillic merge, supplemental glyph merge, glyph ranges,
  pixel snapping and oversampling in either path.
- Startup explicitly builds the atlas before DX12 backend initialization.
  Runtime waits for GPU completion, invalidates DX12 device objects, clears and
  rebuilds the atlas, then recreates DX12 device objects before the next frame.
- No parameter/configuration mismatch has been established by source review.
- The relevant implementation/testing guidance files under `docs/assistant/`
  and `docs/code-style.md` are absent; architecture and safety-invariant docs
  were read.

## Approved scope

- Add deterministic parity coverage that hashes or byte-compares the CPU atlas
  and checks font/source metrics for cold start at X versus build-at-Y, clear,
  and rebuild-at-X.
- Inspect the DX12 invalidation/recreation sequence and identify any concrete
  divergence from first device-object creation.
- If deterministic evidence isolates a defect, correct only that defect,
  rerun the relevant harnesses and `test.cmd`, build the stable ASI, and verify
  its output. If no discrepancy is found, stop without production changes or a
  new ASI build.
- No game launch. No Git commands or Git-state inspection.

## Explicit non-goals

- No layout/column scaling, UI spacing changes, localization edits, font-family
  change, font-size range/default change, or camera/runtime behavior change.
- Do not infer that individual pixel sizes are inherently poor without the
  requested cold-start-versus-runtime comparison.
- Do not replace the current production ASI unless an evidenced fix is made and
  the approved validation passes.

## Files or areas expected to be touched

- `tests/overlay/localization_harness.cpp` and possibly a focused overlay test
  helper.
- Only if a reproducible mismatch is found: the minimal relevant file(s) in
  `src/overlay/localization_font.cpp`, `src/overlay/renderer_runtime.cpp`, or
  the DX12 backend call path.
- `backlog/TASKLOG.md` and this plan if a bounded result is established; plan
  will be archived under `research/completed/` when finished.
- Stable ASI output only if an evidenced code fix is made and builds pass.

## Batches and validation

1. Compare atlas pixels, texture dimensions, font metrics, source configs and
   glyph coverage for cold build X and simulated runtime-style rebuild to X
   (using an intermediate size). Include multiple representative target sizes.
2. If CPU atlas parity passes, inspect the renderer/backend lifecycle for state
   that differs only during device-object recreation. Do not claim full GPU or
   runtime parity based on the CPU harness alone.
3. If a concrete mismatch is isolated, fix only it and rerun the focused
   harness, `test.cmd`, and `build-overlay-settings.cmd`; verify the resulting
   ASI. If not isolated, stop and report the remaining runtime-only question.
4. Perform a read-only filesystem/path review against this plan. Omit Git review
   per the user's explicit prohibition.

## Risks and rollback/safe-failure behavior

- Atlas data comparisons must exclude nondeterministic pointers/addresses and
  compare rendered texture bytes plus stable metrics/configuration instead.
- Avoid modifying a locked atlas or rebuilding inside an active ImGui frame.
- Any production repair must preserve the existing GPU wait, invalidation,
  last-known-good restore and fail-closed behavior.
- Do not replace the existing ASI when parity remains unproven or validation
  fails.

## Stop conditions and phase gates

- Stop production changes if cold and runtime-style CPU atlases match and no
  concrete backend discrepancy can be established from code evidence.
- Stop before build if a proposed change expands into layout scaling, broader
  font tuning, or camera/runtime semantics.
- After any evidenced repair and successful build verification, stop and hand
  the ASI to the user for the controlled in-game A/B: cold start at X versus
  runtime change to X.

## Expected final Git review

Git review/state inspection is intentionally omitted at the user's standing
instruction. Review the exact changed paths, parity-test evidence and build
artifact directly instead.
