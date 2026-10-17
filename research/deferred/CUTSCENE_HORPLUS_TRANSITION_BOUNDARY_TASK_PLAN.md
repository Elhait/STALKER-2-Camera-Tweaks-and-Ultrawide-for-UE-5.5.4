# Cutscene Hor+ Transition Boundary Diagnostic Task Plan

Status: In progress — Batch 1 instrumentation is implemented; Batch 2 runtime correlation is pending.

## Objective

Identify the semantic boundary between the active cinematic phase and the cinematic-to-gameplay camera transition. Determine why the validated cinematic Hor+ conversion remains active long enough to produce a temporarily oversized FOV after cutscene exit.

## Established evidence and current state

- The current executable is S.T.A.L.K.E.R. 2 version 2.0.3, tested at native `5120x1440` (32:9).
- A/B letterbox hooks produce native `5120x1440` cinematic rendering without the previous crash.
- Dynamic live-FOV Hor+ conversion produces correct-looking cinematic framing.
- After cutscene exit, FOV is temporarily too large and then returns smoothly to the correct gameplay value.
- The current hypothesis is that the validated cinematic FOV entry remains active during camera blend/transition; this is not yet an ownership conclusion.
- No frame delay, timer, replay of the gameplay hook or hard-coded FOV compensation is accepted.

## Approved scope

- Add or use a narrowly scoped diagnostic tracer around the validated cinematic FOV entry and existing A/B setter activity.
- Record semantic state needed to distinguish cinematic-active, pre-end blend, cutscene exit and gameplay phases.
- Correlate FOV-hook hits with source FOV, converted FOV, live aspect, game-owned cinematic global, sequence number and manual event markers.
- Compare vanilla 16:9 and experimental native 32:9 where needed, without changing the experimental policy.
- Use the result to identify a natural state/event gate for the Hor+ policy, if one is evidenced.

## Explicit non-goals

- Do not add frame delays, timers, cooldowns or guessed post-cutscene windows.
- Do not change the Hor+ formula, A/B setter implementation or stable gameplay ASI.
- Do not write game-owned FOV/global state or call downstream game functions from the tracer.
- Do not claim that a FOV-hook hit is causal without semantic correlation.
- Do not implement a transition gate until the diagnostic evidence establishes its state/event contract.

## Expected files and areas

- `02-Research/Ghidra/ghidra-scripts/` only if an existing reusable research helper must be updated.
- `src/cutscene_fov_semantics_trace.cpp` or a narrowly named successor for the diagnostic tracer.
- `build-artifacts/test-scripts/` for the tracer build script if required.
- `build-artifacts/test-asi/` for disposable tracer output.
- `backlog/active/CUTSCENE_HORPLUS_IMPLEMENTATION_TASK_PLAN.md` only after a confirmed design decision changes implementation scope.
- `backlog/TASKLOG.md` remains deferred until the scoped diagnostic task and any manual validation are complete.

## Batches

### Batch 1 — Define the diagnostic contract

- Confirm the current 2.0.3 executable identity and the validated FOV/A/B addresses or dynamic signatures.
- Define the minimum event stream with one sequence counter: separate A-hit and B-hit counters/last-sequence values, cinematic-global writer hits, FOV-entry hits, source FOV, converted FOV, live aspect, cinematic global and F7/F8 event markers.
- Confirm safe read boundaries and that the tracer is read-only with respect to game state.

Batch 1 result:

- The tracer now resolves and observes A and B setter sites independently, with per-site hit counters and last-sequence values.
- FOV-entry and global-writer records include the current A/B counters and last-sequence values for direct ordering comparison.
- The tracer callbacks only observe context/state and do not change game memory, registers or control flow.
- The diagnostic experimental build includes the same event stream without requiring a second ASI on the shared hook sites.

Validation: static review of the tracer scope and executable identity. No game behavior claim.

### Batch 2 — Targeted runtime correlation

- Run the tracer through a new-game opening cinematic at `5120x1440`.
- Capture cinematic-active, cutscene-ended and post-transition samples.
- Use F7 at the beginning of the cinematic. Press F8 several seconds before the visible end to arm a bounded 10-second observation window; treat it as an observation-start marker, not an exact cutscene-end timestamp.
- Determine whether separate A/B activity, cinematic-global writer activity, FOV source semantics or another observable state marks the natural policy boundary. Compare the last A/B and global-writer sequences with FOV-entry hits throughout the pre-end/blend window and the ten seconds following F8.
- If useful, compare one vanilla 16:9 run against one experimental 32:9 run.

Current test protocol: the diagnostic build starts bounded event logging when it installs, so early cinematic setup cannot be missed. Press F7 at the beginning of the cinematic for correlation. Press F8 several seconds before the visible end; it opens a fixed 10-second observation window after F8. Do not press F8 as an exact end marker.

Validation: user-supplied runtime logs with reproduction steps and marker interpretation. Report confirmed facts separately from hypotheses.

### Batch 3 — Architecture gate

- If a natural semantic gate is established, update the implementation plan with that contract before editing the module.
- If no natural gate is established, stop and report the blocker; do not introduce timing workarounds.
- Keep implementation and manual module testing as a separate approved task/batch.

Validation: plan/diff review only; no implementation claim from diagnostic output alone.

## Risks and safe failure

- Hooking the diagnostic point must not alter registers, flags, control flow or game-owned state.
- Invalid or unavailable diagnostic context must be logged as unavailable and skipped, not dereferenced speculatively.
- Excessive logging can affect timing; keep logging bounded and focused on the listed fields.
- If the tracer cannot be installed safely or produces ambiguous correlation, remove/disable the tracer and report unresolved ownership.

## Stop conditions and phase gates

- Stop if the only proposed fix is a frame count, timer, delayed replay or hard-coded multiplier.
- Stop if the trace cannot distinguish cinematic-active from transition state using reproducible evidence.
- Stop if the candidate state is only a changed value without evidence of semantic ownership or shared downstream effect.
- Do not reopen the accepted A/B contract without contradictory evidence.

## Expected final Git review

After the bounded diagnostic task and any user-run validation, inspect Git status, relevant diff and recent commit; compare changed paths with this plan and classify completed, remaining, deferred, blocked and not-runtime-validated items. Update `backlog/TASKLOG.md` only after that comparison and the user’s manual validation are complete.
