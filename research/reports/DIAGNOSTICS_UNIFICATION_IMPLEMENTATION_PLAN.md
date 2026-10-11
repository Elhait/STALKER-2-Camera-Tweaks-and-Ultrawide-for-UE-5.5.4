# Diagnostics Unification — Implementation Plan

Date: 2026-10-11

Status: **Complete — approved implementation delivered 2026-10-11.** Actual changes and validation are recorded in [implementation report](DIAGNOSTICS_UNIFICATION_IMPLEMENTATION_REPORT.md). The audit/plan text below remains the approved design baseline; the report is the source of truth for delivered outcomes and measured results.

Inputs: [audit](DIAGNOSTICS_UNIFICATION_AUDIT.md), [gate matrix](DIAGNOSTICS_UNIFICATION_GATE_MATRIX.md), current `docs/ARCHITECTURE.md` and `docs/SAFETY_INVARIANTS.md`.

## Contract and scope

One ordinary `STALKER2CameraTweaks.asi` contains all approved passive runtime diagnostics. Existing `[Diagnostics] Enabled` controls diagnostic-only work with the current startup-only application model. Missing key remains false; invalid-value parsing and persistence remain unchanged. No new INI key, diagnostics UI, hotkey, runtime settings owner, reload path or category selector is proposed.

OFF skips diagnostic-only game reads, snapshots, prediction, trace locks, dedup/counters, formatting, log closures and diagnostic I/O. It retains shared camera observations/publication, actual correction locks, native Zoom facts, startup milestones, safety/error logs and necessary operational events. A gate check and compiled code footprint remain; no literal zero-overhead promise is made.

All batches preserve native context/store continuation, output values, camera algorithms, correction/lifecycle/association ownership, generation/epoch semantics and gate/lock order. No runtime source split, new synchronization system, vendor change, release-artifact write, game launch or ASI installation belongs here. B1–B7 and N03–N06 statuses remain untouched. Build-system modernization is a separate future task.

## Candidate disposition

Approve for this plan: I12–I16 passive traces, with I10 OFF-work repair and existing I01–I11/I17–I22/I40 preservation. Classification A permits observation only; existing resource/lock implications are explicitly covered below.

Excluded pending separate decisions: I23 forensic startup tracing changes; I24 supersession/promotion; I25–I30 extra research hooks/traces; I31–I36 camera/replay experiments; I37 Dialogue discovery; I38 forensic watcher executables; I39 font/UI experiments. Existing isolated implementations remain in place. Keep startup forensic opt-in and the lightweight journal policies distinct.

## Batch 1 — Runtime gate foundations and disabled-path continuation

**Files/functions:** `src/diagnostics/diagnostic_runtime.hpp/.cpp` (reuse existing owner, avoid a second flag); `src/plugin/runtime.cpp` (`RuntimeState::cinematicContinuationTrace`, `GetCinematicContinuationTrace`, `Initialize`, `TraceCinematicEnter`, `ReplayManualTransition`, `FlushCinematicContinuationTrace`); `src/diagnostics/cinematic_continuation_trace.hpp/.cpp` only if an actual helper-contract adjustment is required; `tests/diagnostics/diagnostics_gate_harness.cpp`, `tests/cinematics/cinematic_fov_harness.cpp`, actual adapter scenarios in `tests/plugin/runtime_native_adapter_harness.cpp` / its evidence cases.

**Implementation:** retain the startup atomic switch. Initialize the existing nullable trace-owner member once after enabled configuration is known and before native callbacks are activated, outside loader lock. Default OFF creates no diagnostic continuation buffer. Gate adapter trace construction/Begin/Record/Discard/Finish when there is no enabled diagnostic storage. Preserve production continuation transactions. Keep existing helper semantics for direct false/reset calls unless separately demonstrated unnecessary; avoid changing helper tests merely to fit the adapter.

**Disabled cost:** master gate/null checks; no diagnostic buffer allocation, 512-sample reset/copy, trace mutex or trace input comparisons. Mandatory production recovery/publication remains.

**Failure policy:** nothrow allocation failure leaves this trace unavailable; at most a meaningful enabled startup diagnostic message, never a camera readiness failure or recurring allocation retry in the hook.

**Validation:** actual adapter OFF trace allocation/record/lock counters and ON bounded continuation; rapid-chain ENTER/EXIT, baseline acceptance, recovery, disable-in-flight; helper reset tests remain valid. Assert equal register/output/shared-state results and publication ordering OFF/ON. Gate harness covers lambda non-invocation; it is not by itself proof of adapter work suppression.

**Stop:** storage publication requires a new owner API, callback-time allocation, altered native startup ordering or live reconfiguration protocol. Return a design question instead of adding one.

**Rollback:** revert only this batch's owner-initialization/caller guards if validation fails; retain the original storage and helper contract. Do not alter camera transactions to compensate. Preserve pre-existing dirty changes through a scoped patch/reverse patch rather than checkout/reset.

## Batch 2 — HorPlus passive diagnostics

**Files/functions:** `src/plugin/runtime.cpp` (`HorPlusTelemetryOwner`, `ResolveHorPlusTelemetryOwner`, `TraceHorPlusFovState`, writer call site and telemetry reset in `SelectGameplayMode`); existing `src/plugin/runtime_internal.hpp` telemetry fields, only if a mechanically necessary declaration change is identified. No new fields should be needed. Actual adapter harness/evidence cases; affected Gameplay/source/publication harnesses.

**Implementation:** make I12 definitions/call sites available without `HORPLUS_FOV_STATE_DIAGNOSTIC`. Preserve early Enabled check before owner resolution, Dialogue/telemetry locks, cache updates and logging. Preserve reset ordering in mode transitions; if gating diagnostic resets is proposed, first prove they are not functional and cannot yield stale ON data under the startup-only contract. Reuse existing telemetry ownership.

**Disabled cost:** gate check at trace entry; no telemetry locks/caches/endpoint comparisons/logs. Required shared observations are unchanged.

**Validation:** ordinary compilation without the old macro emits expected owner/FOV/baseline trace with INI ON; no diagnostic marker/read/lock/counter work OFF. Cover all Gameplay modes, ADS, owner changes, invalid samples and selected mode changes. Compare actual native register and shared snapshot output OFF/ON.

**Stop:** output or coordinator state differs, lock order changes, or apparently diagnostic storage proves to be a production input. Do not reclassify a production fact into optional telemetry.

**Rollback:** restore only I12 compile conditions/call sites; keep foundation work already validated. No camera fix as part of rollback.

## Batch 3 — Dialogue counters and recovery observations

**Files/functions:** `src/plugin/runtime.cpp` (`LogDialogueDiagnosticReturn`, `LogDialogueDiagnosticSummary`, counter definitions, `ArmDialogueRecoveryEndpointTrace`, `ObserveDialogueRecoveryEndpointTrace`, `TraceDialogueBoundary`, summary in `ResetAllRuntimeResources`); `tests/plugin/runtime_native_adapter_harness.cpp` / native evidence cases; `tests/plugin/dialogue_boundary_transaction_harness.cpp` and relevant Dialogue provenance/recovery harnesses. Verify existing test filenames before edits.

**Implementation:** compile I15/I16 normally; add early runtime gates at counter/summary/arm/read helpers and around additional recovery-preparation expressions in the callback. Enable event/change-based bounded endpoint trace. Preserve the 256-sample bound, source-change invalidation, native XMM6→XMM1 semantics, mutex scope, shared source events and lifecycle arming location. Emit a summary only when enabled and meaningful data exists, without declaring cleanup success.

**Disabled cost:** checks only; no additional counters, endpoint predicate preparation, diagnostic reads/TLS snapshots or summary formatting. The native target read used by the actual Dialogue transaction remains.

**Validation:** compile formerly optional code in the ordinary adapter; invalid/NaN input, unreadable target, foreign source, Active/Exiting/Rearm, disable-in-flight and recovery. Assert native register/memory parity; prove diagnostic extra reads absent OFF, unchanged production read counts, bounded ON samples/dedup and nonempty-only summary. Extend existing coverage concretely rather than claiming the historically incomplete RSI/RBX/reset matrix is already closed.

**Stop:** new worker/hotkey needed, arbitrary memory sweep proposed, production predicates must change, or counter/reset synchronization requires a new mutex/lifecycle contract. `DIALOGUE_DISCOVERY_DIAGNOSTIC` is explicitly excluded.

**Rollback:** restore only the newly compiled counters/endpoint branches and their gates; leave actual Dialogue transaction/source provenance intact.

## Batch 4 — Cinematic writer and MatchGameplay prediction

**Files/functions:** `src/plugin/runtime.cpp` (`HorPlusCinematicWriterTraceSnapshot`, `ReadHorPlusCinematicWriterState`, snapshot-change/log helpers, `TraceHorPlusCinematicWriterBefore/After`, associated writer call sites, diagnostic `g_cinematicEnterAspect` telemetry publication/reset); `src/diagnostics/matchgameplay_prediction.hpp` only if a demonstrated gate integration requires it; `tests/diagnostics/matchgameplay_prediction_harness.cpp`, actual adapter evidence and relevant cinematic lifecycle/writer tests.

**Implementation:** compile I13/I14 always. Keep snapshot reads/prediction/log formatting behind the existing master gate and existing changed-event predicate. Preserve before/after placement relative to transaction, XMM0 mutation and native continuation. Keep source/output invalid reads contained. Prediction stays observational and cannot replace or publish a runtime correction result. `matchGameplay*` production fields and transaction updates stay unconditional; do not rename them as incidental cleanup.

**Disabled cost:** branch checks; no RSI/RBX diagnostic reads, before/after snapshot, TLS dedup or projection math. No additional hook or lock.

**Validation:** authored/Gameplay initial baseline, continuation/carry, rapid-chain generations, stale/foreign source, invalid/overflow RSI/RBX, rejected/indexed EXIT and disable/reenable. For each, compare register output and memory/store/publication sequences OFF/ON. Validate offset arithmetic before diagnostic reads; current SafeRead is not a concurrent object-lifetime or access-violation barrier. Confirm unknown prediction inputs remain unavailable and cannot create a match/correction. Do not present BEFORE/AFTER pre-native-store memory reads as proof of the subsequent engine consumer.

**Stop:** prediction touches output, source ownership, correction decision, active lifecycle or native ABI; added reads cannot be bounded safely; a new callback hook/owner interface is required.

**Rollback:** restore only I13/I14 conditional boundaries and diagnostic aspect telemetry gates; retain production baseline/lifecycle data unchanged.

## Batch 5 — Build/test flag cleanup and documentation

**Files:** `build.cmd`, `tools/build/build-diagnostic.cmd`, `test.cmd`, `tests/runner/native_runtime_evidence.ps1`, relevant flag consumers/audits if found; `docs/ARCHITECTURE.md`, `TESTING_AND_RESEARCH.md`, `README.md` only for the diagnostic availability/startup-selection description that actually changes. Preserve `test.cmd` physical CRLF and `.gitattributes`.

**Implementation:** remove plumbing for the five migrated passive compile gates only after ordinary compilation exercises them. Remove obsolete `ZOOM_TRANSITION_DIAGNOSTIC`, `CAMERA_STATE_SNAPSHOT_DIAGNOSTIC` and no-consumer present-break switch references after confirming their complete reference sets. Retain forensic timeline/journal policy, functional Overlay defines, vendor/test injection flags and excluded experiments.

Keep `build.cmd` as the production one-run entry point and its ordinary artifact name. Retain the diagnostic wrapper/profile/output override compatibility initially; both outputs have identical approved passive diagnostic capability, selected at runtime by INI. Explain any remaining forensic opt-in purpose. Do not silently retire a public wrapper or repurpose the diagnostic output filename. Do not change source lists, optimization/toolchain/link flags, incremental building, object directories or CMake in this batch.

**Disabled cost:** no runtime change beyond validated earlier batches; removing obsolete switches must not change functional hooks/resources.

**Validation:** compile ordinary/compatibility diagnostic configurations without old passive defines; verify actual compiler arguments; run runner inventory audit and immediate-failure checks. Preserve actual-adapter `runtime.cpp` inclusion once and source-response inputs. Confirm no promotion of extra forensic hooks/experiments and normal journal remains. Review all flag-reference changes, CRLF, Git attributes and artifact output paths.

**Stop:** registry modernization, compiler/profile redesign or wrapper removal becomes necessary. Return that as the separate Build System Modernization task.

**Rollback:** restore only flag plumbing/wrapper docs; earlier always-compiled passive code still runs under its runtime gates. Do not revert unrelated runner remediation or source moves.

## Batch 6 — Full regression and measured performance validation

**Files:** adapter/diagnostic harnesses and canonical `test.cmd` only where meaningful coverage must be registered; scoped implementation report. No production logger redesign or additional instrumentation by default.

**Deterministic acceptance:** use actual adapters with Diagnostics OFF/ON. Count diagnostic-only memory reads, prepared snapshots, trace lock attempts, predictions, allocations and emissions at the production leaf boundaries. OFF counts must be zero for migrated purely diagnostic work; separately record required functional reads/locks/publications. No rewritten test callback implementation. Assert unchanged native register/store order, source/generation/lifecycle decisions and shared snapshot values.

**Performance acceptance:** establish repeatable isolated fixture timing for OFF versus the preserved baseline and ON bounded workloads, with matched compiler/profile/configuration and enough repetitions to report distributions. Time I12 locks, I13 reads/predictions and I16 recovery traces separately; include stable repeated samples versus changing FOV and skip-like rapid ENTER/EXIT sequences. Report count/timing evidence and its limits. No arbitrary FPS claim from offline timings, no mandatory numerical threshold invented by this audit. A meaningful new cost or log flood stops promotion for that instrument until explained.

**Required suite/builds:** focused gate/performance/prediction/provenance/activation/startup/continuation/actual-adapter checks; all four N06-M01 scenarios; full canonical `test.cmd` with raw exit code and inventory equality; production build and compatibility diagnostic build with captured flags/exit codes, warnings compared to baseline and SHA-256 of outputs. Direct outputs/objects to ignored isolated `build-artifacts/` paths; preserve existing ASIs and release package. No game or ASI installation.

**Final source review:** verify no diagnostic-only preparation escapes OFF gates, no new owner/mutex/hook, no camera transaction changes, no unrelated file edits, no reclassified B1–B7/N03–N06 gates. Capture Git index/worktree state and hashes of untouched files. Existing dirty source changes belong to the user.

**Rollback:** if an instrument fails neutrality or bounded work, keep that instrument's compile isolation until its defect is resolved; do not turn off shared observations or weaken a failing test. Preserve independently validated prior batches. If a new independent suite failure occurs, record its first cause and stop dependent checks without expanding production scope.

## Future live validation and closure

After approved implementation and offline validation, a separately authorized matched in-game run should compare ordinary ASI with Diagnostics OFF/ON for Gameplay modes/ADS, Zoom, first and chained Cinematics, Dialogue, recovery, resolution/aspect changes, Overlay activation and clean exit. This audit does not authorize that run and does not require resurrecting the old E349 artifact for the new implementation. Record the current built/tested ASI identity when that run is performed.

Documentation closure requires an implementation report with per-batch actual changes, raw exits, warning comparison, SHA-256, measured OFF/ON work/cost and live limits. Do not use build success or static neutrality as proof of in-game timing/object lifetime. N03 remains Partially mitigated and N04–N06 retain their recorded open verification scopes unless separately resolved with relevant evidence.

## Approval recommendation

Approve the bounded passive migration and OFF continuation-work repair in Batches 1–4, then the narrow flag cleanup and validation in Batches 5–6. Do not approve automatic promotion of discovery/forensic/behavior-changing tools. No new INI category key is needed. Keep CMake/Ninja and incremental/source-registry modernization for a subsequent explicit task.
