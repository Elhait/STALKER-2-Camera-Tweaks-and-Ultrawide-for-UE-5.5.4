# Post-EXIT blend activation observation

Date: 2026-11-12. Status: implementation and offline validation complete;
manual runtime evidence pending. Diagnostic-only investigation. This plan
supersedes the earlier “existing hooks only” restriction **only** for one
structurally resolved observation point after native start-FOV capture. It does
not authorize production camera changes or additional native
observation points.

## Objective

Determine whether a native start-capture event can be joined to a later
active tick/setter-boundary observation and current writer sample, including
re-arming of the same component and overlapping blend contexts on one camera.
The result is evidence for choosing a future recovery contract; this phase
does not select A/B math or implement recovery.

## Authority and boundaries

- New code exists only under `POST_EXIT_BLEND_DIAGNOSTIC`; production source
  behavior and production artifact remain unchanged.
- Add exactly one new native observation point, resolved from a unique
  executable-section signature and validated instruction/operand structure
  after the start capture (`MOVSS [RSI+0x58],XMM0`) and before subsequent
  activation/query branches. This point records a candidate epoch; it does not
  alone prove the initializer completed successfully. A matching subsequent
  tick is required to establish that the captured state became active. The RVA
  from the current EXE is evidence only, never a gate.
- The callback is read-only: it does not change registers, FOV, native fields,
  invocation order, correction admission or lifecycle state.
- Reuse the existing EXIT, blend tick/setter-boundary and writer observation
  points. A pre-call setter observation is not described as setter completion
  or direct post-setter readback.
- No global ownership manager, timer, polling, new thread, production
  capability, recovery/math edit, or automatic game/debugger/graphics run.
- If exact activation-to-writer causality requires another native hook, stop
  at that evidence boundary and report the additional permission needed.

## Required behavior and safety rules

1. Each observed start capture gets a session-unique candidate epoch, including
   when the same component pointer is re-armed. A candidate is promoted only
   to “tick observed” after a later callback matches its component, receiver,
   start and target.
2. Capture component and receiver identities, start and target values, thread,
   bounded caller evidence, and whether the caller return address matches a
   currently validated cinematic ENTER/EXIT callsite. Unknown caller remains
   unknown; it does not inherit EXIT attribution from target equality or time.
3. Derive the caller return-address location from the inspected native frame
   only after an actual adapter test validates the SafetyHook `rsp` contract,
   stack arithmetic, readable range and known-callsite classification. Never
   infer native PC from `context.rip` or mutate `rsp`/`trampoline_rsp`.
4. Carry candidate epoch through existing blend callback and writer records
   only on component+receiver match and compatible captured start/target.
   Re-arm replaces the current epoch. Pointer reuse, unknown caller, field
   mismatch, episode/generation retirement, failed resolution or capacity
   exhaustion invalidates or leaves attribution unknown; none may revive stale
   epoch evidence.
5. Multiple components on one receiver remain independently observable.
   Do not select a single “owner” from recency alone or let diagnostic
   observation authorize production correction.
6. Keep storage and records bounded by session-wide limits. With Diagnostics
   OFF, the new observer performs no added stack read, memory read, sequence
   update, formatting or logging.
7. Existing pass-through register values and ordinary runtime decisions remain
   identical. Resolver failure disables only this diagnostic capability.

## Batches

### Batch 1 — resolver and stack-evidence proof

- Add a reusable structural resolver for the successful-start instruction
  sequence: unique `.text` match, executable bounds, decoded instruction
  lengths, exact register/base/displacement operands, and bounded neighboring
  instruction checks. No SHA/version/RVA selection.
- Add a pure caller-evidence reader/model with overflow-safe stack offset and
  explicit unknown dispositions.
- Extend the actual native adapter with controlled stack images covering the
  expected prologue frame, readable and unreadable stack, overflow, unrelated
  return addresses, ENTER/EXIT return-address matches, and malformed context.
- Compile/run focused CPU-only resolver and adapter tests; no game/runtime
  session in this batch.

### Batch 2 — fixed-storage activation lineage observer

- Extend diagnostic-only bounded storage with candidate-epoch assignment, re-arm,
  episode/generation retirement, receiver/component matching, overflow and
  pointer-reuse dispositions.
- Integrate the one new successful-start hook plus existing tick/setter and
  writer observations. Keep entry values, callback output and writer values
  explicitly distinct. Do not call native helpers from the observer.
- Add deterministic scenarios for same-object re-arm, A→B→A interleaving,
  persistent B, unrelated receiver, source/generation changes, EXIT while B is
  active, cleanup/retirement, and diagnostics OFF.

### Batch 3 — review and diagnostic candidate

- Run focused tests and the canonical non-graphics test entry point if its
  current registry remains within the approved CPU-only scope.
- Build production and diagnostic Release artifacts separately; verify the
  production binary is unchanged in code and contains no new observer marker.
- Review the complete diff and candidate PE, compute SHA-256, and preserve all
  prior ASIs. Do not install the candidate or upload it to VirusTotal.
- Produce a concise runtime protocol explaining what the one manual run can
  establish and what still requires a second hook. The user decides whether to
  install/run it.

## Acceptance criteria

- Resolver accepts only one structurally valid site within `.text`; zero,
  duplicate, truncated or wrong-operand matches fail closed.
- Candidate epoch changes on every captured re-arm even at identical
  addresses; no stale epoch survives explicit retirement or generation change.
- A tick/writer record receives an epoch only from matching activation
  evidence, never from pointer/target/numeric equality alone.
- Interleaved contexts are independently represented and bounded.
- Diagnostics OFF adds no observation work; hook failure does not affect
  Dialogue, Gameplay, Cinematics or other production capabilities.
- No claim of setter completion, unique native ownership, final camera state or
  rendered pixels is made without a corresponding observation point.

## Expected files

- `src/hooks/signatures/signature_definitions.hpp` and a focused resolver
  implementation/header if needed.
- `src/plugin/runtime.cpp` and diagnostic observation model under
  `src/diagnostics/`.
- Focused resolver/adapter/observer harness cases and canonical registry
  wiring.
- This plan and a dated result report. No production architecture, recovery
  transaction or user documentation changes.

## Validation / preservation

Sequential execution only, finite supervisor timeout and verified child
cleanup. The previous diagnostic evidence and all ASIs remain immutable.
Static/CPU-only checks and build outputs do not establish the game runtime
lineage. Any remaining causality gap is reported as an evidence limit, not
closed by inference.
