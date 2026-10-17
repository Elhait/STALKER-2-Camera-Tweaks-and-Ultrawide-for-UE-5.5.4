# Cinematic Aspect-Ratio Axis Constraint Ownership — Task Plan

Status: Axis-constraint direction OPEN / UNRESOLVED — Batch 1A complete,
Batch 1B complete with blocked outcome, Batch 2 blocked.

## Objective

Determine whether the STALKER 2 cinematic camera-cut path uses an effective
UE5 aspect-ratio axis constraint and whether `MaintainYFOV` can be applied at
that policy boundary without manually writing a Hor+ transformed FOV into
durable cinematic camera state.

The intended behavior is:

```text
authored cinematic FOV ≈ 90
effective axis policy = MaintainYFOV
viewport = 32:9
→ projection presents Hor+
→ durable cinematic FOV remains near authored value
```

## Established evidence and current state

- UE 5.5 exposes `EAspectRatioAxisConstraint` with
  `AspectRatio_MaintainYFOV`, `AspectRatio_MaintainXFOV` and
  `AspectRatio_MajorAxisFOV`.
- UE 5.5 documents the policy at multiple layers:
  `FMinimalViewInfo::AspectRatioAxisConstraint`,
  `UCameraComponent::AspectRatioAxisConstraint`,
  `ULocalPlayer::AspectRatioAxisConstraint` and
  `FCameraCutPlaybackCapability::GetAspectRatioAxisConstraintOverride()`.
- Existing STALKER 2 research confirmed cinematic aspect/FOV lifecycle
  boundaries but did not establish the effective axis-constraint owner.
- Current production behavior uses the existing full-Hor+ path and remains the
  control baseline.
- The authored 16:9 @ approximately 90-degree image remains the composition
  reference; no claim is made that `MaintainYFOV` is currently consumed by
  STALKER 2.

## New engine-side anchor

Official UE 5.5 API documentation supplies a source-level correspondence
target, without proving that the stripped STALKER 2 binary retains the same
implementation or call path:

- `ULocalPlayer::AspectRatioAxisConstraint` is the documented local-player
  fallback policy.
- `FMinimalViewInfo` carries an optional `AspectRatioAxisConstraint` override.
- `FCameraCutPlaybackCapability::GetAspectRatioAxisConstraintOverride()` is a
  documented camera-cut-specific override input.
- UE projection construction consumes the effective constraint while forming
  the view/projection data from the view description and viewport.

This is a new engine-side evidence class. It permits one future, narrow
source-to-binary correspondence pass; it does not reopen a broad renderer or
generic `FMinimalViewInfo` scan.

## Approved scope

- Current Steam 2.0.4 static ownership inventory, after the existing identity
  gate passes.
- Inspect the following ownership order:

  ```text
  FMinimalViewInfo
  → CameraCutPlaybackCapability override
  → cinematic evaluation / camera-cut application
  → UCameraComponent override
  → ULocalPlayer fallback
  ```

- Identify effective constraint reads, writes, overrides and fallback order.
- For every constraint candidate, record when it is read or applied relative
  to the FOV lifecycle: before effective-FOV construction, after authored
  cinematic FOV, before view/projection construction, or after another path
  has already converted the FOV.
- If a candidate owner is found, prove whether the effective value is inherited,
  overridden or synthesized specifically for camera cuts.
- Correlate only concrete cinematic camera-cut paths with the known ENTER,
  steady and EXIT lifecycle.
- Classify each result as `CONFIRMED OWNER`, `CONFIRMED DOWNSTREAM BOUNDARY`,
  `BOUNDED LEAD`, `REJECTED` or `DEFERRED / REQUIRES LOWER-LEVEL IMPLEMENTATION`.

### Batch 1B — GetProjectionData structural correspondence

This is a separately tracked bounded batch enabled by the new UE 5.5
engine-source correspondence. Its primary target is the current-build
equivalent of `ULocalPlayer::GetProjectionData` and its effective-constraint
handoff into projection construction.

Look specifically for:

- viewport or sub-rect dimensions;
- aspect derivation;
- perspective projection construction;
- effective FOV input;
- a small enum/branch with X-vs-Y FOV treatment;
- a LocalPlayer-like fallback read;
- an optional override arriving with view or camera-cut data.

Do not require a complete `LocalPlayer → FMinimalViewInfo →
CameraCutPlaybackCapability → projection` mapping for a positive result. A
current-build projection boundary with axis-dependent semantics and safe
observation context is sufficient for `PARTIAL PASS`.

## Explicit non-goals

- No production source, ASI, INI, build or release-asset changes.
- No guessed enum writes, object offsets or camera-component assumptions.
- No renderer hook or projection-matrix implementation.
- No Panini changes and no further blind renderer-CVar search.
- No runtime test until a safe effective owner or test boundary is established.
- No claim that a UE API symbol exists by name in the stripped shipping binary
  without current-build evidence.

## Files or areas expected to be touched

- This task plan.
- One durable report under `research/reports`.
- Existing Ghidra/static evidence only; reusable scripts only if a bounded
  query is necessary.
- No stable source, build output, runtime artifact or release asset.

## Batches

### Batch 1A — Generic ownership inventory

Status: `COMPLETE / UNRESOLVED`.

- Pass the current Steam 2.0.4 identity gate.
- Start with view-description and camera-cut evaluation semantics, then trace
  only enough surrounding data flow to establish effective ownership.
- Record enum-like values, override flags, caller/object context and whether
  the path is cinematic-specific or generic.
- Record the candidate's position relative to authored FOV, effective FOV and
  view/projection construction, plus the inherited/overridden/synthesized
  status of its camera-cut value.
- Stop if the evidence becomes a broad renderer or generic field scan.

Validation: identity header, decoded instructions/data flow and current-build
caller/object evidence. Static evidence is not runtime proof.

Result: `research/reports/CINEMATIC_ASPECT_RATIO_AXIS_CONSTRAINT_OWNERSHIP_BATCH1.md`
records `Identity: PASS` and a bounded unresolved result. No current-build
mapping was established for `FMinimalViewInfo`, camera-cut override,
`UCameraComponent` override or `ULocalPlayer` fallback, including the timing
of any constraint read relative to authored/effective FOV and projection
construction. Existing cinematic FOV/aspect boundaries are not promoted as
axis-constraint owners. Batch 1 passes as a completed inventory; Batch 2 is
blocked pending a new concrete runtime or engine-side anchor.

### Batch 1B — GetProjectionData structural correspondence

Status: `COMPLETE — NO PROMOTABLE CURRENT-BUILD BOUNDARY`.

- Target only the current-build structural equivalent of
  `ULocalPlayer::GetProjectionData` and the effective axis-policy handoff.
- Require current-build caller/object context and the position of the
  constraint read relative to authored FOV, effective FOV and projection
  construction.
- Promote a candidate only when its axis-dependent branch semantics are
  established. A generic `tan()`/aspect/matrix helper is insufficient.

Validation: current-build static evidence and decoded/data-flow context; no
runtime or production claim.

Possible outcomes:

- `PASS`: effective-constraint application identified with current-build
  caller/object context and X/Y FOV semantics.
- `PARTIAL PASS`: projection boundary identified and safely observable, but
  upstream effective owner remains unresolved.
- `BLOCKED`: only generic projection math is found, no axis-policy
  discriminator is established, or the search escapes into broad renderer
  territory.

Result: `COMPLETE — BLOCKED OUTCOME` — the UE 5.5 source contract was
confirmed, but no
current-build structural signature, caller/object context and axis-dependent
projection semantics were established. Existing historical 2.0.3 projection
logs are not current-build evidence. See
`research/reports/CINEMATIC_ASPECT_RATIO_AXIS_CONSTRAINT_OWNERSHIP_BATCH1B.md`.

The hypothesis itself is not rejected. Do not repeat this work under a broader
or renamed `FMinimalViewInfo`/renderer scan. Further work requires a genuinely
new evidence class, such as independent engine-side symbol or metadata
correspondence, known UE function byte correspondence, a concrete
renderer/projection anchor, runtime observation, frame-capture evidence or
other independently verifiable evidence.

### Batch 2 — Reversible runtime policy test

Status: `BLOCKED` pending Batch 1B.

- Test `MaintainYFOV` only at the confirmed effective boundary.
- Compare 16:9 reference, current 32:9 Full Hor+ control and test-only result.
- Verify authored/durable FOV, center composition, vertical coverage, side
  coverage and cinematic EXIT handoff.
- Keep the test reversible and isolated from the production ASI.

### Batch 3 — Architecture decision

Status: `BLOCKED` pending Batch 2.

- Promote only if the cinematic path consumes the policy and the test changes
  projection as expected while preserving durable FOV and EXIT behavior.
- If the constraint exists but cinematic bypasses it, classify the branch as
  blocked/deferred rather than forcing a write.
- Any production implementation requires a separate approved task.

## Acceptance criteria

### PASS

- Effective axis-constraint owner identified.
- Cinematic path proven to consume it.
- Test-only `MaintainYFOV` changes the 32:9 projection as expected.
- Authored/durable cinematic FOV remains near its native value.
- No unacceptable crop, composition regression or EXIT seam is introduced.

### BLOCKED

- Constraint exists but cinematic path bypasses it.
- Ownership cannot be resolved.
- A lower-level override replaces it.
- Test changes no projection behavior.
- Safe reversible access to the boundary cannot be established.

## Risks and safe failure

- Stripped binaries may not retain UE API names or may inline the policy.
- A field with enum-like values may be unrelated to aspect-ratio constraint;
  names/values alone are insufficient.
- A generic local-player policy may be overridden by cinematic evaluation or a
  camera-cut-specific value.
- On ambiguity or mismatch, stop and preserve the current Full Hor+ baseline.

## Stop conditions and phase gates

- Identity must be `PASS` before static interpretation.
- Stop on a broad renderer scan or guessed object/field mapping.
- Batch 2 remains blocked until a new evidence class establishes a safe effective owner or
  concrete reversible observable boundary.
- Stop before production changes, public configuration changes or release
  preparation.

## Expected final Git review

- Report exact research files created or modified.
- Separate confirmed ownership, bounded leads, rejected candidates, deferred
  branches and not-runtime-validated claims.
- Confirm production source, ASI, INI, build output and release assets were not
  changed.
- Do not stage, commit, tag or push as part of this task.
