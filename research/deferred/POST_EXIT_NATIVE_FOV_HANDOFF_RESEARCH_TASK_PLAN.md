# Task Plan — Post-EXIT Native FOV Handoff Research

## Objective

Find the native transition setup that generates the post-cinematic FOV
interpolation stream, so a future improvement can make the cinematic/gameplay
handoff effectively immediate without replacing the released cinematic fix.

## Established evidence and current state

- The released ASI path remains the control/fallback implementation.
- On the current Steam 2.0.5 executable, cinematic EXIT reports native target
  FOV 90, followed by a multi-sample residual stream around `126.8 → 90`.
- A downstream 500 ms guard repeatedly clamped samples, did not stop the
  upstream interpolation and contaminated dialogue detection; it is rejected.
- The current Auto aspect correction is separate and must remain intact.

## Current status

- Combined atomic cinematic/gameplay handoff remains the validated behavioral
  candidate and was not modified by this research.
- WinDbg confirmed the physical live-field setter at RVA `0x205FCC8`:
  `MOVSS [RCX+0x230], XMM1; RET`.
- Runtime confirmed the same live camera source across gameplay, cinematic
  ENTER (`~126.870`), EXIT recovery (`~126.831`) and final recovery (`90`).
- The attempted native-caller capture did not recover a trustworthy caller:
  `[RSP]` points to a private hook/trampoline region, while the inspected
  secondary stack address belongs to unrelated AK/plugin-list code.
- Native interpolation producer, transition state, alpha, duration and
  immediate-completion mechanism remain unresolved.
- Native FOV bypass status: `DEFERRED` pending a new independent anchor.

## Approved scope

- Static/runtime evidence work anchored to the validated cinematic EXIT boundary
  and known camera writer.
- Identify transition target/start/alpha/duration/speed ownership if concrete
  evidence exists.
- Keep the released FOV/aspect/dialogue behavior as the safe fallback.
- Implement a production change only after a concrete upstream anchor and a
  bounded runtime feasibility result are established.

## Explicit non-goals

- No timer-based post-EXIT writes.
- No frame counters or repeated writer-level FOV clamps.
- No changes to gameplay aspect replay, Auto aspect resolution or dialogue.
- No broad renderer/camera subsystem scan without a new anchor.
- No replacement of the released path before runtime validation.
- No installation, deletion, commit, tag or publication.

## Files or areas expected to be touched

- Research reports and bounded diagnostic source under `research/` or `src/`
  only if a concrete probe is justified.
- `src/experimental_cinematic_21_9_combined_fix_204.cpp` only for a validated
  production improvement; otherwise it remains untouched.
- `research/reports/POST_EXIT_NATIVE_FOV_HANDOFF_RESEARCH.md`.

## Batches

### Batch 1 — Upstream ownership audit

Use the current executable identity and validated EXIT anchor to inspect the
immediate transition setup and its arguments. Record only concrete evidence for
target/start/interpolation state or duration ownership.

### Batch 2 — Bounded feasibility probe

Only if Batch 1 yields a concrete safe anchor, create a separate diagnostic
probe. It must compare the native handoff with the released fallback and must
not alter production behavior by default.

### Batch 3 — Production promotion gate

Promote a change only if it preserves the old path as fail-safe and runtime
evidence shows a better handoff without gameplay/dialogue regression.

## Validation

- Every static conclusion is tied to the current executable SHA-256 and known
  runtime anchor.
- No new runtime probe is accepted without a concrete owner/callsite.
- Any production build is checked separately from diagnostic artifacts.
- User runtime evidence is required before claiming the handoff is improved.

## Risks and rollback / safe failure

- An incorrect upstream hook could affect all camera FOV updates. Require exact
  instruction/argument validation and keep native pass-through on failure.
- If no concrete owner is found, close/defer the research batch without source
  changes. The released implementation remains available.
- Rollback is limited to the bounded source diff; no existing release files are
  deleted or overwritten.

## Stop conditions and phase gates

- Stop after the immediate bounded audit if no transition owner is recoverable.
- Stop before runtime writes if target/start/duration semantics are uncertain.
- Stop promotion if the new path changes gameplay or dialogue behavior.
- Close/defer the current bypass branch after the bounded debugger capture when
  no trustworthy native caller or transition-state anchor is recovered.

## Expected final Git review

Inspect status, relevant diff/statistics, changed paths and recent commits.
Report completed, remaining, deferred, blocked and not-runtime-validated
items. Do not commit or publish.
