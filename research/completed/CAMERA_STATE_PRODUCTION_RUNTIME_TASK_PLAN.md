# Production Camera State Runtime Task Plan

## Objective

Promote the existing `CameraStateSnapshot` composition into the shared
production runtime model used by diagnostics and the overlay. Add retained
neutral Gameplay FOV and coherent live Native/HorPlus FOV evidence without
creating a parallel state/store or changing camera correction behavior.

## Established evidence and current state

- `camera::CameraStateSnapshot` already models presentation, zoom, dialogue,
  gameplay mode, FOV evidence, source and generation provenance.
- Snapshot composition is currently under `CAMERA_STATE_SNAPSHOT_DIAGNOSTIC`.
- Zoom evidence is currently populated only under `ZOOM_TRANSITION_DIAGNOSTIC`.
- `GameplayBaselineStore` is production state used by cinematic integration and
  must retain its existing semantics; it is not suitable for retained neutral
  Gameplay FOV because it updates on eligible writer observations.
- The existing CameraWriter observation already provides a coherent input/result
  pair for Native/HorPlus FOV.
- The user runtime run established neutral endpoints around 90.66, 100.65 and
  110.66, while zoom endpoints such as 26.14 and 52.28 are transient camera
  values.

## Approved scope

- Make `CameraStateSnapshot` composition available in production.
- Promote only the existing zoom transition evidence needed to maintain the
  snapshot; preserve the validated hook/resolver contract and camera semantics.
- Keep diagnostic logging and trace formatting behind `Diagnostics.Enabled`.
- Add retained neutral Gameplay FOV inside the existing camera snapshot model.
- Capture neutral Gameplay FOV only from bounded neutral gameplay observations:
  stable Gameplay owner, no dialogue/cinematic/recovery, and no active zoom
  transition. Hold it through zoom and other non-neutral lifecycle states.
- Store the latest coherent CameraWriter Native/HorPlus pair in the same central
  snapshot model.
- Expose read-only camera FOV facts through the existing semantic snapshot and
  replace the user-facing Camera Integration `FOV path` row.
- Add deterministic tests for state composition, neutral retention, zoom hold,
  coherent pairs, lifecycle exclusions, invalid state and diagnostics ON/OFF
  production-state equivalence.
- Run `test.cmd`, production build, overlay build and `git diff --check`.
- Do not launch the game.

## Explicit non-goals

- Do not change `GameplayBaselineStore`, HorPlus transforms, gameplay correction,
  cinematic behavior, dialogue behavior or recovery behavior.
- Do not use `source+0x234`, offsets, rounding or an inferred slider offset.
- Do not create a second neutral-FOV store, classifier layer or new camera hook.
- Do not make diagnostics a source of state; diagnostics consume the shared
  snapshot.
- Do not claim runtime validation from deterministic tests or builds.

## Expected files or areas

- `src/camera/camera_state_snapshot.hpp/.cpp`
- `src/plugin/runtime.cpp`
- `src/plugin/runtime_settings.hpp`
- `src/overlay/camera_integration.hpp/.cpp`
- relevant deterministic harnesses and `test.cmd`
- `build.cmd` / overlay build script if required by shared production sources
- `backlog/TASKLOG.md`

## Implementation batches

### Batch 1 — shared snapshot and zoom evidence

Move snapshot composition and the minimal existing zoom transition evidence out
of diagnostic-only compilation. Maintain a thread-safe current snapshot owned by
the existing runtime state and gate only diagnostic output on the INI setting.

### Batch 2 — FOV semantics

Add retained neutral Gameplay FOV and coherent live Native/HorPlus fields to the
existing snapshot/evidence model. Update retained neutral state only at the
bounded neutral capture condition; reject zoom, dialogue, cinematic, recovery
and transient observations.

### Batch 3 — semantic overlay projection

Expose the central snapshot through the existing read-only semantic API and
render Gameplay/Native/HorPlus FOV values in Camera Integration with unavailable
states handled explicitly.

### Batch 4 — deterministic validation and builds

Add/update harnesses, run the full test suite, production build, overlay build and
diff checks. Review changed paths against this plan and archive the plan.

## Risks and safe-failure behavior

- Promoted zoom hooks remain guarded by the existing executable identity and
  instruction validation. Failure leaves zoom evidence unavailable and does not
  affect camera correction.
- If no confirmed neutral observation exists, retained Gameplay FOV remains
  invalid/unavailable; the overlay must not invent a value.
- Snapshot publication uses a coherent copy under the existing runtime owner;
  diagnostics and overlay read the same published state.
- On build or test failure, preserve existing ASIs and do not launch the game.

## Stop conditions and phase gates

- Stop if promoting zoom evidence requires a new signature, altered instruction
  contract or camera write.
- Stop if any change modifies `GameplayBaselineStore` or correction behavior.
- Stop if the neutral capture condition cannot be represented without a new
  classifier or parallel store; report the boundary instead.
- Stop before runtime launch regardless of static validation outcome.

## Expected final Git review

Compare changed paths with this plan, separate pre-existing worktree changes,
record deterministic/build evidence separately from runtime evidence, and add a
factual task-log entry before archiving this plan.
