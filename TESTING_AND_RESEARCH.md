# Testing and Research

This file is the concise, cumulative record of what the project has tested, discovered, established, rejected or left unresolved. It is not a transcript of each run. Artifact hashes, raw logs, detailed measurements and procedure-level evidence remain in the linked reports and evidence records.

Version 3.0.0 is published. The scope and version of a future release have not yet been decided. User-facing feature descriptions are in [`README.md`](README.md); historical reports retain the detailed evidence and context for the findings below.

## Test infrastructure and results

- The canonical suite is `test.cmd`. CMake + Ninja builds isolated harness targets; a manifest-backed runner executes them serially and propagates every nonzero child result, including signed Windows exception codes and timeouts.
- The registry contains **60 harness targets** and **63 ordered invocations**. The production-adapter matrix covers 102 cases with Diagnostics OFF and ON (**204 isolated processes**); N06-M01 adds four game-derived Zoom fixture invocations. Harness, invocation and scenario counts are distinct.
- Hybrid migration confirmed dependency-tracked incremental builds: unchanged warm runs do not recompile C++; changes to a harness, shared header, included production source, resource or generated input rebuild the dependent targets. The recorded median full warm run is about **119 seconds**, compared with about **391 seconds** in the earlier measured runner. These timings characterize the tested machine and setup, not a universal guarantee.
- Exit-contract fixtures cover success, positive and negative failures, access violation, timeout and prevention of running a stale executable after compile failure.
- Localization/resource checks cover 18 locales, catalog/resource parity, glyph metadata and embedded resources. They do not establish perfect Arabic shaping, bidirectional layout or visual rendering of every glyph in every runtime environment.
- Focused domain harnesses test deterministic policies and transactions. Actual-adapter harnesses execute production callbacks with controlled external dependencies. Local native-hook fixtures test selected continuation and fault cases. Each proves only its named contract; none substitutes for live-game evidence.

Details: [test infrastructure architecture audit](research/reports/TEST_INFRASTRUCTURE_ARCHITECTURE_AUDIT.md), [performance baseline](research/reports/TEST_EXECUTION_PERFORMANCE_BASELINE.md), [Hybrid migration plan](research/reports/TEST_INFRASTRUCTURE_HYBRID_MIGRATION_PLAN.md) and [implementation report](research/reports/TEST_INFRASTRUCTURE_HYBRID_MIGRATION_IMPLEMENTATION_REPORT.md).

## Runtime findings

### Gameplay and camera ownership

- The Gameplay camera writer and its register/source contracts are validated for the supported executable evidence recorded in [`docs/SUPPORTED_BUILD_MANIFEST.md`](docs/SUPPORTED_BUILD_MANIFEST.md). Signature resolution is guarded; an executable update requires fresh resolver and runtime validation.
- `RuntimeCombine` combines aspect recalculation with HorPlus: it synchronizes the Gameplay camera aspect to the current viewport, verifies the read-back, and only then applies HorPlus. This avoids depending only on the aspect captured when the game camera was created. Runtime aspect/resolution transitions and camera recovery have been exercised in game.
- Gameplay, Cinematic and Dialogue corrections share coordinated ownership and recovery. Offline transaction tests cover source/generation association, transition evidence and selected recovery paths; they do not emulate every game callback schedule.
- A native game aspect transition can trigger a camera reevaluation even when the final stored aspect value alone does not explain the visual result. The exact native operation and downstream projection rebuild remain unidentified; this control is not treated as a production mechanism.

### Cinematics and Dialogue

- Cinematic framing and FOV behavior has been tested across automatic and forced aspect policies, ENTER/EXIT, continuation, rapid transitions and Gameplay recovery. A prior defect that rejected the confirmed Gameplay baseline at initial establishment and continuation was fixed with source/generation/ownership-bounded acceptance; unrelated FOV samples remain fail-closed.
- Cinematic aspect selection uses the currently selected policy on the next ENTER. Actual observations remain distinct from Overlay potential assessments; configuration changes must not be presented as new camera observations.
- Dialogue correction uses `Adaptive`, `Reduced` and `Disabled`; there is no user-selectable `Native` Dialogue mode. Adaptive and Reduced preserve the game's optical zoom relationship to the captured Gameplay baseline. Disabling correction leaves the game's default dialogue zoom behavior, which targets 70° FOV.
- Dialogue lifecycle, EXIT recovery, consecutive dialogues, cinematic isolation and ADS specificity have domain/adapter and historical live-runtime coverage. The Dialogue hook contract uses the validated XMM6 input and XMM1 output boundary documented in the supported-build manifest.

### Overlay and localization

- The Overlay is an independent consumer of camera state. Its production path uses a private D3D11 device and DirectComposition surfaces, not game DXGI factory, swapchain or Present hooks. It can be disabled independently from camera correction.
- Overlay lifecycle, input, composition/presenter behavior, localization resources, layout and presentation have focused tests. Source files are organized by responsibility; the moves were checked with path/resource/build audits.
- Aspect ratios in the right-side technical panel share one presentation formatter. Offline locale/glyph audits do not prove all shaping, RTL or graphics-driver behavior.

### Live smoke coverage

User-run live smoke tests, including the CMake/Ninja-built production candidate, have exercised normal startup/readiness, Gameplay RuntimeCombine and aspect changes, Cinematic ENTER/EXIT and recovery, Dialogue lifecycle, Zoom IN/OUT callback events, Overlay startup/frames/toggle/resize, and Diagnostics enabled and disabled. This establishes those observed paths for the tested build and scenarios. It does not establish complete visual Zoom behavior, every hook teardown or failure path, or behavior with every graphics wrapper. Detailed logs and artifact identity remain in runtime evidence, not this overview.

## Native runtime safety

- **Zoom exception containment:** Zoom IN/OUT callbacks enter through `SafeMidHookEntry`; production-adapter tests and a local executable-hook fixture cover normal and selected exceptional paths while preserving native continuation context.
- **SafetyHook executable lifetime:** hook state distinguishes unpatched, patched and uncertain target bytes. Executable trampoline/stub storage is retained when patched code may still reference it. This is a safety retention policy, not proof of successful teardown; isolated fault-injection tests cover selected protection, write, move, reset and destruction paths.
- **Startup exception containment:** the initialization thread has an outer exception boundary and guarded, one-attempt cleanup. Controlled tests cover early/partial failures and secondary cleanup errors. Full game-specific initialization and installer failure behavior is not reproduced by the offline fixture.
- N06-M01 fixtures use game-derived Zoom instruction/register contracts and exercise both directions and diagnostic settings. Static hook-site validation plus fixtures do not establish all live native consumers, object lifetime or continuation behavior.

Detailed status, limitations and evidence: [native-runtime evidence audit](research/evidence/runtime-decomposition/NATIVE_RUNTIME_EVIDENCE_AUDIT_2026-10-09.md), [N01–N03 remediation report](research/evidence/runtime-decomposition/N01_N03_NATIVE_RUNTIME_SAFETY_REMEDIATION_IMPLEMENTATION_2026-10-09.md), [N06 consolidated evidence](research/evidence/native-runtime/N06/N06_NATIVE_RUNTIME_EVIDENCE_CONSOLIDATED.md) and [N06-M01 validation](research/evidence/native-runtime/N06/tools/n06_m01_validate.ps1).

## Diagnostics and performance

- Passive camera/runtime diagnostics are compiled into the ordinary ASI and controlled by `[Diagnostics] Enabled`. OFF-path adapter checks confirm diagnostic-only continuation trace work, counters, predictions and selected extra reads are gated; shared observations needed by production behavior remain active.
- The isolated continuation-trace measurement showed a large reduction in that diagnostic leaf path when disabled. It is not a whole-frame, disk-I/O or in-game FPS measurement.
- Repeated restoration-state logging was reduced to state changes. This removed the observed duplicate-log flood in the tested scenario; it was not presented as proof that logging caused a previous 0-FPS stall.
- A long cinematic-skip stall was not reproducible after a system restart with modules enabled. No causal link to the mod or operating system was established.

See the [Diagnostics Unification report](research/reports/DIAGNOSTICS_UNIFICATION_IMPLEMENTATION_REPORT.md) and the relevant dated runtime reports for details.

## Closed, rejected and deferred research

- **Weapon/viewmodel FOV:** UE4SS and package analysis established the reference `.wvf` path and causally connected `MPC_FOV.TanFOV` changes to visible viewmodel framing. A safe, patch-resilient UE reflection/invocation bridge for the standalone ASI has not been established, so a production repair remains deferred.
- **Subtitle horizontal centering:** the displacement reproduces without the mod. Tested child alignment/justification and fixed offsets did not provide a robust correction; root translation moves the composition but no safe post-layout geometry contract is available. No production subtitle modification was made.
- **Native aspect reevaluation:** a settings-driven transition changes the rendered result without requiring the final camera aspect field to equal the physical display aspect. The exact engine reevaluation trigger remains unknown; storage-only writes are not considered equivalent.
- **Overlay graphics compatibility:** the production Overlay avoids game presentation hooks, but universal compatibility with every driver, Frame Generation, VRR or third-party wrapper is not established. HDR/scRGB parity and some rendering/shaping cases remain outside the proven contract.
- **Runtime decomposition:** B1–B7 are complete as an architecture campaign. Hotkey mapping/persistence and independent domain helpers were extracted where useful; adapters tied to `RuntimeState`, native callback ABI, lifecycle, publication and synchronization remain with the single runtime owner. The file is not being split merely to reduce its line count.

Further historical investigations are indexed in [`research/README.md`](research/README.md). The complete dated reports and evidence are retained under [`research/reports/`](research/reports/) and [`research/evidence/`](research/evidence/); consult them for exact procedures and provenance rather than duplicating run-specific details here.
