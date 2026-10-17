# Runtime.cpp Decomposition — Final Architecture Checkpoint B1–B7

Date: 2026-10-10  
Status: `RUNTIME_CPP_DECOMPOSITION` complete; B1–B7 closed according to their individually approved dispositions  
Scope: architecture synthesis only; no implementation or validation reruns

## Executive summary

The decomposition has separated reusable policy, state-machine transactions,
and persistence from native runtime composition while preserving one
process-resident `RuntimeState`, one `CameraCorrectionGate`, and centralized
callback/startup ownership. The remaining `runtime.cpp` is principally the
composition root: it binds game callback context, owner-held state, locks,
native memory operations, hook objects, publication, diagnostics, and
cross-domain lifecycle ordering.

B1–B6 remain complete according to their separately approved results. The
user approved B7 closure on 2026-10-10 as **Complete — with documented
`Remain` decisions**. The full decomposition campaign is now complete at
B1–B7, without another source split. This architectural closure does not
imply full runtime verification or close the separate N03–N06 gates.

## What was actually separated

- **B1 — owner binding:** plugin-private cross-translation-unit access resolves
  to the existing resident owner; no second owner or gate was introduced.
- **B2 — settings support:** hotkey mapping and persistence moved to
  `runtime_settings.cpp`. State-dependent application/readback operations
  remain with the runtime owner under the existing lock order.
- **B3 — shared state helpers:** pure snapshot/publication/freshness operations
  and performance-event counters are separate. Atomic multi-owner snapshot
  assembly, publication/freshness adapters, viewport facade, wake, and
  deferred logging remain runtime integration.
- **B4 — Dialogue:** Dialogue state-machine and transaction helpers are
  separated; native callback, owner-bound state views, reset wrappers, and
  install glue remain.
- **B5 — Cinematics:** selection, aspect/lifecycle transactions, recovery and
  continuation decisions, and trace models are separated; native
  ENTER/EXIT/aspect callbacks, source/generation wiring, recovery handoff,
  publication/wake, and transactional installation remain.
- **B6 — Gameplay writer:** writer routing, mode policy, HorPlus and
  AspectRecalculation decisions, RuntimeCombine synchronization/readback
  gating, and recovery/source policies are already in domain/transaction
  helpers; native callback, owner-bound views and engine operations remain.
- **B7 — existing separations:** `DllMain` already lives in `dll_entry.cpp`;
  worker creation/stop/join mechanics already live in `WorkerLifecycle`;
  resolver/transaction helpers and optional Overlay-start decision helpers
  are already separate. The runtime-specific startup, install, worker-loop,
  and reset orchestration remains with the runtime owner.

Per-batch decisions are in the
[implementation plan](../../completed/RUNTIME_CPP_DECOMPOSITION_IMPLEMENTATION_PLAN_2026-10-08.md),
[move ledger](MOVE_LEDGER_2026-10-08.md), and B5–B7 boundary reports.

## B7 ownership decisions recommended

Keep `InitializeThread`/`Initialize`, readiness and failure-cleanup wrappers,
`ResetAllRuntimeResources`, public shutdown/detach facades, owner-bound hook
install/commit/rollback functions, runtime worker facades and subsystem loops,
hotkey orchestration, native exception wrappers, and plugin-to-Overlay
readiness handshake with the runtime owner. Their ordering crosses the same
`RuntimeState`, HookSet, capability flags and gates or composes B1–B6
boundaries. Moving them would require exporting mutable owner state, changing
lock/thread contracts, or duplicating transaction order without removing the
underlying coordination.

Keep `WorkerLifecycle`, DLL entry, pure hook resolvers/transaction policy,
callback-independent mode and hotkey predicates, and Overlay's own presenter
lifecycle in their existing components. These are already meaningful
boundaries; do not add another manager or wrapper for symmetry.

The reviewed source also shows an N03-sensitive path where recoverable
initialization cleanup, runtime completion state, and later Overlay-ready
notification are separate decisions. This checkpoint treats their combined
failure semantics as **not established by offline source/tests alone**, not
as a proven defect. It remains a game-specific validation question under N03
and the exact-artifact gate.

## Contracts retained

- One resident `RuntimeState`; one correction gate; no duplicated coordinator,
  HookSet, publication path, or mutable state API.
- Existing native ABI/register and original instruction continuation
  contracts remain beside callback adapters.
- Existing correction-gate/mutex order, startup/install commit order, worker
  join-before-hook-reset order, retained-resource failure behavior, and
  loader-lock restrictions remain unchanged.
- Optional Overlay failure/readiness remains distinct from camera-correction
  ownership; Overlay owns its own discovery/presenter resources.
- B1–B6 approved outcomes remain final; this checkpoint does not revise them.
- B7 is closed with the classifications in the B7 review report and move
  ledger; no additional source split is authorized by this campaign.

## Remaining verification gates

- **N03 — `Partially mitigated`:** the outer actual `InitializeThread` entry is
  exception-contained and guarded cleanup is tested, but the full
  game-specific Initialize/signature-scan/partial-hook-install failure path
  is not verified.
- **N04 — open:** production startup/install/partial rollback orchestration.
- **N05 — open:** arbitrary game callback quiescence and patch-removal races.
- **N06 — open:** native replay provenance for current game/mod/build identity.
- **Final remediation artifact live validation — pending:** previous live
  smoke loaded ASI SHA-256
  `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771`; final
  remediation ASI is
  `E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81`.
  Evidence from the former does not validate the latter.

N01 and N02 remain `Fixed` within their recorded offline/tested bounds. Their
status and all historical test results were not changed by this review.
Neither B7 review nor architectural closure satisfies the open runtime gates.

## Closure decision

The user approved B7 as **Complete — with documented `Remain` decisions** and
approved ending `RUNTIME_CPP_DECOMPOSITION` at B1–B7. No further source splits
are planned or authorized under this campaign. A future, concrete maintenance
need may be evaluated separately; it does not reopen this completed work.

The independent evidence gates remain as recorded above: N03 is `Partially
mitigated`; N04–N06 are open; and live validation of the exact final
remediation ASI remains pending. The prior live-smoke SHA mismatch is not
resolved by this architecture closure.

This closure changed documentation only. No production/vendor code, tests,
build scripts, configuration, or release artifacts were changed. No tests,
builds, ASI installation, or game launch were performed.
