# Cinematic Axis-Constraint Runtime Reflection — Task Plan

Status: `BATCH 1 COMPLETE / A2.1-A2.3 PASS/PARTIAL; A2.4 bounded partial, no promotable consumer`.

## Objective

Determine whether the existing diagnostic ASI infrastructure can use Unreal
reflection at runtime to identify and read, without writes, the effective
aspect-ratio axis policy layers relevant to cinematic camera cuts:

```text
/Script/Engine.CameraComponent
  bOverrideAspectRatioAxisConstraint
  AspectRatioAxisConstraint

/Script/Engine.LocalPlayer
  AspectRatioAxisConstraint

CameraComponent::SetAspectRatioAxisConstraint
```

The desired output is owner, property type, offset, enum identity and safe
read-only values for gameplay, cinematic ENTER/steady and EXIT where the
runtime object can be identified safely.

## Established evidence and current state

- Steam 2.0.4 `global.utoc` ScriptObjects contains the relevant reflected
  engine vocabulary and class/function relationships.
- `SetAspectRatioAxisConstraint` is nested under reflected `CameraComponent`.
- `EAspectRatioAxisConstraint`, `LocalPlayer`, `CineCameraComponent`,
  `PlayerCameraManager` and camera-cut metadata are present.
- The current executable contains matching reflected strings, but no native
  function binding, field offset or runtime consumption has been proven.
- Cooked asset property interpretation is limited by the absence of a matching
  `.usmap`; runtime reflection may bypass that limitation.

## Approved scope

- Inspect existing diagnostic/runtime infrastructure and its safe lifecycle.
- If infrastructure is suitable, add one diagnostic-only, read-only tracer
  under the research/experimental path.
- Resolve `UClass`, properties and `UFunction` by reflection name where the
  runtime APIs are safely available.
- Log owner class, property name, property type, offset, enum identity and
  readable instance values.
- Correlate observations with gameplay, cinematic ENTER, steady cinematic and
  EXIT only after object discovery is safe and bounded.
- Tie all executable observations to the known Steam 2.0.4 runtime identity.
- Characterize the already-known live camera/source pointer passed through the
  validated gameplay writer using static producer/caller provenance.
- Keep this characterization restricted to that known pointer; no generic
  pointer or UObject registry scan is permitted.

## Explicit non-goals

- No writes to enum fields, override flags, camera objects or projection state.
- No production ASI, INI, release asset or stable source behavior changes.
- No guessed UObject offsets, guessed enum values or arbitrary pointer scans.
- Do not read `UObject + 0x10` or any other presumed UE layout field without
  independent type/layout evidence.
- No native setter hook or projection hook in this task.
- No broad renderer scan, package repack or asset modification.
- No claim that a reflected property is effective for cinematics merely because
  it exists or is readable.

## Files or areas expected to be touched

- This task plan.
- One durable report under `research/reports`.
- Existing diagnostic source/build helper only if a bounded tracer is needed.
- Diagnostic build output may be placed under the existing research/build
  artifact policy; production output must remain untouched.

## Batches

### Batch 1 — Infrastructure and API feasibility

Status: `COMPLETE / BLOCKED OUTCOME`.

- Inspect existing ASI diagnostics for safe module/object discovery,
  logging and lifecycle gating.
- Determine whether runtime reflection APIs or stable reflected globals are
  already available without guessed addresses.
- Confirm how the tracer can identify the current LocalPlayer/CameraComponent
  instances without broad pointer scanning.

Validation: read-only infrastructure review and a bounded feasibility result;
no game launch or source behavior change is implied by this batch.

Result: no existing safe UE reflection bridge was found in the ASI diagnostic
infrastructure. There are no reusable `UObject/UClass/FProperty/UFunction`
types or resolved object-registry globals. See
`research/reports/CINEMATIC_AXIS_CONSTRAINT_RUNTIME_REFLECTION_BATCH1.md`.
Batch 2 remains blocked: Track B establishes a safe observation of the known
writer input, but not a safe UE reflection/object access path. Track A native
correspondence is still ready for bounded work.

### Track A — Setter-family/native correspondence

Status: `A1 COMPLETE; A2.1/A2.2 PASS; A2.3 PARTIAL PASS; A2.4 bounded partial`.

Canonical phase mapping for this track:

```text
A2-M1  property metadata route       — historical bounded negative
A2-M2  schema acquisition route      — historical bounded negative
A2.1  class-level registration family — PASS
A2.2  setter-path decode               — PASS
A2.3  source reader / writer bridge   — PARTIAL PASS
A2.4  output axis-state consumer      — ACTIVE
```

The `A2-M1` and `A2-M2` labels are planning aliases only; historical reports
are not renamed or rewritten.

- Treat `SetAspectRatioAxisConstraint` as a metadata/layout anchor, not the
  effective cinematic owner.
- Inspect sibling reflected `CameraComponent` functions together:
  `SetAspectRatioAxisConstraint`, `SetAspectRatio`, `SetFieldOfView` and
  `SetConstraintAspectRatio` where present.
- Track both `bOverrideAspectRatioAxisConstraint` and
  `AspectRatioAxisConstraint` through any native correspondence.
- If a field candidate is established, enumerate readers and classify them as
  defaults/serialization, camera/view construction or projection policy.
- No writes or hook installation.

Validation: current 2.0.4 metadata/native correspondence with decoded
instruction and caller/object context. A string xref alone is only a lead.

Track A1 result: the current executable and `global.utoc` confirm the
reflected setter-family vocabulary, but the bounded metadata/string probe did
not establish a native registration record, thunk, implementation or field
offset. See
`research/reports/CINEMATIC_AXIS_CONSTRAINT_TRACK_A1_METADATA_FINGERPRINT.md`.

### Track A2 — Three bounded evidence routes

Status: `A2.1-A2.2 PASS; A2.3 PARTIAL PASS — +0x258 reaches known camera/view output state; projection owner unresolved`.

- A2.1: reconstruct a class-level native-registration fingerprint from a
  wider bounded set of reflected `CameraComponent` sibling functions. Stop
  if no shared registration structure appears.
- A2.2: query property-level metadata for
  `AspectRatioAxisConstraint`,
  `bOverrideAspectRatioAxisConstraint` and
  `LocalPlayer::AspectRatioAxisConstraint`.
- A2.3: search for a matching current-build unversioned-property schema or
  `.usmap` with explicit provenance.

A2.2 found the reflected class/type vocabulary but no property descriptors or
field offsets. A2.3 found no matching current-build schema. See
`research/reports/CINEMATIC_AXIS_CONSTRAINT_TRACK_A2_METADATA_SCHEMA.md`.
Neither result authorizes a runtime read or write. Track A2.1 remains the
only pending route in this bounded cycle. Its first invocation failed before
script execution; the corrected tooling mini-pass now completes, but the
canonical 2.0.4 program has no containing function at the known anchor or
the exact string xref. See
`research/reports/CINEMATIC_AXIS_CONSTRAINT_TRACK_A2_1_REGISTRATION_ATTEMPT.md`
and
`research/reports/CINEMATIC_AXIS_CONSTRAINT_GHIDRA_TOOLING_VALIDATION_204.md`.
This is not a negative registration result. The subsequent read-only database
readiness audit found the canonical 2.0.4 program with 412,707 functions and
30,458,375 instructions; ENTER/EXIT anchors resolve to an enclosing function.
See
`research/reports/CINEMATIC_AXIS_CONSTRAINT_GHIDRA_DATABASE_READINESS_204.md`.
A2.1 is now complete: a shared registration-family region provides four
reproducible name-to-.text target bindings. See
`research/reports/CINEMATIC_AXIS_CONSTRAINT_TRACK_A2_1_REGISTRATION_FAMILY.md`.
Setter-path decoding is now complete. `SetAspectRatioAxisConstraint` writes
the byte at `object + 0x258`; `SetAspectRatio` corroborates `+0x254`, while
the constraint-related bitfield at `+0x259` and FOV/selector path at `+0x262`
remain semantically classified but not promoted to reflected property names.
See
`research/reports/CINEMATIC_AXIS_CONSTRAINT_TRACK_A2_2_SETTER_PATH.md`.
The reader audit confirmed that the known camera/view writer conditionally reads
`source +0x258` when a `source +0x259` mask `0x2` is set, then copies the value
into an output view-state region. This is a camera/view-state bridge, not yet a
projection-matrix or effective axis-policy owner. See
`research/reports/CINEMATIC_AXIS_CONSTRAINT_TRACK_A2_3_READER_AUDIT.md`.

### A2.4 — Output axis-state consumer

Status: `COMPLETE — bounded partial; no promotable effective consumer`.

Start only from the current-build output fields established by A2.3:

```text
RBX +0x60  axis-constraint value candidate
RBX +0x64  validity/presence candidate
```

Goal: identify the first direct consumer that tests `+0x64`, consumes `+0x60`
and also carries camera/view FOV or aspect context. A meaningful intermediate
view-policy consumer is sufficient; a final projection-matrix owner is not
required for this batch.

Non-goals: generic `+0x60`/`+0x64` scans without the output-object provenance,
generic matrix/tangent searches, renderer-wide scans, runtime writes and
production changes.

Validation: current Steam 2.0.4 identity gate, bounded direct-reference
inventory and instruction-context classification. The inventory produced only
generic/copying candidates; no camera/FOV/aspect consumer was promoted. See
`research/reports/CINEMATIC_AXIS_CONSTRAINT_TRACK_A2_4_OUTPUT_CONSUMER.md`.

### Track B — Known camera/source object characterization

Status: `COMPLETE — provenance established; UE object type unresolved`.

- Start from the source pointer already passed to the validated camera writer.
- Use static producer/caller provenance to determine whether it is a
  `CameraComponent`, subclass, embedded camera state, view-description-like
  structure or another game-owned camera object.
- Compare vtable/type/ownership evidence only when independently established.
- Relate the known source state to the cinematic FOV/aspect lifecycle.

Validation: static provenance of the pointer's producer, caller and object
role. No guessed UObject layout, generic memory scan or pointer enumeration.

Result: the validated Steam 2.0.4 camera writer receives the known source
pointer in `RSI` and copies source camera state into the output view state in
`RBX`. The source state exposes the already validated FOV/aspect-related
fields used by the writer: primary FOV at `+0x230`, secondary FOV at
`+0x234`, aspect at `+0x254`, flags at `+0x259` and a selector at `+0x262`.
The output writes include FOV at `+0x30` and aspect at `+0x5C`.

This confirms a game-owned camera/source state block and its role in the
camera-view copy path. It does not establish that the pointer is a
`UCameraComponent`, a reflected UObject, a camera subclass, or an embedded
`FMinimalViewInfo`. Vtable identity, UObject class identity, producer chain
and axis-constraint ownership remain unresolved. Reading `UObject + 0x10`
or any presumed UE layout remains prohibited.

See `research/reports/CINEMATIC_AXIS_CONSTRAINT_RUNTIME_REFLECTION_TRACK_B.md`.

### Batch 2 — Diagnostic-only reflection tracer

Status: `BLOCKED` pending Track A or Track B establishing a safe access path.

- Implement only if Batch 1 establishes a safe access path.
- Resolve the named classes/properties/functions by reflection.
- Log metadata and read-only values; fail closed on missing class/property,
  ambiguity, invalid object or unsafe lifecycle state.
- Build the diagnostic artifact separately from production.

Validation: compile/build correctness plus explicit limits; build success is
not runtime proof.

### Batch 3 — Runtime observation

Status: `BLOCKED` pending Batch 2.

- Launch only the diagnostic artifact against Steam 2.0.4.
- Observe gameplay, cinematic ENTER, steady cinematic and EXIT.
- Record whether CameraComponent override, LocalPlayer fallback or another
  reflected layer changes or remains stable.

Validation: runtime log and user-visible behavior, with no writes.

### Batch 4 — Decision

Status: `BLOCKED` pending Batch 3.

- Promote only a reproducible owner/value/lifecycle correlation.
- If reflection is unavailable or values are not effective, classify the
  result as bounded negative/partial and route to native correspondence.
- Any test-only policy write requires a separate approved task.

## Risks and rollback / safe failure

- UE reflection globals or object lifetimes may be unavailable or unsafe from
  an injected ASI context.
- A valid property may be inherited, overridden or synthesized elsewhere.
- Diagnostic object discovery can become a broad pointer scan; stop before that.
- On ambiguity, invalid pointers, missing APIs or lifecycle uncertainty, log a
  bounded failure and do not install hooks or write memory.
- Remove or ignore only the diagnostic artifact within its scoped research
  location if the task is abandoned; do not touch production files.

## Stop conditions and phase gates

- Stop before source edit if no safe reflection access path exists.
- Stop before build if implementation would require guessed offsets or writes.
- Stop before runtime if the diagnostic artifact cannot fail closed.
- Stop if object discovery expands into a generic memory scan or renderer hook.
- Stop before any MaintainYFOV write; that is outside this task.

## Expected final Git review

- Review only the exact diagnostic/report paths touched by this task.
- Separate metadata proof, runtime observation, unresolved ownership and
  not-runtime-validated claims.
- Confirm production source, ASI, INI, release assets and game packages were
  not changed.
- Do not stage, commit, tag or push as part of this task.
