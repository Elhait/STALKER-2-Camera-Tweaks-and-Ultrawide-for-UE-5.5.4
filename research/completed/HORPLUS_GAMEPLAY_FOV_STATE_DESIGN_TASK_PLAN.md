# HorPlus Gameplay FOV State Design Task Plan

## Objective

Define a production-safe state/cache and change-only telemetry model for `Gameplay.Mode=HorPlus` before changing runtime behavior. The design must separate gameplay FOV from cinematic FOV and make cache validity depend on the inputs that produced the cached result.

## Established evidence and current state

- `TryTransformHorPlus()` is runtime-validated for tested ultrawide aspects, native ADS inputs, and 16:9 bypass.
- ADS ownership aggregation and completion are runtime validated for the tested Steam 2.0.5 matrix.
- Cinematic ENTER/EXIT and Dialogue exclusion paths already exist.
- Current `ApplyHorPlusGameplay()` reads the current writer input and applies HorPlus on every eligible writer callback; no gameplay HorPlus cache or owner/validity model currently exists.
- `ConfiguredGameplayFov` has not yet been located as a validated standalone runtime source in the current source.
- Cinematic authored/native FOV is a separate domain; observed `90` is not an invariant.

## Approved scope

- Static design only for:
  - `CameraOwner` classification inputs;
  - `GameplayHorPlusState` validity and invalidation;
  - three gameplay FOV values;
  - separate cinematic FOV values;
  - change-only telemetry for handled and unclassified gameplay FOV changes;
  - the first bounded runtime acceptance matrix.
- Identify the smallest source seam needed to obtain configured gameplay FOV, without implementing it in this batch.

## Explicit non-goals

- Do not change `ApplyHorPlusGameplay()`.
- Do not change `AspectRecalculation`.
- Do not change cinematic or Dialogue behavior.
- Do not add sprint, damage, binocular, or other unsupported owner handling.
- Do not claim that unknown gameplay FOV changes are safe until runtime evidence exists.
- Do not build an ASI or launch the game in this design batch.

## Proposed contract

### Domains

Gameplay domain:

```text
ConfiguredGameplayFov
NativeGameplayCameraFov
HorPlusGameplayCameraFov
```

Cinematic domain:

```text
NativeCinematicFov
CinematicHorPlusFov
```

`NativeCinematicFov` must never populate `NativeGameplayCameraFov` or manufacture a valid gameplay cache.

### Gameplay cache

```text
GameplayHorPlusState {
    valid
    configuredFov           // only when a validated source exists
    nativeGameplayFov
    runtimeAspect
    gameplayMode
    horPlusGameplayFov
}
```

`valid=true` only when the cached result corresponds to the current native gameplay FOV, runtime aspect and mode, plus configured FOV when that value is part of the validated input contract.

### Ownership

```text
Gameplay / ADS     → eligible for gameplay HorPlus policy
Cinematic          → cinematic policy only
Dialogue           → Dialogue policy only
Recovery           → no gameplay HorPlus transform
Unknown            → native pass-through and change-only diagnostic
```

The cinematic bypass rule is not unconditional: cinematic samples must not recalculate gameplay state. A previously valid gameplay cache may be preserved; if it is invalid, it remains invalid until a trustworthy gameplay-camera value is observed after gameplay ownership returns.

### Change-only telemetry

For every actual native gameplay FOV change, record:

```text
GAMEPLAY_FOV_CHANGE
configured=...
nativeOld=...
nativeNew=...
modifier=...
horPlusOld=...
horPlusNew=...
aspect=...
owner=...
cacheValid=...
reason=...
handled=...
```

`modifier = nativeNew - configured` is diagnostic evidence only; it is not a production classification rule.

## Batches and validation

### Batch 1 — static design

- Trace current HorPlus, coordinator, ADS, Dialogue and cinematic state ownership.
- Locate or explicitly mark the missing configured-FOV source.
- Record invariants and invalidation triggers: aspect, mode, configured FOV, load/ownership return.
- Produce the design report.

Validation: source cross-reference and design review; no build or runtime.

### Batch 2 — telemetry-only implementation design

- After approval of Batch 1, add change-only telemetry behind a diagnostic macro.
- Do not alter HorPlus output or owner behavior.

Validation: harness/build only; no stable ASI replacement.

### Batch 3 — one runtime acceptance pass

- Use a non-90 configured FOV if available.
- Exercise gameplay idle, ADS, sprint/run, weapon changes, damage/DoT, death/respawn, Dialogue, Cinematic, and binocular 2.0.
- Classify only observed unknown FOV changes; do not infer unsupported behavior in advance.

Validation: supplied runtime log and visual result; separate from build evidence.

## Risks and safe failure

- Treating cinematic FOV as gameplay FOV can corrupt the gameplay cache; domain separation is mandatory.
- A missing configured-FOV source must leave that field unknown rather than inventing a read.
- Unknown ownership must pass through natively and be logged, not transformed by assumption.
- Existing `AspectRecalculation` remains the fallback if HorPlus behavior is unsuitable.

## Stop conditions

- Stop before implementation if configured FOV cannot be sourced without a new unvalidated hook.
- Stop if any proposed design requires continuous heavy FOV processing during stable idle gameplay.
- Stop and reopen ownership design if runtime shows a foreign owner being transformed by HorPlus.

## Expected final review

- Confirm this batch changed only research/design documentation.
- Distinguish established source facts, hypotheses and missing seams.
- Keep implementation and runtime validation explicitly pending.

## Approved follow-up — shared lifecycle FOV publication (2026-10-07)

This approved implementation follow-up supersedes the earlier static-design-only
scope above for the bounded publication work below. It does not reopen or alter
Gameplay/Cinematic/Dialogue camera-correction policy.

### Objective and semantic contract

Publish four distinct camera FOV roles in the shared Camera State:

- `Gameplay FOV`: confirmed Gameplay baseline/reference; Current while freshly
  established by eligible Gameplay observations, Retained across owner changes,
  and Unavailable only when no valid baseline exists or an explicit invalidator
  clears it.
- `Native FOV`: actual input to the current production camera transaction.
- `HorPlus Target`: target produced/carried by the production policy or lifecycle
  state; never recomputed by the Overlay.
- `Current Output FOV`: actual output returned by the current production camera
  transaction, which can differ from both input and target.

Transition boundary operands (Gameplay boundary and Cinematic ENTER) remain
separate Transition Evidence and are not aliases for these four roles.
Dialogue publication uses the existing XMM6 input and transaction output
observation without acquiring camera ownership or changing correction semantics.

### Scope and non-goals

- Extend shared snapshot publication at the existing writer transaction seam
  for Gameplay, Cinematic, continuation and recovery.
- Publish Dialogue input/output from its existing transaction seam, tagged with
  lifecycle/provenance and without changing ownership.
- Expose the four roles and their validity/freshness sufficiently in the
  existing Camera State/Integration presentation for one runtime validation.
- The main Camera Integration summary must project all four shared roles
  directly, including Retained/Last known freshness; expanded Camera State is
  supplemental diagnostics, not a prerequisite for seeing the shared model.
- Add deterministic publication/transaction regression coverage for Current,
  Retained and Unavailable behavior and ensure observation does not alter output.
- Do not change aspect/FOV correction, flags, RuntimeCombine, recovery,
  coordinator ownership, transition assessment, or perform broad UI redesign.
- Do not launch the game, install/deploy the ASI, or claim runtime validation.

### Expected files and validation

Expected source areas: `src/camera/camera_state_snapshot.hpp`,
`src/camera/camera_state_publication.hpp`, `src/plugin/runtime.cpp`,
`src/plugin/camera_writer_transaction.hpp`, the existing Camera State view and
integration presentation/localization, and their focused harnesses. Preserve
all existing staged and unstaged changes; do not stage or commit.

Acceptance: each role carries an independently valid value, provenance and
freshness; a valid Gameplay baseline remains Retained during Cinematic,
Dialogue, continuation and recovery; current Native/Target/Output values track
the actual transaction; Dialogue values are observed without changing its
owner; ENTER operands remain separate; unavailable facts are not synthesized.
Run focused harnesses, localization/catalog validation as applicable, `test.cmd`
and the ordinary production build if the focused checks pass. The runtime
sequence remains user-run and pending after producing a separate candidate
artifact.

### Status

- Approved by the user in chat on 2026-10-07; the four-role shared Camera
  State publication and diagnostic view are implemented in the working tree.
- Focused camera-writer transaction coverage and the locale catalog audit pass.
- Full `test.cmd` reached the localization font-rendering harness after the
  camera/runtime/overlay harnesses passed, then failed on missing existing
  Japanese, Chinese and Korean glyphs. This remains an independent catalog/font
  coverage issue; it is not claimed as a full-suite pass.
- Production-profile candidate build succeeded to the isolated ignored path
  `build-artifacts/STALKER2CameraTweaksFovStateCandidate.asi`; the canonical
  production ASI and game installation were not overwritten or changed.
- Runtime and visual validation are explicitly pending user execution.

### Follow-up — main summary projection regression (2026-10-07)

- Runtime screenshots exposed that the expanded Camera State had the four
  roles while the main Integration summary still projected legacy aggregate
  fields and hid values unless coordinator/freshness aggregates were Current.
- The follow-up changes only shared-fact projection and summary presentation;
  camera correction, transaction, and ownership semantics remain out of scope.
- Regression coverage spans Gameplay, CinematicActive, Dialogue within
  CinematicActive, CinematicExiting recovery, and Gameplay after handoff,
  asserting value, validity and Current/Retained freshness per role.
- Owner metadata was investigated: FOV publication receives a coordinator-
  derived transaction owner, while the `owner=` diagnostic currently prints
  `snapshot.presentation.state`, the last full snapshot's presentation state.
  That is snapshot metadata rather than transaction-owner truth; do not change
  ownership behavior as part of this presentation fix.
- Projection and writer-transaction harnesses pass for all four roles through
  the requested lifecycle phases. The locale/catalog audit passes after its
  dashboard contract was updated for the four-row summary. A production-profile
  candidate was built at
  `build-artifacts/STALKER2CameraTweaksFovSummaryCandidate.asi` (SHA-256
  `E8BB21E414DB849D5D549FC1E761741B6FC1A0A2BC9BAFE77EA81851C9CAFD83`).
- `test.cmd` remains not fully green because the localization font-rendering
  harness reports existing missing Japanese, Chinese and Korean glyphs; all
  camera/integration harnesses, including the new projection cases, passed.
- Runtime/visual validation remains pending. The candidate was not installed
  and the game was not launched.
