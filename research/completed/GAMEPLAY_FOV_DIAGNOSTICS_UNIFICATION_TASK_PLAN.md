# Gameplay FOV Diagnostics Unification Task Plan

## Objective

Make the passive Gameplay FOV source trace available in the normal ASI and
control it exclusively through `[Diagnostics] Enabled` in the INI. The existing
separate diagnostic profile remains available only for compile-time diagnostic
features and experimental hooks that are not safe or appropriate for the
normal build.

## Established evidence and current state

- The FOV source trace is attached to the existing Gameplay CameraWriter path;
  it does not install a new hook.
- The current trace is enclosed by `GAMEPLAY_FOV_SOURCE_TRACE_DIAGNOSTIC` and
  is therefore absent from the normal ASI.
- The runtime already has a shared `diagnostics::Enabled()` gate populated from
  `[Diagnostics] Enabled`.
- A separate diagnostic ASI was built for the current source-field experiment,
  but no new game run is part of this task.

## Approved scope

- Remove the compile-time-only requirement for this passive FOV trace.
- Guard its source reads, event comparison and logging with the existing
  runtime diagnostics setting.
- Remove the build-script switch that exists only for this trace.
- Update the diagnostic build wrapper/output description so it no longer implies
  that this trace requires a separate ASI.
- Build the normal ASI to a dedicated test artifact for later user testing.
- Record the source/build result and perform a read-only Git review.

## Explicit non-goals

- Do not remove or unify other compile-time diagnostic profiles.
- Do not change camera transforms, hooks, semantic snapshot logic or overlay UI.
- Do not launch the game or claim runtime validation.
- Do not overwrite the user's existing diagnostic ASI or unrelated artifacts.
- Do not stage, commit, reset or clean the worktree.

## Expected files or areas

- `src/plugin/runtime.cpp`
- `build.cmd`
- `build-gameplay-fov-source-diagnostic.cmd`
- `backlog/TASKLOG.md`
- `research/completed/` task-plan archive
- dedicated output under `build-artifacts/test-asi/`

## Batches and validation

### Batch 1 — runtime gate

Compile the passive trace in every build, but perform no source-field reads or
logging when `diagnostics::Enabled()` is false.

Validation: source review and test/build compilation.

### Batch 2 — build/profile cleanup

Remove the trace-specific compile define and make the helper wrapper build the
normal ASI to an isolated artifact with a descriptive name.

Validation: normal ASI build, artifact existence/size/hash, `git diff --check`.

### Batch 3 — final review

Compare changed paths against this plan, update the task log, and archive this
plan. Runtime testing remains with the user.

## Risks and safe-failure behavior

- The trace is passive and uses an existing hook path. If diagnostics are off,
  it returns before the additional source reads and has no behavior effect.
- If the source reads are invalid, existing `SafeRead` behavior reports them as
  unreadable; no camera state is written.
- If the build fails, preserve the existing ASIs and report the failure without
  changing runtime artifacts.

## Stop conditions and phase gates

- Stop if the trace requires a new hook or camera behavior change.
- Stop if the source/build diff touches unrelated overlay or camera logic.
- Stop before runtime launch; user will perform the later INI-controlled test.

## Expected final Git review

Confirm only the approved source/build/task-log/plan paths changed, identify
existing unrelated worktree changes separately, and report build evidence apart
from runtime evidence.
