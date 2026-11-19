# M1.6 isolated composite Writer

2026-11-17. Implementation and focused validation complete, pending user review.
M2/M3 remain NO-GO. Runtime callbacks, R1, ASI and game were not changed.

## Red evidence

Before core modification, the existing isolated interface failed the new
single-Writer-commit and 300-unobserved-corrections expectations. Exact output
and verified cleanup are in `../evidence/lifecycle-authority-m16-2026-11-17/red.json`.
M1.5 had separately reproduced zero diagnostic transaction identity. The new
composite tests require invocation ID 1 in both observation modes, independently
of diagnostic frame identity (0 versus 500).

## Composite relation

`Accept(WriterEvent)` stages source replacement, source-bound termination,
recovery completion, bounded provider confirmation, Gameplay admission, baseline
trust and correction selection in a private sequential working relation. Reused
low-level reducers can commit only to this private local state; those revisions
are never externally published. A successful Writer publishes one final state
revision and decision. A rejected binding or proposal exception cannot publish
a partial working state. Unexpected mutation through a supposedly pure policy
callback is detected rather than overwriting the newer committed state.

The numeric proposal callback receives immutable owner, selected policy,
reference, incoming FOV and provenance. It returns only a numerical proposal and
reference dependency. It receives no mutable lifecycle view; its purity remains
a caller contract. Production numeric helpers are exercised by test proposals.
This interface does not claim complete production policy integration.

PID7372 source-owned completion and Gameplay admission now occur in that single
composite Writer revision. Receiver replacement invalidates A; B can receive
PolicyAdmitted only from B's original sample and accepted source-bound inputs.
No inherited A numeric reference or GameplayEligible verdict is used as proof.
Prepared inputs cannot update the original Gameplay reference.

## Execution and observation

`WriterInvocation` is move-only and local to the caller's PRE invocation.
Authority is noncopyable/nonmovable and must outlive its invocation receipts.
`PrepareWriterArgument` consumes the private permission once, checks the actual
Authority/revision/invocation, and prepares a numeric argument. Public receipt
metadata cannot resurrect a stale permission. Destruction abandons unused
permission; there is no map of long-lived execution permissions.

ArgumentPrepared is distinct from Applied. Native MOVSS still occurs after PRE.
Only explicitly supplied named readback can produce an Applied observation.
An aborted unprepared invocation releases its optional observation registration
without manufacturing a result or adding another lifecycle commit.

Accepted Writer invocation IDs come from an Authority counter independent of
the optional diagnosticFrameId. IDs identify invocations, not engine ownership,
allocation lifetime, episodes or store completion. Binding those IDs to actual
native callbacks remains the next adapter task.

Composite Writer uses a separate bounded observations map. At 128 pending
windows, new tracking becomes Unavailable while execution continues. Existing
pending records are not evicted. Delayed readback is retained historically and
cannot resurrect a retired source or replace another invocation's current
output. Every successor Writer clears current readback facts while preserving
the separate pending history. Explicit correlated closure releases tracking
capacity; losing all readbacks cannot exhaust execution permission storage.

The old low-level PrepareEffect path remains solely for existing M1
characterization/counterexample tests, with its historical capacity behavior.
It is not the new composite execution API and must not be used by a future
Writer adapter. Existing ENTER/EXIT/Setter/settings test events remain separate.

## Focused validation

- Sequential supervised MSVC/Ninja build of existing M1 harness: PASS.
- Default harness: 95 assertions PASS, including 29 new M1.6 assertions.
- Shadow invocation: 31 assertions PASS.
- Separate composite invocation: 29 assertions PASS (same new cases as default).
- 600 corrections per observation mode: identical expected FOV, independent
  invocation identity and uninterrupted execution. ON retains at most 128 windows;
  472 additional tracking requests are unavailable. OFF retains zero windows.
- Tested: single commit, same-event recovery admission, capacity/late readback,
  explicit closure, aborted PRE, single-use/moved/stale capability, settings
  supersession, invalid generation, proposal exception and unexpected reentry.
- Supported fixture sequence: Gameplay 153.931 -> Cinematic 153.931 -> bounded
  provider sample 153.925 -> Dialogue 149.071 -> Cinematic 153.931. Prepared
  Dialogue continuation is not mapped twice. Production numeric functions are
  used; native fact flags remain fixture premises, not discovered bindings.
- Registry audit PASS: 60 targets / 63 invocations / 0 missing inputs.
- Every owned build/test process tree: cleanup verified by the existing supervisor.
- Frozen R1 identities unchanged; runtime/R1 hashes recorded in evidence.
- Full suite, CTest, live callbacks, native memory execution, graphics, game and
  release build were not run.

Task-relevant core and harness changes were reviewed; prior workspace changes
were preserved. Remaining work is full differential parity of concrete
production Writer/Setter/ENTER/EXIT/settings transactions, original/prepared and
terminal evidence binding, actual output/thread readback correlation, executor
scope lifetime and shutdown integration. This report does not authorize M2.
