# Runtime.cpp Decomposition — Final Architecture Checkpoint

Date: 2026-10-10  
Status: **B1–B6 complete with their individual approved outcomes; B7 not approved**  
Scope: final documentary checkpoint; no production changes, tests, builds, or game launch

## Outcome

The B1–B6 decomposition campaign is complete. The goal was to separate
independent responsibilities without weakening the single runtime owner or
changing camera behavior—not to minimize `runtime.cpp` line count. The
remaining file is primarily the native composition/orchestration boundary
that binds callbacks, owner state, synchronization, memory operations,
publication, and subsystem handoffs.

## What is separated

- **B1 — owner access boundary:** a plugin-private typed binding reaches the
  same process-resident `RuntimeState`; it did not create another owner or
  correction gate.
- **B2 — settings support:** hotkey mapping and persistence were moved to
  `runtime_settings.cpp`. Four state-dependent settings/readback operations
  remain with the owner.
- **B3 — shared-state domain helpers:** pure snapshot/publication/freshness
  helpers and performance event counters are separated. Cross-owner
  publication, freshness adapters, semantic snapshot assembly, viewport
  facade, and deferred logging remain in the integration owner.
- **B4 — Dialogue domain logic:** Dialogue state-machine/transaction helpers
  are separated. Native callbacks, owner-bound views, reset wrappers, and
  callback/install glue remain.
- **B5 — Cinematic domain logic:** selection, aspect policy, lifecycle
  transactions, recovery/continuation decisions, and trace models are
  separated. Native ENTER/EXIT/aspect callbacks, source/generation wiring,
  recovery handoff, publication/wake, and installer orchestration remain.
- **B6 — Gameplay writer domain/transaction logic:** writer routing, mode
  policy, HorPlus and AspectRecalculation decisions, RuntimeCombine
  synchronization/readback gating, and recovery/source policies are already
  implemented in domain/transaction helpers. The native writer callback,
  RuntimeState-bound views, engine memory/viewport operations, and its
  publication/diagnostic call ordering remain in `runtime.cpp`.

Detailed, per-batch decisions are recorded in the
[implementation plan](../../completed/RUNTIME_CPP_DECOMPOSITION_IMPLEMENTATION_PLAN_2026-10-08.md),
[move ledger](MOVE_LEDGER_2026-10-08.md), and B5/B6 boundary reports.

## Owner boundaries intentionally preserved

- One process-resident `RuntimeState` and one `CameraCorrectionGate`.
- Native callback ABI, register/store sequencing, and hook context stay beside
  the callbacks that own them.
- `CameraCorrectionGate → dialogueMutex` ordering and deferred logging after
  gate release remain unchanged.
- B3 publication/freshness adapters and B5 Cinematic recovery are consumed by
  the Gameplay writer through the existing owner; no duplicate state,
  coordinator, publication path, or synchronization API was added.
- Production installers, startup/reset/worker orchestration, and hook
  lifetime remain centralized. B7 was not reviewed, so this checkpoint does
  not classify its symbols as `Remain` or `Already extracted`.

The approved B2–B6 `Remain` decisions are not unfinished extraction work.
They record cases where moving runtime glue would only relocate complexity,
broaden mutable owner access, split a callback lifecycle, or add cross-TU
coupling without a demonstrated architecture benefit.

## Independent validation gates — still open

- **N03: `Partially mitigated`.** Do not report complete startup failure-path
  coverage.
- **N04–N06: open.** These remain separate native runtime evidence gates and
  are not closed by structural decomposition.
- **Final-artifact live validation: pending.** The supplied live-smoke logs
  loaded ASI SHA-256
  `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771`; the
  final N01–N03 remediation ASI is
  `E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81`.
  The former does not validate the latter. A live test using the exact final
  artifact remains pending.
- **B7:** review and implementation are not approved. Revisit only if a
  specific maintenance need identifies a cohesive, independently valuable
  boundary; do not resume decomposition by default.

No production/vendor source, tests, build scripts, release artifacts, or
configuration were changed for this checkpoint. No tests, builds, or game
runtime were run.
