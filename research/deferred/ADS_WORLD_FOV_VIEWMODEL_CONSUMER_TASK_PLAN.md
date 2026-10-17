# ADS World-FOV to Viewmodel Consumer — Task Plan

Status: Batch 4 tracer built; awaiting one bounded in-game runtime capture

## Objective

Identify which current Steam `2.0.4` consumer observes the ADS-driven world-FOV
transition and owns or rebuilds downstream first-person/viewmodel presentation
state that visually normalizes after ADS.

## Established evidence and current state

- Canonical executable SHA-256:
  `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`.
- Canonical Ghidra project: `Dump/STALKER2-Ghidra`.
- `.text` size: `130818560`; image base: `0x140000000`.
- ADS IN/OUT resolve at `RVA 0x6ABE7E / 0x6AC01B` and share
  `FUN_1406ABB9C`.
- Runtime differential shows world FOV changing during visual correction while
  inspected `+0x234`, `+0x262`, camera identity, mesh assignment and primitive
  setter remain unchanged.
- The primitive setter/MRSD branch is closed as non-causal for the captured
  correction. The downstream presentation owner remains open.

## Approved scope

1. Pass the matching-image identity gate before interpretation.
2. Start from current `+0x230` world-FOV reads and the validated ADS/camera
   neighborhood, not from an executable-wide generic renderer search.
3. Rank only candidates with a defensible combination of world-FOV data-flow,
   first-person state/selection, camera/view construction or derived
   presentation writes.
4. Inspect local basic blocks and direct callees for the strongest candidates.
5. Produce one report with candidate evidence, rejection reasons and a runtime
   gate only if a concrete owner is found.

## Explicit non-goals

- Do not create a runtime tracer in the absence of a concrete target.
- Do not scan or rank the generic renderer/projection universe broadly.
- Do not treat raw `+0x230` offset intersections as ownership proof.
- Do not modify production C++/ASI/INI logic or release files.
- Do not reopen the closed primitive/MRSD branch without contradictory evidence.
- Do not add hard-coded weapon FOV values or formulas.

## Files or areas expected to be touched

- This task plan.
- One read-only Ghidra helper under `02-Research/Ghidra/ghidra-scripts` if a
  bounded query is required.
- One report under `02-Research/Ghidra/reports`.
- One `backlog/TASKLOG.md` entry only after the bounded theory is accepted,
  rejected or a candidate is explicitly deferred.

## Batches and validation

### Batch 1 — Matching-image identity gate

Verify executable SHA-256, `.text` size and SHA-256, image base/section layout,
ADS anchors and the current camera-writer anchor. Record explicit
`Identity: PASS`; stop interpretation on mismatch.

Result: PASS on the canonical 2.0.4 program. The retained 2.0.2 and 2.0.3
entries were stopped by the identity gate and were not used for interpretation.

### Batch 2 — Bounded world-FOV consumer discovery

Inspect current `+0x230` reads in the validated ADS/camera-related slice and
their nearby `+0x234`, `+0x262`, projection math, first-person branches and
derived-state writes. Exclude constructors, serialization, reflection,
editor/debug and high-fan-out generic helpers.

Result: PASS. `FUN_140AF4022` contains the bounded camera-state-to-output-state
handoff. It copies `camera +0x230` to output `+0x30`, selects `camera +0x234`
or the copied world FOV using `+0x262`, and calls `FUN_140AF4FA4`.

### Batch 3 — Candidate ranking and stop decision

Rank surviving candidates by direct world-FOV data-flow, first-person or
viewmodel ownership, causal proximity to ADS/camera update and specificity of
the derived presentation write. If no candidate meets the gate, close this
static branch as unresolved without creating a tracer.

Result: a concrete P1 runtime candidate was found. `FUN_140AF4FA4` performs
`tanf`/`atanf` projection math and updates derived output fields at `+0x30`,
`+0x40`, `+0x9D0`, `+0x9D4` and `+0x9D8`. Runtime causality remains unproven.

### Batch 4 — Bounded runtime gate and final review

Approved next step: validate only the callsite at
`RVA 0xAF42A7` or `FUN_140AF4FA4`, correlated with existing ADS markers and
the visual correction. Then review identity provenance, changed paths and
stop-condition compliance. Add one factual task-log entry only after the
candidate is accepted, rejected or explicitly deferred.

Runtime gate scope is read-only. It records the source/output pair, source
`+0x230/+0x234/+0x262`, output `+0x30/+0x38/+0x9D0/+0x9D4/+0x9D8`, consumer
PRE/POST state and existing ADS phase markers. No camera, primitive,
render-state or production writes are permitted.

Tracer built successfully as:
`build-artifacts/test-asi/STALKER2AdsWorldFovConsumerRuntimeGate204.asi`.

The first capture produced valid PRE/ADS evidence, but no POST events because the
initial post hook was attached to the early `RET` at `RVA 0xAF4FDD`. A second
capture passed identity and installed the hook at `RVA 0xAF50B7`, but produced
neither POST nor mismatch diagnostics. The canonical byte audit then confirmed
the last guaranteed derived-state write at `RVA 0xAF50AF`
(`MOVSS [RSI+0x9D4],XMM6`). The tracer was moved there and rebuilt; the
next gate is one repeat user-run capture of `cinematic EXIT → wrong weapon
framing → ADS → correct framing`.

The subsequent canonical CFG audit established that `0xAF50AF` is the final
write only on the non-early projection path. `FUN_140AF4FA4` can branch from
entry to cleanup/RET at `0xAF4FCC`, so `0xAF50AF` is not a universal POST point.
The two captures with no POST callback do not resolve whether that early path
was selected or the mid-hook failed to observe the instruction. No further ASI
hook relocation is authorized until the entry predicate and calling convention
are reconciled statically.

The approved follow-up is PRE-only runtime observation of `source+0x25C`,
`abs(+0x25C)`, the derived `projection`/`early` predicate and source flags
`+0x260/+0x261`. The POST hook was removed from this gate because CFG has already
established that `0xAF50AF` is conditional. The read-only tracer rebuilt
successfully; production behavior remains untouched.

The capture recorded 1914 PRE events. `source+0x25C` remained `0`, its absolute
value remained `0`, and the computed predicate remained `early` before, during
and after the ADS correction sequence. The projection/tanf/atanf path was not
entered. This rejects `FUN_140AF4FA4` as the causal owner for the captured
post-cinematic weapon correction. The world-FOV propagation finding remains
valid, but this downstream branch is closed without expanding into renderer
search or changing production code.

The capture recorded 1914 PRE events. `source+0x25C` remained `0`, its absolute
value remained `0`, and the computed predicate remained `early` before, during
and after the ADS correction sequence. The projection/tanf/atanf path was not
entered. This rejects `FUN_140AF4FA4` as the causal owner for the captured
post-cinematic weapon correction. The world-FOV propagation finding remains
valid, but this downstream branch is closed without expanding into renderer
search or changing production code.

## Candidate promotion gate

A candidate must show more than a matching offset. It must have a bounded local
data-flow connecting the ADS/world-FOV signal to at least one of:

- first-person/viewmodel selection or enable state;
- camera/view construction or projection parameter derivation;
- a derived presentation transform/state write;
- a direct, low-fan-out caller relationship to the validated ADS/camera path.

One or two of these without ownership evidence is insufficient.

## Risks and safe-failure behavior

- Raw offset matches may be generic and are not promoted.
- Large/high-fan-out functions remain historical candidates unless a local block
  has a specific causal contract.
- Unknown ownership is recorded as `UNRESOLVED`, not converted into a hook
  proposal.
- No runtime or production behavior changes occur in this task.
- Agent-owned headless Ghidra sessions must exit and canonical project locks
  must be verified absent after every run.

## Hard stop condition

If bounded data-flow from the confirmed world-FOV signal does not reach a
specific consumer with sufficient camera/first-person/viewmodel ownership
evidence, stop. Do not expand into generic renderer/projection functions and do
not create a runtime tracer without a concrete target.

## Expected final Git review

For the current static phase, confirm only this plan, the two read-only helpers
and the report changed. Confirm no production source, ASI, runtime tracer or
historical conclusion was silently rewritten. The task-log entry remains
pending until the runtime gate resolves the candidate.
