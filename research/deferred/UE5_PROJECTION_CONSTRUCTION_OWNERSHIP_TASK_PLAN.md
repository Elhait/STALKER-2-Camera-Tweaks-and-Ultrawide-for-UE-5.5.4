# UE5 Projection Construction Ownership — Task Plan

Status: Batch 1 complete — no confirmed current-build projection owner;
Batch 2 blocked; bounded static direction exhausted.

## Objective

Identify a current-build UE5 view/projection-construction boundary where FOV
and aspect/viewport dimensions become axis-specific projection state. Then
determine whether the confirmed cinematic lifecycle reaches that boundary.

This task supplies a possible new entry point for the blocked
`CINEMATIC_PROJECTION_REMAP_FEASIBILITY` task. It does not implement a remap.

## Established evidence and current state

- Current Steam 2.0.4 executable identity is known and must pass before static
  interpretation: SHA-256
  `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`, image
  base `0x140000000`, `.text` raw size `130818560`.
- Cinematic aspect and live-FOV lifecycle boundaries are already established,
  but the bounded search from those boundaries did not find a projection-only
  owner.
- The current production cinematic path remains full rectilinear Hor+ and is
  the unchanged control baseline.
- The required target is a temporary projection boundary that could, in a
  later task, preserve the authored 16:9 composition more closely on 32:9
  without changing durable cinematic camera FOV.

## Approved scope

- Static, read-only ownership inventory for current Steam 2.0.4 evidence.
- Start from semantic projection-construction indicators:
  - FOV to tangent/tan-half-FOV or reciprocal focal-scale conversion;
  - viewport width/height or aspect consumption;
  - axis-specific X/Y projection scale or matrix construction;
  - temporary view/projection state passed toward rendering.
- Trace only enough callers/callees to establish whether a candidate is
  reachable from the known cinematic lifecycle.
- Classify candidates as `CONFIRMED OWNER`, `CONFIRMED DOWNSTREAM BOUNDARY`,
  `BOUNDED LEAD`, `REJECTED` or `DEFERRED / REQUIRES LOWER-LEVEL IMPLEMENTATION`.
- Record executable identity, addresses/RVAs, decoded instructions,
  caller/object context and evidence limits.

## Explicit non-goals

- No production source, ASI, INI or release-asset changes.
- No runtime tracer, game launch, injection or visual test.
- No guessed matrix/offset writes or hook implementation.
- No broad unbounded renderer, matrix or global FOV scan.
- No Panini implementation or `HorPlusStrength` configuration.
- No promotion of historical 2.0.3 findings as current 2.0.4 ownership.

## Files or areas expected to be touched

- This task plan.
- One durable report under `research/reports`.
- Existing research evidence may be read but must not be silently rewritten.
- No stable source, build output or release asset changes.

## Batches

### Batch 1 — Current-build projection-construction inventory

- Pass the current 2.0.4 identity gate.
- Inventory existing bounded evidence and identify only concrete semantic
  projection-construction candidates.
- Trace candidate data flow between FOV, aspect/viewport and temporary
  projection state, then correlate directionally back to cinematic lifecycle.
- Record candidates and stop if no safe current-build boundary is established.

Validation: static evidence tied to the identity header; decoded instruction
and caller/object evidence where a candidate exists. No runtime claims.

Result: `research/reports/UE5_PROJECTION_CONSTRUCTION_OWNERSHIP_BATCH1.md`
records `Identity: PASS` and a bounded negative result. Current-build
cinematic aspect/FOV boundaries are confirmed, but no temporary UE5
view/projection construction boundary combining FOV with aspect/viewport state
was established. Historical 2.0.3 `FUN_140186BE8` is not promoted: its callers
and data flow are tied to scene-resource/lighting processing rather than the
cinematic camera. Batch 1 passes as a completed bounded inventory; Batch 2 is
blocked because no strongest statically valid candidate exists.

This task's static direction is exhausted for the current evidence set. Do not
create another renamed static scan without a new concrete runtime or engine
side anchor. Any deeper renderer/projection work requires a separate approved
lower-level feasibility task.

### Batch 2 — Bounded lifecycle correlation

Status: `BLOCKED` pending a candidate from Batch 1.

- Correlate only one strongest statically valid candidate with cinematic
  ENTER/steady/EXIT.
- Confirm whether it is reached by cinematic presentation and whether it owns
  temporary projection state.
- Preserve the production full-Hor+ path; no implementation.

### Batch 3 — Architecture handoff

Status: `BLOCKED` pending Batch 2.

- If a projection boundary is confirmed, prepare a separate implementation or
  feasibility task for any required renderer hook.
- If no boundary is confirmed, record the negative result and stop without a
  workaround.

## Risks and safe-failure behavior

- Generic renderer/math helpers are not owners without cinematic reachability
  and object/data-flow evidence.
- A FOV conversion alone is not proof of projection ownership.
- A matrix-looking function that mutates durable camera state is not a
  projection-only boundary.
- Any identity mismatch, ambiguous calling convention or unsafe continuation
  stops the batch and preserves the current implementation.

## Stop conditions and phase gates

- Identity must be `PASS` before semantic interpretation.
- Stop on a broad scan that loses a concrete semantic/data-flow anchor.
- Stop if only generic renderer candidates are found without cinematic
  reachability.
- Stop before runtime correlation if no strongest statically valid candidate
  exists.
- Stop before implementation regardless of a positive ownership result; any
  renderer-hook work requires a separate approved task.

## Expected final Git review

- Report exact research files created or modified.
- Confirm no production source, ASI, INI or release asset changed.
- Separate confirmed ownership, bounded leads, rejected candidates, deferred
  lower-level work and not-runtime-validated claims.
- Do not stage, commit, tag or push as part of this task.
