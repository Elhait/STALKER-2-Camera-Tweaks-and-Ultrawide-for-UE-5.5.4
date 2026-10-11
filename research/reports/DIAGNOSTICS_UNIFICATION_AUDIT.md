# Diagnostics Unification — Runtime Gates Architecture Audit

Date: 2026-10-11

Status: **Audit complete; implementation not authorized.**

Companion documents: [gate matrix](DIAGNOSTICS_UNIFICATION_GATE_MATRIX.md) and [implementation plan](DIAGNOSTICS_UNIFICATION_IMPLEMENTATION_PLAN.md).

## Scope and evidence

This is a static review of the current working tree, not of the pasted historical build script alone. It covers project-owned instrumentation in `src/`, production/diagnostic entry points, the canonical test registry and its adapter-input generator. Existing staged source reorganizations and safety remediation changes were preserved. No compiler, harness, game, installer or build was run for this audit.

Authority: `docs/ARCHITECTURE.md` (Persistent runtime diagnostics; Build and test policy), `docs/SAFETY_INVARIANTS.md`, current configuration/runtime ownership and native callback source. Existing evidence was read for coverage limits, not counted as freshly executed validation:

- `research/evidence/runtime-decomposition/NATIVE_RUNTIME_EVIDENCE_AUDIT_2026-10-09.md`.
- `research/evidence/runtime-decomposition/N01_N03_NATIVE_RUNTIME_SAFETY_REMEDIATION_IMPLEMENTATION_2026-10-09.md`.
- `research/evidence/native-runtime/N06/N06_NATIVE_RUNTIME_EVIDENCE_CONSOLIDATED.md`.

The source baseline at inspection has SHA-256 `31756C900260C59ACE0898BE279B68B8CAD8BF9DE9BCF9608478AE8FE2BCE6BB` for `src/plugin/runtime.cpp`, `9C79ACE6E0CA39014F35898E47E3A94538D86B81436EC26FC8003DE8A7668C94` for `build.cmd`, and `9F3738296A92ACFD5DBF3F41B16A9FC185490376E19385E572C5BEC3EF59E4FF` for `test.cmd`. Line references below belong to this working-tree source and will move during implementation.

## Outcome

The ordinary ASI already contains substantial persistent runtime diagnostics. The remaining supported diagnostic build defines selectively add HorPlus FOV telemetry, cinematic before/after writer observations and MatchGameplay prediction. Optional Dialogue counters/recovery tracing still require build-time switches. These are suitable candidates for INI-gated availability after the gate and memory/lifetime checks described below.

Removing every diagnostic-looking preprocessor condition would be unsafe. Several old experimental branches change game memory, callback output, replay state or worker/hook installation. They must remain outside automatic production unification.

The recommended runtime switch is the existing `[Diagnostics] Enabled` boolean, with the current startup-only application behavior. No additional category key, settings owner, mutex, camera hook or runtime reload API is justified for the passive migration. Preserve the startup journal and operational/safety logs under their existing independent policies.

## Confirmed findings

| ID | Classification | Evidence | Consequence and bounded remedy |
| --- | --- | --- | --- |
| DU01 | Contract defect | `build.cmd:41–49` supplies `HORPLUS_FOV_STATE_DIAGNOSTIC`, `HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC`, `MATCHGAMEPLAY_DIAGNOSTIC` only for the diagnostic profile; consumers are in `runtime.cpp:4336`, `3841`, `3918` and callback call sites. | Ordinary INI-enabled diagnostics cannot activate these implementations. Make the passive implementation always compiled and retain early runtime gates. |
| DU02 | Contract defect in optional instrumentation | `TraceDialogueBoundary:3674` increments counters without `Enabled()`; `LogDialogueDiagnosticSummary:3556` logs unconditionally; endpoint arming/observation at `3591/3600` has no master gate. | Enabling these compile flags alone would introduce diagnostic work/logging with INI OFF. Guard caller preparation, arming, counters, reads and shutdown summary, not only emission. |
| DU03 | Confirmed diagnostic OFF work | `RuntimeState:237` eagerly allocates `CinematicContinuationTrace`; accepted ENTER calls `Begin(false)` or `Discard()` at `3161`; `ReplayManualTransition:4968` explicitly enters trace preparation when diagnostics are OFF; `Record(false)`, `Finish(false)` and `Begin(false)` take a mutex and reset a 512-sample state. | OFF still constructs a trace input, locks and clears diagnostic storage. Make storage conditional on enabled startup configuration and guard the adapter before trace preparation. Preserve helper reset contracts for direct users; do not gate production continuation/recovery transactions. No millisecond cost was measured. |
| DU04 | Misleading build flags | `ZOOM_TRANSITION_DIAGNOSTIC`, `CAMERA_STATE_SNAPSHOT_DIAGNOSTIC` occur in build/test switches but have no conditional consumers in current `src/`. `OVERLAY_BREAK_ON_PRESENT_FAILURE` likewise has no source consumer. | These switches do not establish availability in this revision. Remove obsolete references after repository-wide consumer checks; do not create behavior to justify a stale flag. |
| DU05 | Safety-critical design constraint | `GetCameraWriterCinematicStateView:1469` and `GetCinematicLifecycleStateView:1502` expose `matchGameplay*` fields unconditionally; `cinematic_lifecycle_transaction.hpp:212–223` publishes ENTER/baseline facts and resets at `424–426`. | The names resemble diagnostic state but these are production inputs. Keep their publication/reset and ownership unconditional. Only gate the observational prediction/formatting that reads them. |
| DU06 | Experimental coupling; exclude | `COMBINED_GAMEPLAY_DIAGNOSTIC` includes F8 aspect writes at `2533` and replay rearm suppression at `2763`; `GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER` writes flags at `2708`; `HORPLUS_GAMEPLAY_DIAGNOSTIC` writes XMM0 at `5528`; delayed/deferred/atomicity branches alter transitions. | These are not passive diagnostics. Keep compile isolation and do not activate them through the ordinary INI switch. |
| DU07 | Lifecycle/forensic review required | `DIALOGUE_DISCOVERY_DIAGNOSTIC` starts a worker at `6599`; F12 arms a 10-second sweep (`DialogueDiscoveryLoop:5760`) and the callback reads 33 words plus register/field snapshots (`5690`). | Broad memory discovery and a dedicated hotkey/worker should not be added automatically. F12 also defaults to DialogueCycle. Keep the current research gate; a future promotion needs a separate design decision. |
| DU08 | Current API limit, not a defect | `FeatureConfig::diagnosticsEnabled` defaults false; parser accepts `[Diagnostics] Enabled`; production calls `SetEnabled` only after startup configuration load (`6428`). `RuntimeSettingKind` has no Diagnostics variant and no production configuration reload path was found. | INI edits take effect after restart. Do not promise hot reload or introduce category/reload APIs as part of unification. Test-only `SetEnabled` calls do not establish a user-facing runtime toggle. |
| DU09 | Verification requirement | Candidate traces perform extra reads and acquire existing telemetry/diagnostic locks while the correction gate is held. Event-based emission can still happen frequently during a changing FOV. | Prove OFF work suppression, memory containment, unchanged output/state and bounded ON emission. Performance remains unmeasured; no numeric budget or zero-overhead claim is made here. |

## Existing runtime gate and configuration

`src/diagnostics/diagnostic_runtime.cpp` owns one atomic boolean, initially false. `SetEnabled` stores with release; `Enabled` loads with acquire. `EmitIfEnabled` skips invoking its lambda while disabled. This is a suitable thread-safe visibility mechanism for a startup selection; it is not a callback-quiescence, resource-publication or reload protocol.

`src/config/config_repository.cpp:324–328` parses the existing boolean with `ParseBool`; `FeatureConfig:89` defaults false. `config_template.cpp` registers one Diagnostics key (`Enabled=false`). Unknown/invalid-value behavior must remain with the current parser. Managed INI writing retains the setting; no new persistence channel is needed.

Startup ordering matters: `Initialize` loads configuration, starts configured Overlay discovery, then publishes `diagnosticsEnabled`, then settings/camera capabilities and native hooks. Before publication, the gate remains false. Any new lazy diagnostic storage must be created after the selection is known and before its native callback becomes active, outside loader lock. No new callback allocation or installation ordering change is proposed.

Shutdown continues to close readiness and signal/join workers before hook reset. Diagnostic teardown summaries may run only when enabled and when meaningful data exists. They must not turn failed cleanup or retained executable/worker resources into a successful teardown claim.

## Gate model proposed for implementation

| Channel | Authoritative policy | OFF expectations |
| --- | --- | --- |
| Passive runtime diagnostics | Existing `diagnostics::Enabled()` / `[Diagnostics] Enabled` | Return before diagnostic-only reads, lock acquisition, snapshots, predictions, dedup updates, string creation, log closures or I/O. A gate check and native pass-through boundary remain. |
| Shared production observations | Existing camera/lifecycle/association owner | Continue required sampling, snapshots, freshness, generations and publications independently of Diagnostic Mode. |
| Startup milestones | Existing production startup-journal policy | Retain the bounded journal even with Diagnostics OFF. DllMain remains capture-only; file flush occurs outside loader lock. |
| Safety, configuration and operational lifecycle logs | Existing failure/lifecycle policy | Preserve errors, local capability failures, configuration/persistence failures, readiness and meaningful lifecycle events. Never gate the general logger globally. |
| Forensic startup tracing | Existing explicit `CAMERA_TWEAKS_STARTUP_TIMELINE=1` build opt-in | Remain separate; module enumeration, DLL notification registration and instruction/export inspection are not ordinary observations. It replaces the journal because they target the same log. |
| Experimental writes/research hooks/scans | Explicit isolated research gates | Remain absent from the ordinary supported artifact until separately approved. |

Use caller gates where argument construction performs work, and helper gates as a defensive boundary. `EmitIfEnabled` is useful when the lambda includes the entire expensive preparation. A log function cannot undo already executed argument formatting. Do not interpret every `Log`/`Renderer::Log` call as diagnostic-only: those entry points also deliver mandatory operational failures.

Startup-only selection removes the need for production off/on transition resets. Tests may exercise the atomic helper dynamically, but broad runtime toggling would require an explicit contract for caches, TLS dedup, continuation buffers, callback quiescence and performance summary rearming. That extension is not part of the proposed implementation.

## Memory, synchronization and native safety

- **HorPlus trace:** `TraceHorPlusFovState` starts with an early gate. It uses existing `g_dialogueMutex` then `g_horPlusFovTelemetryMutex` under the correction boundary; it writes only telemetry/dedup state. Preserve acquisition order and the telemetry reset in `SelectGameplayMode`. Do not create another telemetry owner.
- **Cinematic writer trace:** before/after readers use RSI/RBX, validated memory reads of aspect/flags, source `+0x230/+0x234`, output `+0x30/+0x5C`. They return before reads when OFF. MatchGameplay evaluates only for an emitted snapshot and does not publish a correction candidate. Keep invalid/stale source handling and callback exception containment.
- **Dialogue endpoint trace:** uses RSI `+0x28/+0x2C/+0x30`, TLS state and a maximum of 256 observations. Gate the additional stable-sample comparisons before calling it. The production target read and Dialogue transaction remain active. Arming/clearing must stay at their existing lifecycle locations when enabled.
- **Continuation diagnostic trace:** its private 512-sample buffer/mutex is independent of production continuation state. A nullable resident-owner member is the narrow ownership location for optional storage. Allocation failure should disable only this diagnostic, preserve correction and avoid repeated callback allocation attempts. Prepare diagnostic input only when ON and the tracing condition holds; shared `writerTransaction` results are unaffected.
- **Zoom:** `SafeMidHookEntry` preserves N02 containment. Hooks are installed independently of Diagnostics because Zoom hold/active/freshness facts are consumed by production camera presentation. Four memory reads and shared snapshot publication currently occur with OFF. This is an explicit functional-observation exception, not proof of free diagnostics. Do not remove hooks, change the known-image guard, or gate shared data without a separate dependency proof. Extra pure diagnostic counters/dedup may be separated only after use-site checks.
- **Performance telemetry:** `ScopedRuntimeMutex` performs the required mutex lock/unlock when OFF but skips timestamps/statistics; `ScopedRuntimeLog` skips measurements; events/summary preparation are early-gated. Existing functional locks are not diagnostic overhead to eliminate. Summary is explicitly requested on Overlay activation and drained by `HotkeyLoop`, not automatically on every sample.
- **Logging:** `Log` defers formatting/emission until correction-gate release; `LogImmediate` is auxiliary/noexcept. Synchronous logger work remains possible after gate release. Overlay composition logging uses a mutex, stream write and flush; do not redesign either logger without measurements. New ON traces need meaningful dedup/bounds to avoid repeating the historical flood.
- **Startup:** the journal uses 48 events, 16 once markers and an SRW-protected file drain. Forensic timeline uses 128 events, module notifications and byte inspection; it is not equivalent to the journal. Both have loader-lock-sensitive capture paths and must retain their distinct policies.

No candidate requires changing register/store order, original instruction continuation, SafetyHook patch-state/retention, correction ownership or native ABI. Nevertheless, static inspection cannot establish live object lifetime or timing neutrality. Preserve existing `SafeMidHookEntry` registrations and do not promote the old unwrapped forensic hook callbacks into production.

`platform::win32::ReadMemory` validates committed/readable protection and region bounds with `VirtualQuery`, then copies with `memcpy`. This is not a lifetime lease or structured-exception barrier: memory can change after validation, and the caller must bound register-derived offset arithmetic. `SafeMidHookEntry` contains C++ exceptions under the current `/EHsc` build; it is not an access-violation isolation guarantee. Migrating diagnostic reads therefore needs invalid/overflow-address cases and must preserve the existing N04/object-lifetime evidence limit. If safety requires a new general memory-access mechanism, stop that promotion for separate approval rather than disguising it as gate cleanup.

## Tools suitable for the ordinary artifact

Suitable after implementation validation, using the existing switch: HorPlus FOV/owner/baseline trace; cinematic before/after writer trace; MatchGameplay prediction; Dialogue callback counters/summary; bounded Dialogue recovery endpoint trace; the already-present restoration/source/association/lifecycle/publication/provenance/performance/Overlay activation tools. The continuation diagnostic needs its OFF allocation/locking preparation fixed within the existing runtime owner. These are observations, not new camera authority.

Separate architecture approval remains required for: Dialogue memory discovery and F12 worker; extra post-EXIT writer/producer/consumer hooks; broad forensic/startup/module scans; camera-mutating one-shot, combined, delayed/deferred/atomicity and HorPlus POC tools; experimental font/UI builds. The pure `FOV_SETTINGS_TRACE_DIAGNOSTIC` trace is passive but requires a supersession decision against the richer production source trace before adding redundant output.

## Validation coverage and limits

Existing foundation: diagnostics gate, performance telemetry, MatchGameplay prediction, Dialogue provenance, camera writer identity, cinematic lifecycle context, startup journal/timeline and Overlay activation harnesses; cinematic continuation tests in `tests/cinematics/cinematic_fov_harness.cpp`; actual adapter harnesses and N06-M01 executable fixtures.

These do not yet prove zero diagnostic-only work in all disabled adapters. `runtime_native_adapter_harness.cpp` includes actual production `runtime.cpp` once and supports Diagnostic Mode OFF/ON; `tests/runner/runtime_native_adapter_inputs.ps1` extracts its remaining inputs from the build registry. Preserve that arrangement and add observable memory/trace/log counters for the candidate boundaries, not rewritten adapter replicas. Add dedicated compiled coverage for currently optional Dialogue code before claiming it verified.

No tests, builds, performance measurements or live validation were performed in this audit. Future checks must distinguish deterministic work counts, local callback timing and in-game performance. A PASS from the earlier runner remediation is not fresh diagnostics-unification evidence.

## Preserved gates and disposition

This audit does not reopen B1–B7 decomposition or change N01–N06. N01/N02 remain Fixed within their recorded bounds; N03 remains Partially mitigated; N04/N05 remain Open; N06 remains Partially verified / Open. Historical exact-artifact/live evidence limits remain in their existing records. New diagnostic compilation does not close them.

Proceed to the six bounded batches only after implementation approval. Build-system modernization, source-registry migration and CMake/Ninja are a separate task. Ordinary `build.cmd` should remain a one-run production entry point, producing `STALKER2CameraTweaks.asi`.
