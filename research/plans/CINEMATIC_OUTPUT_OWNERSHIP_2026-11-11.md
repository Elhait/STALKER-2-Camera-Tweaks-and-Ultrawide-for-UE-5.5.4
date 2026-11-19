# Cinematic source/output ownership correction

Date: 2026-11-11. Scope authorized by the project owner: correct the weapon-switch cinematic regression while preserving per-invocation admission and Gameplay recovery. No Git operations, game launch, debugger, graphics fixtures or automatic installation.

## Evidence and current status

The bounded native-provenance investigation and production implementation are complete; see [native provenance and EXIT freshness](../reports/CINEMATIC_NATIVE_OUTPUT_PROVENANCE_2026-11-11.md). Focused CPU-only regression tests pass on the final rebuilt harnesses. The canonical non-graphics suite builds the changed targets but stops at an unrelated configuration hardlink-fixture setup error. Manual in-game acceptance is **not complete**, so the visual regression is not yet runtime-proven fixed.

- Supplied runtime log: generation 1 initially transforms 90.3115 to 127.119, then reports a same-RSI/different-RBX conflict at 03:00:58.173. Returning to the original RBX at 03:00:58.202 does not restore correction. Recorded AFTER samples: 6 applied / 33 skipped. EXIT captures no recovery owner.
- Generation 2 records 569 applied AFTER samples and captures a recovery owner. These counts describe emitted records, not all frames.
- `src/cinematics/cinematic_fov.cpp`, ObserveWriter: changing output permanently invalidates association until the next generation. AuthorizeInitialEstablishmentRetention rejects the poisoned association.
- `tests/regression/module_ownership_harness.cpp` explicitly expects `same_rsi_with_conflicting_rbx_fails_closed`. This expectation must be refined, not merely deleted.
- `src/plugin/runtime.cpp`, ReplayManualTransition, passes source/output addresses, input FOV and source aspect/flags into CameraWriterInput. There is no independently established output ownership fact in that input.
- `src/gameplay/gameplay_camera.cpp` validates the native instruction `MOVSS [RBX+0x30], XMM0`. Historical source/output-copy evidence is in `research/reports/CINEMATIC_AXIS_CONSTRAINT_RUNTIME_REFLECTION_TRACK_B.md`; it does not establish a reflected output type or distinguish a semantically foreign destination with otherwise valid memory.
- The Gameplay-baseline continuation branch in `src/plugin/camera_writer_transaction.hpp` also checks association RBX. It must participate in the same correction; changing ObserveWriter alone is insufficient.
- Recovery Capture currently copies observedWriterRbx from association. The last observed callback may belong to a different source. Latest observation and latest admitted output must remain distinct.

## Implementation progress — 2026-11-11

The association retains confirmed source ownership across destination transitions; a writer destination is recorded only by a generation/source/current-sequence admission while CinematicActive and correction is enabled. Initial establishment and continuation consume that same invocation evidence. EXIT captures the last admitted output and sequence as historical metadata; Gameplay recovery still uses the current invocation and source ownership. A conflicting aspect target and a new generation clear admitted evidence. Focused cases cover A→B→A, B persisting through EXIT, rejected foreign source, zero output, historical capture after rejection, and recovery with a different current destination.

Updated `docs/ARCHITECTURE.md` and `docs/SAFETY_INVARIANTS.md` to state the contract. The canonical registry audit passed: 114 native-adapter scenarios and 228 child invocations. Final rebuilt harnesses passed all 14 focused invocations, including diagnostics OFF/ON for four production-adapter scenarios. `test.cmd -NonGraphics` built the suite, but the full runner stopped at `config_persistence_harness` with `hardlink_fixture_setup_error=5` / `staging_name_ownership_and_hardlinks=FAIL`; this prevents claiming the full canonical suite passed. Process-tree cleanup was verified by the test runner. No game, debugger, WARP, graphics fixture, Release ASI build, or installation was performed. Manual in-game acceptance remains required.

## Required contract

1. Generation ownership comes from the accepted cinematic lifecycle/settings generation and confirmed aspect-store source. Numeric similarity never creates ownership.
2. Source association is not permanently revoked solely because a legitimate output destination changes.
3. Output identity is invocation-local. The validated complete source-to-output native copy boundary, accepted generation and confirmed cinematic source together provide admission provenance. A stable RBX is not required. Nonzero/readable/writable addresses, matching FOV and unchanged RSI alone are not semantic ownership proofs. No unobservable foreign-output discriminator may be invented.
4. Rejecting a foreign/invalid output skips that invocation and cannot overwrite the last admitted output or revive/extend establishment. A conflicting aspect-store target remains a real ownership conflict.
5. EXIT may capture only a generation-matching confirmed source with matching admitted evidence. The stored output is the last admitted destination, explicitly historical metadata, not proof of a currently active output. An unrelated latest observation or EXIT's own RBX cannot supply or update it. No output pointers are dereferenced after their invocation merely because they were cached. Recovery authority remains source/generation-bound and uses each new native writer invocation.
6. Initial establishment, Gameplay-baseline continuation, EXIT capture and post-EXIT recovery must agree on these boundaries. Existing authored-departure and interpolation rules remain intact.

## Admission prerequisite

Resolved for the bounded correction scope by the linked research report: the native copy invocation selects the destination for this call. No further semantic discriminator is established. The report does not claim that none exists elsewhere or that the hook is the game's only FOV writer. Two outputs with identical observable native provenance cannot be distinguished as legitimate versus semantically foreign; tests and production admission must acknowledge that limit.

Freshness is lifecycle/sequence-based historical evidence, not elapsed time or proof of continued allocation. The last admitted output must match the still-valid generation/source at EXIT. Rejected observations do not replace it; a real ownership conflict invalidates source authority. Recovery never writes through that cached output.

## Implementation sequence

1. Native provenance/freshness investigation complete as a bounded engineering proposal; integrate its explicit limits into implementation and tests.
2. Update source association and per-call output admission in cinematic_fov and the production writer adapter/transaction. Preserve diagnostic observation separately from last admitted output.
3. Make continuation and EXIT/recovery consume the same admitted generation/source/output evidence. Update relevant architecture/safety documentation to supersede pair immutability.
4. Refine the old ownership test and add actual-adapter regressions. Review the complete scoped diff before building.
5. **Complete offline validation:** final rebuilt focused CPU-only tests passed (14/14). Canonical `test.cmd -NonGraphics` build completed, but its full test run is blocked at the configuration hardlink-fixture setup error above. No graphics fixtures were run; runner process-tree cleanup was verified.

Expected files: cinematic_fov.hpp/.cpp; camera_writer_transaction.hpp; runtime.cpp only where native admission is established; module_ownership_harness.cpp; runtime_native_adapter_harness.cpp and the lifecycle/recovery harnesses; relevant test registration and current ownership contracts if required.

## Required regression matrix

| Scenario | Required outcome |
| --- | --- |
| Confirmed S/A -> S/B -> S/A | Both admitted destinations receive expected FOV; generation/source association survives. |
| Confirmed S/A -> S/B, B stays | No dependence on returning to A; EXIT captures B according to the freshness rule. |
| Foreign source or missing/invalid invocation admission for B | Native pass-through; cannot replace admitted evidence or gain correction from numeric resemblance. A semantically foreign B indistinguishable at the validated native boundary is not falsely claimed rejectable. |
| Source changes during A/B switching | No mutation for the unassociated source; no revival of retired authority. |
| EXIT while admitted B is active | Snapshot has correct generation, S and B; Gameplay recovery and completion still work. |
| EXIT after rejected B | No recovery ownership transfer to B; A may remain last-admitted historical metadata if source/generation ownership remains valid. No cached destination is dereferenced. |
| New generation, stale output/source evidence | Old evidence cannot authorize correction or EXIT capture. |
| Conflicting aspect target, disabled correction, authored departure | Existing fail-closed and native interpolation behavior preserved. |
| Diagnostics OFF/ON | Identical production decisions and output values. |

## Acceptance and non-goals

Acceptance requires the actual FOV/register output and recovery owner assertions, not only flags. Those assertions pass in the focused harness. Offline results cannot close the original visual regression: one manual replay with other weapon/pistol conditions remains necessary after implementation.

No camera-formula changes, timers, weapon-name detection, SafetyHook changes, Overlay changes, new hooks or binary replacement to mask the issue. No release build or installation is claimed by this plan. Preserve the supplied log and existing ASI.
