# Gameplay FOV Source Diagnostic — Task Plan

## Objective

Build a fresh, isolated diagnostic ASI that determines whether the live Gameplay camera source field at `source+0x234` tracks the selected Gameplay FOV through settings changes, ADS/binocular zoom, and camera/lifecycle changes. Capture only meaningful changes at the already validated CameraWriter boundary.

## Established evidence and current state

- Historical runtime notes correlate `secondaryFOV` at `source+0x234` with the selected Gameplay FOV (90 setting → 90 field); `primaryFOV` was described as transient/current camera state.
- Current diagnostics can read the field as `cameraFirstPersonFov`, but existing mode-change logging may not capture every settings/ADS transition.
- The active CameraWriter callback already owns a validated live source pointer and coherent input/result FOV values. No new hook is needed.
- Source-flow review confirms the active `HorPlus` branch calls `ApplyHorPlusGameplay` for each CameraWriter invocation; the trace therefore runs in the requested HorPlus test mode.
- Existing diagnostic ASI and shared object output paths are present; this task must use a distinct ASI filename and a distinct object directory to preserve them.
- Relevant assistant-guideline documents listed in the workspace AGENTS.md are absent from this checkout; repository AGENTS.md and the existing build/test scripts govern this bounded task.

## Approved scope

- Add compile-gated, read-only, change-only telemetry sampling `source+0x234` in the existing CameraWriter path.
- Each trace event records sequence, thread, source identity, candidate value/read validity, writer input, HorPlus result, aspect, flags, and coordinator/owner state.
- Preserve one coherent event record; do not retain the candidate as production configuration state.
- Add a dedicated build entrypoint/configuration that writes to a unique diagnostic ASI and isolated object directory.
- Build and statically review the diagnostic ASI; provide concise run instructions for the user-controlled runtime session.

## Explicit non-goals

- No production ASI behavior, FOV math, hook resolution, hook count, settings/UI, or overlay changes.
- No writes to game memory and no new hook, polling loop, or per-frame log spam.
- No claim that `+0x234` is the selected setting until runtime evidence confirms it.
- Do not launch the game, install/copy the ASI into the game directory, or overwrite existing diagnostic/release artifacts.
- No Ghidra analysis, release packaging, commit, or Git staging.

## Expected files or areas

- `src/plugin/runtime.cpp` — compile-gated read-only trace at the current writer callback.
- `build.cmd` and a dedicated `build-gameplay-fov-source-diagnostic.cmd` wrapper — opt-in macro plus isolated object/output paths.
- `GAMEPLAY_FOV_SOURCE_DIAGNOSTIC_TASK_PLAN.md` during execution; archive to `research/completed/` after review.
- `backlog/TASKLOG.md` after build and final review.
- New outputs: `build-artifacts/test-asi/fov-source-diagnostic/STALKER2CameraTweaksGameplayFovSourceDiagnostic.asi` (plus linker side files in that dedicated folder) and `build-artifacts/obj-gameplay-fov-source-diagnostic/`.

## Batches and validation

1. Implement diagnostic-only sample/change detection and a unique isolated build profile. Review preprocessor scope to ensure normal/production compilation has no added sampling or logging.
2. Run `build-gameplay-fov-source-diagnostic.cmd`; confirm the expected ASI exists, is non-empty, and the existing diagnostic ASI/shared object directory were not overwritten.
3. Run scoped `git diff --check` and inspect exact changed paths/diffs against this plan. Verify both the dedicated wrapper and `build.cmd` resolve paths from the repository location. Do not launch the game.

## Risks and rollback / safe failure

- Reading `+0x234` is diagnostic-only and occurs only with the live source pointer supplied to the existing validated callback. Failed reads or invalid floats are logged as unavailable; they are never substituted from writer input/result.
- Change-only comparison is per callback thread and includes source identity so camera recreation produces a new event. No game-state write or control-flow decision may depend on the sample.
- If the instrumented branch cannot be isolated behind its build macro, stop before building; do not change production behavior to make the trace work.
- The output path and object directory are unique. If either unexpectedly exists before the build, stop and select a new reviewed path rather than overwrite it.
- To rollback, remove only the exact newly created ASI/object directory and revert only the exact source/build-script changes from this task after checking for user edits; never use broad cleanup.

## Stop conditions and phase gates

- Stop before implementation if the existing validated writer boundary does not provide a valid source pointer or a coherent input/result pair.
- Stop if isolated build support would require changing unrelated build behavior.
- The build gate proves compilation only. Runtime semantics remain pending until the user runs the diagnostic ASI alone and supplies its log.

## Final Git review

- Inspect status, staged and unstaged diffs, changed paths, and recent commits read-only.
- Preserve all pre-existing staged, modified, and untracked work; do not stage or commit.
- Report completed, remaining, deferred, blocked, and not-runtime-validated separately.
- Archive this plan under `research/completed/` only after the build and final review.

## Execution result — 2026-09-22

- Implemented change-only trace under `GAMEPLAY_FOV_SOURCE_TRACE_DIAGNOSTIC`; source-flow review confirmed the regular HorPlus CameraWriter branch invokes the instrumented function.
- Trace includes timestamped logger output, trace and writer sequence, source/thread, `source+0x234` candidate/read validity, `source+0x230` comparison field, writer input, HorPlus result, aspect/flags, coordinator, Gameplay Enabled/Mode and transform eligibility/result.
- Added isolated wrapper and root-relative build handling. The pre-existing `STALKER2CameraTweaksDiagnostic.asi` and shared `build-artifacts/obj` were not targeted.
- Dedicated diagnostic build: PASS. Artifact: `build-artifacts/test-asi/fov-source-diagnostic/STALKER2CameraTweaksGameplayFovSourceDiagnostic.asi`, 1,221,120 bytes, SHA-256 `0AC546F0982E41F0684E860301821D525BC3F8E6C61BB55B957A9B2B08948FAF`.
- Static marker check found `GAMEPLAY_FOV_SOURCE_TRACE`, `selectedCandidate(+0x234)`, and `gameplayMode=` in the artifact. Scoped `git diff --check` passed; only existing LF/CRLF normalization warnings were emitted.
- No game launch or runtime test. Selected-setting semantics and ADS/binocular persistence remain unvalidated until the user runs this ASI and shares the resulting log.
