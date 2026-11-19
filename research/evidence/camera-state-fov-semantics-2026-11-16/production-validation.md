# Camera State FOV semantics — production implementation

Status: IMPLEMENTED — OFFLINE VALIDATED; in-game display confirmation pending.
This report concerns the publication/UI change after the four captured red regressions.
It does not reopen U2 correction algorithms, baseline admission or visual continuity.

## Implemented contract

- Gameplay FOV is the production GameplayBaselineStore reference. No zoom event is
  required. Its original admission sequence/source remain intact. Confirmed Dialogue
  retains the reference; receiver mismatch and production invalidation cannot restore it.
- The zoom-qualified neutral observation remains separate, with its existing stability,
  source and zoom guards. Zoom transitions retire that observation independently.
- Original Native FOV captures identify Gameplay writer, Dialogue setter or authored
  Cinematic ENTER. Prepared writer input cannot overwrite an original capture as Current.
  An ENTER reference is retained/history, not a reconstructed native blend trajectory.
- Writer Input and Planned Output are separate. The legacy currentOutputFov C++ member
  is explicitly a plan; no caller promotes it to actual output.
- Full and partial publication interfaces carry the existing writer transaction identity.
  Readbacks match transaction ID, receiver, output object, thread and instrumentation
  generation. Instrumentation generation is not native receiver lifetime. Snapshot event
  sequence and numerical equality do not establish correlation.
- Primary post-store and post-helper observations remain separate facts. Effective selects
  an available correlated named observation, preserving both boundaries in camera details.
  A readback different from Planned remains the observed value. Missing/unreadable,
  foreign, stale or uncorrelated input cannot verify the current plan.
- Diagnostics OFF preserves Planned and correction behavior; Effective is Unavailable.
  Existing observers supply readback; no new hooks or always-on memory reads were added.
- Runtime adapters still reject unreadable camera sources, and publish before changing
  the writer register. The Camera State lock remains a projection boundary.

## Validation

- Four original red publication regressions: PASS.
- Added receiver replacement, no resurrection after confirmed Dialogue, production
  invalidation, N/N-1 identity, receiver/output/thread/generation mismatches, unreadable
  and unnamed readback, lifecycle retirement and Diagnostics OFF checks: PASS.
- Actual private runtime adapters exercise writer -> native-store fixture -> primary
  observer -> post-helper observer -> Overlay projection under Diagnostics OFF/ON.
  The post-helper fixture deliberately changes the stored value: Planned stays intact,
  primary readback stays intact and Effective uses the helper observation: PASS.
- Existing neutral-observation fixtures were moved to the separate neutral fact; their
  qualification was not weakened. Other old assertions were updated only where the
  accepted field meaning changed (original ENTER reference versus live prepared input).
- Full canonical test.cmd: 62/62 PASS, 0 FAIL; 1 explicit hardlink subcase SKIP (error=5).
  NonGraphics=0, WARP_case_omitted=0. Runtime adapter reports 230 fresh processes, OFF/ON.
- All 18 catalogs: key/placeholder/UTF-8 and embedded glyph coverage PASS. Font subsets
  regenerated using the existing pinned tool and SHA-verified upstream fonts. The font
  addition is required by the new text; it does not alter renderer resource ownership.
- Standard production Release build: PASS. Root ASI matches the candidate SHA-256.
- Complete task diff reviewed; git diff --check PASS. No correction formulas, mode
  selection, provider admission, GameplayHorPlus, Reduced or recovery policy changes.
- Every validation/build/font tool used bounded process-tree supervision. Cleanup was
  verified, followed by a read-only host check: no residual compiler/harness/helper.

## Scope and remaining evidence

No files in the game directory were changed. No runtime display or final rendered FOV
claim is made. Arabic shaping/RTL and in-game font layout are not established by glyph
coverage. No new frame-time performance claim is made.

One targeted display check remains: keep the current GameplayHorPlus / Dialogue Reduced
configuration and complete Gameplay -> Cinematic 1 -> Dialogue -> Cinematic 2. Gameplay
reference should be Current in admitted Gameplay and Retained through confirmed owners.
Prepared/corrected values belong in Writer Input, never Current original Native. Captured
original references show their boundary and historical status. With Diagnostics ON,
Effective should come from the same writer transaction at post-store/helper; OFF should
leave Effective Unavailable. Missing originals are allowed; no guessed value is expected.
No repeat Gameplay A/B or separate control Dialogue is required for this display check.

Artifact identity and complete validation output are adjacent JSON files.
