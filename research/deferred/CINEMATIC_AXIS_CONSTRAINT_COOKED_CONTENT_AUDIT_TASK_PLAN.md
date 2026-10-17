# Cinematic Axis-Constraint Cooked Content Audit — Task Plan

Status: `BATCH 1 COMPLETE / BATCH 2A IN PROGRESS / BATCH 2B PARTIAL PASS`.

## Objective

Determine whether STALKER 2 Steam 2.0.4 cooked content explicitly serializes
or references aspect-ratio axis policy, camera-component overrides or related
cinematic camera configuration that can provide a concrete anchor for the
unresolved `MaintainYFOV` research direction.

This task audits the game-content/configuration layer. It does not claim to
enumerate all capabilities of the native UE engine, whose implementation is
in the shipping executable.

## Established evidence and current state

- UE 5.5 documents `AspectRatioAxisConstraint` at LocalPlayer,
  `FMinimalViewInfo`, camera-component and camera-cut layers.
- Current Steam 2.0.4 static correspondence did not establish an effective
  axis-constraint owner or safe runtime boundary.
- The previous static ownership direction is complete with no promotable
  current-build boundary; this task introduces a different evidence class.
- Production Full Hor+ remains unchanged and is the control baseline.

## Approved scope

- Steam 2.0.4 game packages and their associated cooked-content metadata only.
- Inventory package/container formats, mappings and available extraction or
  inspection tools before interpreting serialized properties.
- Search for these property and enum concepts where representation permits:

  ```text
  AspectRatioAxisConstraint
  bOverrideAspectRatioAxisConstraint
  AspectRatio_MaintainYFOV
  AspectRatio_MaintainXFOV
  AspectRatio_MajorAxisFOV
  ConstrainAspectRatio
  bConstrainAspectRatio
  FieldOfView
  AspectRatio
  ```

- Identify relevant cooked classes/assets, including LocalPlayer-like,
  PlayerCameraManager, CameraComponent/CineCameraComponent and
  LevelSequence/CameraCut content.
- Record package, asset/class/property provenance and mapping/version limits.

## Explicit non-goals

- No executable patching, ASI changes, INI changes or production source edits.
- No runtime MaintainYFOV write or guessed enum/object offset.
- No deletion, repacking or modification of game packages.
- No claim that absence from cooked content proves the engine feature absent.
- No broad executable renderer scan as a substitute for package analysis.

## Files and areas expected to be touched

- This task plan.
- One durable report under `research/reports`.
- Optional bounded inspection notes or scripts only inside the approved
  research areas if an existing tool cannot produce reproducible output.
- No stable source, release asset, game package or runtime artifact changes.

## Batches

### Batch 1 — Package and mapping inventory

Status: `COMPLETE / POSITIVE METADATA ANCHOR`.

- Locate the Steam 2.0.4 package/container set and verify exact paths.
- Identify `.pak`, IoStore `.utoc`/`.ucas`, mapping/schema and metadata files.
- Identify available read-only inspection/extraction tooling.
- Record content-version and mapping limitations before property search.

Validation: inventory with exact paths, file sizes/timestamps where useful,
container type and tool/mapping provenance. Do not modify packages.

Result: `research/reports/CINEMATIC_AXIS_CONSTRAINT_COOKED_CONTENT_AUDIT_BATCH1.md`
records the installed 2.0.4 package set, IoStore directory-index access,
available `retoc`/`repak` tooling and a positive `global.utoc` reflection
anchor containing `EAspectRatioAxisConstraint`,
`SetAspectRatioAxisConstraint`, LocalPlayer/camera classes,
`CalculateProjectionMatrix` and `MovieSceneCameraCutSection`. No compatible
`.usmap` or equivalent property schema was found in the workspace inventory;
serialized-value interpretation remains gated.

### Batch 2A — Cooked ownership/content correlation

Status: `IN PROGRESS / OWNER NOT YET ESTABLISHED`.

- Use `global.utoc` script-object metadata to identify exact outer/index
  relationships for camera and cinematic classes/functions.
- Search only content relevant to LocalPlayer-like, PlayerCameraManager,
  CameraComponent/CineCameraComponent and MovieScene camera-cut ownership.
- Establish STALKER-specific class/asset relationships before attempting to
  interpret serialized property values.
- Inspect serialized properties using a matching mapping/schema where
  available; distinguish names, enum values and raw numeric fields.
- Classify findings as `CONFIRMED GAME-CONTENT USAGE`, `BOUNDED LEAD`,
  `NO COOKED CONTENT OVERRIDE FOUND` or `UNRESOLVED`.

Validation: package/class/asset provenance and metadata indices. A missing
`.usmap` limits property-value interpretation but does not block owner
relationship analysis.

### Batch 2B — Reflection-to-native correspondence

Status: `PARTIAL PASS / BOUNDED LEAD`.

- Start from the reflected `SetAspectRatioAxisConstraint` and
  `CalculateProjectionMatrix` identities.
- Record global index, outer index, CDO/class relationship and neighboring
  reflected objects/functions.
- Determine whether the metadata can be correlated to a native registration,
  exec thunk, exported symbol, byte pattern or current-build executable
  function without guessing an object offset.
- If a setter/field/readers chain is established, classify it as a candidate
  anchor; do not write to it.

Validation: reproducible metadata-to-native correspondence tied to the Steam
2.0.4 identity. Current-image strings are a lead, not a native function
address, registration binding, property offset or runtime proof. Reflection
presence alone is not native implementation proof.

### Batch 3 — Correlation and next-step decision

Status: `BLOCKED` pending completion of 2A/2B native/content correlation.

- Correlate any positive class/property/asset or native correspondence finding with the known cinematic
  lifecycle only if provenance is concrete.
- Promote a result only when the asset/class/property owner and serialized
  value are reproducible.
- If no override is found, preserve the weaker conclusion that no cooked
  content override was found; do not infer native absence.

## Risks and safe failure

- Cooked UE5 content may use IoStore, unversioned property serialization or
  encrypted/unsupported mappings.
- A package search may find unrelated engine/plugin content; asset context is
  required before promotion.
- Numeric enum values without a valid schema are not sufficient by themselves.
- Stop on missing/mismatched mapping, destructive tooling or any request to
  repack/modify packages.

## Stop conditions and phase gates

- Stop if the package set cannot be tied to Steam 2.0.4 content.
- Stop before interpreting raw values when no matching mapping/schema exists.
- Stop on broad indiscriminate extraction once relevant package classes/assets
  can no longer be bounded.
- A negative result must be reported as `NO COOKED CONTENT OVERRIDE FOUND`,
  never as proof that UE does not support the mechanism.
- Any runtime or production change requires a separate approved task.

## Expected final Git review

- Report exact research files created or modified and package paths read.
- Separate confirmed content usage, bounded leads, negative findings,
  unresolved mapping limits and deferred native ownership.
- Confirm no package, executable, source, ASI, INI or release asset was changed.
- Do not stage, commit, tag or push as part of this task.
