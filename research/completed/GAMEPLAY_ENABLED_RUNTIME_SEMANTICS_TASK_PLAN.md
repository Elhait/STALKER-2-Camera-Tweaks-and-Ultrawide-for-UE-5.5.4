# Gameplay.Enabled Runtime Semantics — Task Plan

## Objective and established evidence
- Repair live Gameplay.Enabled semantics through existing safe game camera lifecycle paths.
- User reports the overlay checkbox changes but gameplay behavior does not. `ReplayManualTransition` currently early-returns when disabled, without scheduling restoration. Existing mode retry deferred on `actual_runtime_aspect_not_established`.
- Runtime game launch is expressly prohibited for this batch.

## Approved scope
- Trace Enabled/Mode/pending/coordinator/replay dataflow and existing native restoration path.
- Implement safe disable/enable apply/defer semantics, with disabled authoritative and latest selected mode used after re-enable.
- Add minimal pure decision/helper coverage and integrate focused tests into `test.cmd` if needed.
- Add explicit request/applied/deferred/failure and pending-recovery telemetry.
- Write factual report and archive this plan after review.

## Non-goals
- No overlay UI/API redesign, resize/fullscreen/D3D12, F9-F12, camera math, new hook, timer retry, Dialogue/Cinematics behavior changes, game launch, commit/release, or unrelated cleanup.

## Expected paths
- `src/plugin/runtime.cpp`; optionally `src/gameplay/gameplay_state.hpp/.cpp`; focused `tests/gameplay/*`, `test.cmd`; `research/reports/GAMEPLAY_ENABLED_RUNTIME_SEMANTICS_REPAIR.md`; this plan.

## Batches and validation
1. Confirm callback ordering and why the prior retry deferred.
2. Implement the smallest safe transition using existing native evidence; fail closed if inputs are unavailable.
3. Cover enable/disable, mode preservation/latest mode, deferred state and stale pending suppression deterministically.
4. Run focused harness, `test.cmd`, `build.cmd`, `build-diagnostic.cmd`, combined overlay build, `git diff --check`; read-only Git review.

## Risks, stop conditions, final review
- Never guess FOV/aspect or restore from stale state. If a safe native restoration path cannot be established from source, stop and report the blocker.
- Keep feature state isolated from Dialogue/Cinematics; preserve pre-existing user changes.
- Stop after static/tests/builds; report runtime as NOT_PERFORMED and make no commit.
- Final review compares all changed paths against this plan and separates completed, remaining, deferred, blocked and not-runtime-validated work.
