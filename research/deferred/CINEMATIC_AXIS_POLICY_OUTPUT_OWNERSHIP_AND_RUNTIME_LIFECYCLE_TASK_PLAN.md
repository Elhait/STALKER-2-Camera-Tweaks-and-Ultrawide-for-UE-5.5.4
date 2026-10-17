# Cinematic Axis-Policy Output Ownership and Runtime Lifecycle — Task Plan

Status: `BATCH 1 PASS / BATCH 1.1 PARTIAL / BATCH 2 COMPLETE — NEGATIVE ACTIVATION EVIDENCE`.

## Objective

Determine whether the output object produced by the validated Steam 2.0.4
camera/view writer carries the axis-policy state into a downstream camera/view
consumer, and observe the confirmed source/output fields across gameplay and
cinematic lifecycle states without writes.

## Established evidence and current state

- Canonical executable identity is Steam 2.0.4:
  SHA-256 `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`.
- Reflected `CameraComponent::SetAspectRatioAxisConstraint` maps to current
  native target `0x14559A03E` and writes `source +0x258`.
- The known writer at `0x140AF4022` tests `source +0x259 & 0x2`, then copies
  `source +0x258` to `output +0x60` and sets `output +0x64 = 1`.
- A bounded raw `+0x60/+0x64` inventory found collisions and no promotable
  effective projection consumer.
- The source pointer is a validated game-owned camera/view state pointer; its
  UE UObject/reflection identity remains unresolved.

## Approved scope

- Static caller/output ownership audit starting at `0x140AF4022`.
- Identify the writer's callers, output-object construction/ownership and
  immediate downstream calls using the same object when provenance is retained.
- Inspect only confirmed camera/view fields in that chain.
- Prepare a diagnostic-only read-only lifecycle tracer for the already
  confirmed source/output fields if the existing diagnostic infrastructure can
  safely observe the validated writer path.
- Tie all static results to the canonical 2.0.4 identity gate.

## Explicit non-goals

- No global `+0x60/+0x64` displacement scan.
- No generic renderer, matrix or tangent search.
- No UObject layout guessing, reflection bridge or pointer scanning.
- No writes to source fields, output fields, camera state or projection state.
- No production ASI, INI, release archive or stable source changes.
- No claim that the output field is an axis-constraint property until object
  ownership and downstream semantics are independently established.

## Files or areas expected to be touched

- This task plan.
- Read-only Ghidra helper(s) under `02-Research/Ghidra/ghidra-scripts`.
- One durable report under `research/reports`.
- Diagnostic source only if a safe read-only tracer boundary is established.

## Batches

### Batch 1 — Writer caller and output ownership

Status: `COMPLETE / PASS`.

- Enumerate direct callers of `0x140AF4022`.
- Decode the call-site registers and identify the output argument corresponding
  to the writer's `RBX` state.
- Follow only immediate downstream calls or returned ownership while the same
  output object can be proven.
- Stop when object identity becomes a generic container/copy without a safe
  bridge.

Validation: current-image identity PASS, bounded reference enumeration and
instruction-level call-site/argument evidence. No decompiler fan-out is needed.

Result: `FUN_1453BA300` calls `0x140AF4022` with source `RDI` and output
`RSI`, then immediately calls `[RAX + 0x638]` with the same `RCX=RDI` and
`R8=RSI` pair. See
`research/reports/CINEMATIC_AXIS_POLICY_OUTPUT_OWNERSHIP_BATCH1.md`.

### Batch 1.1 — Classify source-object vslot `+0x638`

Status: `COMPLETE / BOUNDED PARTIAL`.

The same-object handoff is confirmed, but `[RDI->vtable + 0x638]` remains a
polymorphic dispatch without a safe current-build executable target. Generic
vtable/class scanning is outside scope and is stopped. See
`research/reports/CINEMATIC_AXIS_POLICY_OUTPUT_OWNERSHIP_BATCH1_1_VSLOT638.md`.

### Batch 2 — Read-only lifecycle observation

Status: `COMPLETE / NEGATIVE ACTIVATION EVIDENCE`.

- Use the confirmed writer hook as the safe observation boundary.
- Observe only source `+0x258`, source `+0x259 & 0x2`, output `+0x60` and
  output `+0x64`, plus validated pointers and lifecycle markers.
- Log on value changes where practical to avoid unbounded noise.
- Correlate gameplay, cinematic ENTER, steady cinematic, EXIT and restored
  gameplay.
- Fail closed if the pointer lifetime or lifecycle boundary is ambiguous.

Validation: diagnostic log tied to Steam 2.0.4 and explicitly labeled as
read-only runtime evidence. Runtime writes remain prohibited.

Implementation: `src/cinematic_axis_policy_lifecycle_trace_204.cpp`.
Build script: `build-artifacts/test-scripts/build-cinematic-axis-policy-lifecycle-trace-204.cmd`.
Artifact: `build-artifacts/test-asi/STALKER2CinematicAxisPolicyLifecycleTrace204.asi`.
Preparation report: `research/reports/CINEMATIC_AXIS_POLICY_RUNTIME_BATCH2_PREPARED.md`.
Runtime result: `research/reports/CINEMATIC_AXIS_POLICY_RUNTIME_BATCH2_RESULT.md`.

### Batch 3 — Decision

Status: `DECISION REQUIRED / DOWNSTREAM APPLICATION UNRESOLVED`.

- Promote only a reproducible output-owner and lifecycle correlation.
- If fields remain stable and the gate never activates, classify the result as
  negative lifecycle evidence and stop before projection implementation.
- Continue only with a genuinely new downstream interpretation anchor or a
  separately identified clean camera-cut lifecycle boundary.

## Risks and rollback / safe failure

- The writer may return or populate a transient output view state whose owner
  is not retained by callers.
- Register roles may differ at individual call sites; no guessed argument may
  be promoted.
- A diagnostic tracer must not dereference an unvalidated pointer or read
  outside the confirmed field offsets.
- On ambiguity, record the bounded failure and make no hook or memory write.

## Stop conditions and phase gates

- Stop static work if the current executable identity fails.
- Stop if caller/output provenance dissolves into generic copying or requires a
  broad renderer/matrix search.
- Stop before runtime if no safe read-only boundary exists.
- Stop before any write, hook or production change.

## Expected final Git review

- Read-only path/diff review for only the plan, research helpers and report.
- Confirm production source, ASI, INI, release assets and game packages are
  untouched.
- Do not stage, commit, tag or push.
