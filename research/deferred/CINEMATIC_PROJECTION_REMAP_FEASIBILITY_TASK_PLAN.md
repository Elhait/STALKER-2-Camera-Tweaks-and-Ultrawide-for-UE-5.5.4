# Cinematic Projection Remap Feasibility — Task Plan

Status: Parent problem OPEN — bounded static ownership searches remain
exhausted, but current-build UE4SS runtime evidence now confirms the final
cinematic POV in PlayerCameraManager. Upstream POV evaluation owner remains
unresolved; R2 remains blocked.

## Objective

Determine whether the cinematic pipeline exposes a downstream projection
boundary that can change horizontal presentation independently of the authored
cinematic camera FOV. If such a boundary exists, test whether a non-linear or
otherwise bounded remap can keep the 16:9 authored composition close on 32:9
without the current full Hor+ FOV excursion.

## Established evidence and current state

- The current cinematic path uses rectilinear full Hor+ behavior.
- An authored 16:9 cinematic FOV of approximately 90 degrees becomes about
  126.87 degrees at 32:9 while preserving vertical coverage.
- The 16:9 @ authored 90-degree image is the visual reference.
- The 32:9 target should preserve the reference center and vertical scale as
  closely as practical, expose additional side content, reduce peripheral
  stretching and avoid an unnecessary 90-to-126.87 camera-FOV excursion.
- The current full Hor+ implementation remains the control baseline.
- A projection remap mechanism is not yet identified. `Panini` is a possible
  model, not an established engine mechanism or production feature.

### Current-build UE4SS runtime evidence

- The runtime object
  `PlayerCameraManager_2147479042` was observed across the same session in
  gameplay, cinematic and post-cinematic states. Object IDs are session-scoped
  and change only after a game reload; they are not treated as stable native
  addresses across sessions.
- Gameplay baseline at capture time:
  `CameraCachePrivate` and `LastFrameCameraCachePrivate` both reported
  `FOV=90.000000`, `AspectRatio=1.777778` and
  `bConstrainAspectRatio=false`.
- Stable cinematic state at capture time:
  both camera caches reported `FOV=126.869896`,
  `AspectRatio=3.555556` and `bConstrainAspectRatio=true`.
  `ViewTarget.Target` remained the player character and
  `PendingViewTarget.Target` was `None`.
- During the observed EXIT transition, the current/last-frame FOV values were
  `112.870773` and `113.723366`; `AspectRatio` was still `3.555556`, while
  `bConstrainAspectRatio` was already `false`.
- After the transition, both camera caches returned to
  `FOV=90.000000`, `AspectRatio=1.777778` and
  `bConstrainAspectRatio=false` at capture time.
- The inspected PCM child modifiers did not show an active contribution in
  the tested cinematic: `CameraAnimationCameraModifier.ActiveAnimations` was
  empty, `CameraModifier_LookAt.Alpha` was `0`, and
  `CameraModifier_CameraShake.ActiveShakes` was empty. This is a bounded
  negative for the direct child-modifier explanation, not proof that those
  classes can never participate upstream.
- The effective cinematic POV is therefore confirmed in
  `PlayerCameraManager.ViewTarget.POV` and
  `CameraCachePrivate.POV`. The evidence does not identify where that POV was
  calculated, nor does it establish the downstream projection/render consumer.
- Current-build UE4SS metadata adds a concrete game-specific owner candidate:
  `/Script/Stalker2.CameraManager` contains `CameraComponent`,
  `PlayerCameraManager`, `CachedCameraLookAtMod`, `FovMPC` and
  `EnvironmentMPC`. The object dump also contains a live instance:
  `CameraManager_2147482474` at `0x0000024BF75530D0`, owned by
  `BP_SML_C_2147482553`. This is a new object-ownership anchor, not yet proof
  that the object performs cinematic POV construction.
- The reflected `/Script/Stalker2.Obj:GetCameraManagerPtr` function returns a
  `/Script/Stalker2.CameraManager` object type. Its existence provides an
  independently identified game-specific acquisition path for the custom
  camera manager, subject to runtime instance validation.
- Evidence source: UE4SS individual dumps under
  `E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\ue4ss\IndividualObjectDumps`.

## Approved scope

### R1 — Projection ownership

- Identify the downstream cinematic projection consumer, matrix owner or
  equivalent X/Y projection-scale boundary.
- Establish executable/image identity before any static-analysis conclusion.
- Use existing runtime anchors and current 2.0.4 evidence where applicable.

### R2 — Remap feasibility

- Only after R1 identifies a safe observable boundary, test a bounded
  non-linear or projection-space remap against the reference images.
- Compare the candidate with the current rectilinear full Hor+ control.
- Evaluate center composition, vertical coverage, peripheral behavior, FOV
  excursion and cinematic EXIT handoff.

## Explicit non-goals

- No production cinematic FOV or aspect logic changes during R1.
- No dialogue, gameplay, resolver or hotkey changes.
- No guessed writes to camera offsets or projection matrices.
- No render-hook implementation.
- No assumption that the available mechanism is Panini.
- No user-facing `HorPlusStrength` configuration parameter.
- No production promotion without a separate approved implementation batch.

## Expected files or areas

- Ghidra/static-analysis reports and scripts only under the existing research
  areas.
- Runtime diagnostic build/log only if required by the approved batch.
- This task plan and a durable research report.
- Stable source, release assets and public documentation remain untouched
  unless a later implementation task is explicitly approved.

## Batches

### Batch 1 — R1 static ownership inventory

- Confirm executable identity using SHA-256, `.text` size, image base/sections
  and known runtime anchors before analysis.
- Inventory existing cinematic FOV/aspect boundaries and downstream consumers.
- Classify each candidate as confirmed, bounded lead, rejected or deferred.
- Stop if identity does not pass or if a candidate requires guessed writes.

Result: `research/reports/CINEMATIC_PROJECTION_REMAP_FEASIBILITY_R1_OWNERSHIP.md`
records `Identity: PASS` for the current Steam 2.0.4 image and the bounded
ownership inventory. The cinematic aspect store (`RVA 0x6B7CB05`) and live-FOV
boundaries (`RVA 0x2EE6936` / `0x2EE69A7`) are confirmed state/lifecycle
boundaries, not projection-only owners. The existing downstream chain through
`FUN_146B68976`, `FUN_1431D2094` and `FUN_1431D23AE` reaches durable state
handling; no temporary matrix, tan-half-FOV, projection-scale or
aspect-dependent X/Y consumer was established. Renderer/projection ownership
remains a valid deferred lead, even if a future implementation would require a
separate renderer-hook task. R1 passes; R2 is blocked and no production source
or release asset was changed.

### Batch 2 — R1 runtime correlation

Status: `BLOCKED`.

Reason: R1 did not establish a strongest statically valid downstream
projection candidate. Runtime correlation is therefore not justified for this
task. Do not select a previously rejected state or lifecycle candidate merely
to advance this batch.

- Correlate only the strongest statically valid candidate with the existing
  cinematic ENTER/steady/EXIT lifecycle.
- Confirm whether the candidate receives or owns projection state downstream
  of the authored camera FOV.
- Preserve the current full Hor+ behavior during the observation.

### Batch 3 — R2 bounded remap feasibility

Status: `BLOCKED`.

Reason: R2 requires a confirmed safe projection boundary, which R1 did not
establish.

- Build a test-only candidate only if R1 establishes a safe boundary.
- Compare 16:9 reference, current 32:9 full Hor+ and candidate 32:9 output.
- Validate cinematic ENTER, steady state, EXIT and gameplay recovery.
- Record visual and numeric limits; do not change production defaults.

### Batch 4 — PCM evaluation-path ownership (new runtime anchor)

Status: `NEXT / BOUNDED — R2 remains blocked`.

- Start from the confirmed live `PlayerCameraManager` POV oracle, not from
  another generic projection or matrix scan.
- Determine which player/controller/cinematic evaluation path supplies the
  effective `ViewTarget.POV` before it is mirrored into
  `CameraCachePrivate.POV`.
- Treat `UpdateViewTarget`, `CalcCamera`, Blueprint camera overrides and
  camera-cut evaluation as semantic candidates only; do not promote a name
  without current-build object identity or a confirmed downstream call/data
  path.
- Use read-only UE4SS/runtime observation or a concrete current-build native
  correspondence only. No guessed field writes, projection writes or
  production changes.
- The inspected PCM child-modifier branch is closed for the tested scene and
  must not be repeated without new evidence directly implicating one of those
  objects.
- Use the live `Stalker2.CameraManager` instance as the next bounded runtime
  object. Dump it in gameplay and stable cinematic states, then compare its
  `CameraComponent`, `PlayerCameraManager`, `CachedCameraLookAtMod` and
  `FovMPC` references. Do not write to any of these fields.

PASS requires a concrete current-build ownership/lifecycle bridge into the
effective POV. A useful partial result is a reproducible evaluation boundary
even if the ultimate projection consumer remains unresolved. If the search
requires broad renderer/matrix archaeology or loses object identity, stop and
record the branch as bounded unresolved/deferred.

## Validation

- Static conclusions are valid only for the identified executable image.
- Runtime claims are limited to the tested Steam build and exact diagnostic
  candidate identity.
- The 16:9 @ 90-degree reference and current 32:9 full Hor+ control must be
  retained for comparison.
- A candidate must not introduce a larger FOV excursion, visible EXIT defect,
  stale state or regression in the existing cinematic lifecycle.
- A renderer/projection boundary may be recorded as a valid owner or lead.
  R1/R2 must not implement a broad renderer hook; if production use requires
  one, record that requirement and stop before implementation.

## Risks and safe-failure

- If projection ownership is ambiguous, stop and preserve the current stable
  implementation.
- If a candidate requires guessed offsets or guessed matrix writes, reject the
  candidate.
- A renderer/projection boundary may be a valid ownership result even when a
  later production implementation would require a renderer hook. Record that
  requirement and stop before implementation.
- Test-only changes must remain isolated from the production release path.
- Any runtime regression requires discarding the candidate and returning to
  the unchanged full Hor+ control.

## Stop conditions and phase gates

- R1 must pass before R2 begins.
- Stop when no safe downstream projection owner can be established. Record the
  result as a bounded R1 pass with Batch 2 and R2 blocked; do not classify a
  future renderer/projection ownership lead as rejected solely because
  implementation may require a renderer hook.
- Do not reopen this task with a broader renderer scan. Any future search must
  be a separate approved task starting from UE5 view/projection construction:
  identify a current-build boundary where FOV and aspect/viewport dimensions
  become axis-specific projection state, then correlate only confirmed
  candidates back to cinematic ENTER/steady/EXIT.
- Stop before implementation if the identified owner requires a renderer hook
  outside this feasibility task.
- Stop on any identity mismatch or unresolved conflict with existing evidence.
- Stop before production implementation, public configuration changes or
  release preparation.

Parent problem status: `OPEN`. The two bounded static directions are
exhausted for the current evidence set: cinematic lifecycle/state flow toward
projection, and projection-construction indicators traced back toward
cinematic reachability. Further progress requires a new runtime/engine anchor
or a separate approved lower-level renderer/projection feasibility task.

## Expected final Git review

- Report exact research files created or modified.
- Report executable identity and static/runtime evidence boundaries.
- Classify confirmed owners, rejected candidates and deferred branches.
- Confirm stable source and release assets were not changed by feasibility work.
- Do not stage, commit, tag or push as part of this task.
