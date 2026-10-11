# Test infrastructure architecture audit

Date: 2026-10-11. Scope: research only; no migration or remediation approved.
Baseline: HEAD `db67e4dfc56cfe06394c38413f5bb460f797d210` plus the existing dirty working tree, not a clean historical commit.

## Executive finding

The canonical CMD suite is complete by its current **60 unique harness** contract, but is not incremental. It issues **61 compiler/link commands**, compiles **430 source occurrences**, and executes **64 direct harness commands**. The actual-adapter executable is compiled twice with identical inputs and executes the same 102 × Diagnostics OFF/ON matrix twice.

The existing Ninja graph demonstrates useful target-isolated reuse, but covers only **2/60 harnesses and five CTest entries**. It cannot replace the canonical suite today. Its native executables also lack the localization `.res` in their link inputs, unlike the CMD executables.

The highest-priority correctness finding is independent of migration: CMD's `if errorlevel 1` skips negative native exit codes. A controlled exit-code probe demonstrates this; it is not evidence that an existing production callback crashed.

Timing measurements and alternative recommendations are in [the performance baseline](TEST_EXECUTION_PERFORMANCE_BASELINE.md) and [modernization options](TEST_INFRASTRUCTURE_MODERNIZATION_OPTIONS.md).

## Evidence and counts

| Unit | Current canonical value | Meaning |
| --- | ---: | --- |
| Unique harness source basenames | 60 | Inventory equality PASS; not scenario count |
| Compiler/link invocations | 61 | One duplicate native-adapter compilation |
| Source compilation occurrences | 430 | Includes repeatedly compiled production/vendor dependencies |
| Direct executable invocations | 64 | 58 ordinary + two matrix parents + four N06-M01 |
| Native matrix cases | 102 | Each independently executed with Diagnostics OFF and ON |
| Matrix child executions | 408 | 204 per parent × two canonical parent runs |
| N06-M01 executions | 4 | Zoom IN/OUT × OFF/ON |
| Total harness process starts | 472 | 64 direct starts + 408 children; excludes compiler/PS processes |
| CMake test executables | 2 | Native adapter and game-derived Zoom |
| CTest entries | 5 | One 204-child matrix + four Zoom tests |

Repeated runs leave the source inputs unchanged. They do not constitute game runtime verification or change N03–N06 status.

## Findings

| ID | Severity | Confirmed evidence | Recommendation / limits |
| --- | --- | --- | --- |
| TIA-01 | High, false-green risk | `test.cmd` guards at lines 39 and throughout use `if errorlevel 1`; temporary native exit probe returned -1073741819 and bypassed that guard, while an explicit nonzero check detected it | Independently approve nonzero exit propagation and signed crash regression; audit script must validate the improved contract. No existing suite crash alleged |
| TIA-02 | Medium, wasted build/runtime work | Compiler commands at lines 38 and 288 are identical; run commands at 278 and 290 execute the same full OFF/ON matrix | Seek explicit approval to remove duplicate compilation/execution; do not silently change execution multiplicity during a migration |
| TIA-03 | Medium, no incrementality | Every canonical run issues all 61 `cl` commands, even without source changes; no dependency graph/timestamp decisions | Ninja can remove warm compile/link cost; preserve all runs and assertions |
| TIA-04 | Medium, unsafe manual object reuse | Flat `build-artifacts/tests/` object output is overwritten by different definitions; `native_runtime_evidence.ps1 -ReuseObjects` checks existence only | Use isolated target/flag groups and dependency-tracked objects; do not use existence as provenance |
| TIA-05 | Medium, incomplete replacement | CMake has 2 executables / 5 entries rather than 60 harnesses; no test-inventory parity gate | Explicit test registry and command/argument equality before canonical switch |
| TIA-06 | Medium, resource parity gap | `configure_actual_adapter_target` adds a build-order dependency on `localization_resources`, but neither executable links `resource_output`; actual Ninja link command confirms omission | Add resource/link equivalence to any proposed migration gate. Existing CTest success does not prove embedded-resource coverage |
| TIA-07 | Medium, assertion preservation | Ordinary CMD harnesses do not define NDEBUG; language transition and localization use runtime `assert` | Do not apply production Release/NDEBUG settings to all tests. Preserve assertion-bearing targets explicitly |
| TIA-08 | Medium, orchestration completeness | Inventory audit deduplicates names and recognizes argument-bearing lines, but does not validate the four Zoom argument tuples or execution multiplicity | Registry should distinguish executable inventory, test entries, scenarios and process isolation |
| TIA-09 | Low/Medium, hang isolation | Native parent gives each child 10 s and detects any DWORD nonzero; ordinary CMD runs lack timeouts; CTest entries have no explicit TIMEOUT or resource policy | Bounded process timeout and fixtures/temp isolation; choose limits from measured test behavior, not blanket tight limits |
| TIA-10 | Low, repeated preflight work | Four canonical PowerShell processes; candidate always reconfigures and runs locale_preflight | Measure before optimizing; resource/locale audits enforce correctness and must not be blindly omitted |

### Exit-code reproduction

An ignored temporary native helper returned a supplied status, without crashing or patching the suite. Its isolated CMD driver printed:

```text
negative_errorlevel=-1073741819
negative_detected_by_nonzero
positive_errorlevel=1
positive_detected_by_ge1
```

The negative `detected_by_ge1` line was absent. `if errorlevel 1` means greater-than-or-equal to 1, not nonzero. A real access violation can therefore be ignored by this runner, and a compiler crash may also leave an old executable available. The suite's observed successful runs did not exhibit that failure; this is a verified runner contract defect.

## Build and dependency model

### Canonical flags

All compiler invocations expand this COMMON value:

```text
/nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui
```

The generated-header directory `build-artifacts/generated` is prepended to INCLUDE. Native/N06 targets additionally use:

```text
/DNDEBUG /DSAFETYHOOK_TEST_VIRTUAL_PROTECT /DOVERLAY_PRODUCTION
/DOVERLAY_SETTINGS_FRONTEND /DOVERLAY_COMBINED /DOVERLAY_STARTUP_JOURNAL
/Iexternal\imgui\backends
```

Startup journal and signature scanner have their own reviewed per-target defines; all other ordinary harnesses must retain their existing definitions. The complete commands below are authoritative for per-target flags and libraries.

A real compiler probe reported `_MSC_VER=1944`, `_MSVC_LANG=202400`, `__cplusplus=199711` with `/std:c++latest`. This verifies the effective current language mode. Legacy `__cplusplus` without `/Zc:__cplusplus` is not evidence of an old C++ language mode.

### Header tracking and reuse

CMD compiles everything and therefore ordinarily picks up header edits by rebuilding all sources, not by tracking dependencies. Its flat object directory is not a safe incremental cache. No current distinct-source basename collision was found, but sources with different macro contracts reuse the same object basename across sequential commands.

Ninja isolates `runtime_adapter_test_dependencies` from production objects and shares that 59-source object library between native and Zoom tests. The runtime owner is included by each harness and must never also be compiled as a standalone dependency. Reusing production objects is unsafe when `SAFETYHOOK_TEST_VIRTUAL_PROTECT`, NDEBUG, instrumentation, injected adapters or include environments differ. Reuse is allowed only for proven identical compilation contracts.

The candidate's actual-adapter dependency records are populated: native harness 251 dependencies and Zoom 254, marked VALID. They include production owner/header/generated inputs. The current helper uses UTF-8 codepage and CMake's detected localized `/showIncludes` prefix. Historical zero-dependency/stale records still exist for production objects in the candidate cache; they are not evidence that every current profile has been repaired. A replacement requires a cache/version/prefix migration policy and explicit header-change validation in each supported build environment. This audit did not touch headers to manufacture a rebuild.

Production/diagnostic/timeline/discovery presets use distinct binary directories. The candidate helper fixes its own production directory. No project-wide all-harness test presets exist yet.

### Source duplication

| Source | Canonical compile occurrences |
| --- | ---: |
| `src/diagnostics/diagnostic_runtime.cpp` | 20 |
| `src/diagnostics/performance_telemetry.cpp` | 18 |
| `src/dialogue/dialogue_state.cpp` | 15 |
| `src/camera/gameplay_baseline.cpp` | 15 |
| `src/camera/horplus.cpp` | 14 |
| `src/cinematics/cinematic_fov.cpp` | 13 |
| `src/gameplay/aspect_policy.cpp` | 11 |
| `src/camera/fov_observation.cpp` | 11 |
| `src/gameplay/gameplay_state.cpp` | 10 |
| `src/config/feature_config.cpp` | 10 |
| `src/gameplay/horplus_gameplay.cpp` | 10 |
| `src/hooks/instruction_validator.cpp` | 9 |
| `external/safetyhook/Zydis.c` | 9 |
| `src/camera/presentation_state.cpp` | 8 |
| `src/cinematics/cinematic_selection.cpp` | 8 |

The three 61-input actual-adapter compilations account for 183 of the 430 source occurrences. Two are the same native adapter; the third is the game-derived Zoom executable. Ninja's current object library reduces those dependencies to 59 shared compilations plus two harness and two binding-probe compilations on a clean build, without combining runtime owners. Further common libraries require per-target macro/include/assertion equivalence, not file-name matching.

## Runner and auxiliary scripts

| Script / infrastructure | Responsibility | Risks / boundaries |
| --- | --- | --- |
| `test.cmd` | Full 60-harness compile + sequential execution; generates header, RSP and RC resource | Rebuilds all; duplicate native matrix; negative-exit bug; no selective entry point |
| `tests/runner/test_cmd_audit.ps1` | Source/compile/run basename equality and adjacent literal guard | 60/60 PASS; dedup masks multiplicity and future duplicate basenames; argument combinations not enforced |
| `tests/runner/runtime_native_adapter_inputs.ps1` | Validates 61-entry production registry; emits 59-dependency RSP | Correctly excludes runtime owner and DLL entry; stable write if unchanged |
| `tests/runner/localization_catalog_audit.ps1` | Strict UTF-8, 18 locale/212-key parity, placeholders, resource/ID/font wiring and structural UI contracts | Scans source/policy too; not just JSON parsing. Do not equate this with runtime glyph coverage |
| `tools/generate_ini_documentation.ps1` | Generated canonical INI header | Stable unchanged writes; graph must depend on generator and English catalog |
| `tests/runner/native_runtime_evidence.ps1` | Capture, focused/full runs, locale, builds and preservation | Logs exact commands/codes; optional object reuse is not provenance-safe. Historical ordinary/diagnostic defines currently identical; runtime matrix itself still tests OFF/ON |
| `tools/build/test-cmake-candidate.cmd` | VS environment, configure, build two targets, CTest | Five entries only; repeated environment/configure/preflight; same negative-errorlevel class in intermediate guards |
| `CMakeLists.txt` / `CMakePresets.json` | Explicit production registry, object isolation, generated inputs and five tests | No full test registry, no full test presets, no explicit test timeout/resource locking; native resource-link gap |
| `research/evidence/native-runtime/N06/tools/n06_validate.ps1` | Historical game-image/signature provenance validation | Research evidence, not canonical scenario runner; preserve game/build identity separately |
| `research/evidence/native-runtime/N06/tools/n06_m01_validate.ps1` | Focused four game-derived Zoom fixture validations | Specialized historical build/run support; no reason to duplicate provenance work in every canonical run |
| `research/tools/capture_runtime_decomposition_baseline.ps1`, `runtime_decomposition_move_ledger.ps1` | Structural baseline and move classification | Historical/documentation tools, not required compilation/test execution |
| `tools/fonts/rebuild_catalog_subsets.py` | Regenerates font assets | Mutating maintenance tool, not a read-only audit step; not run |
| Other `tools/build/build-*.cmd` and archived build profiles | Product/experimental diagnostic binaries, plus a language-state transition test in the watcher builder | Not extra canonical harness inventory; do not import forensic/discovery builds into ordinary tests |

Ordinary harness scenario assertions run inside their executable. They must not be counted as one assertion just because there is one runner line. Appendix A records known named scenario calls where present; inline assertions remain part of the unchanged executable.

### Isolation, selective execution, and parallelism

Current execution is sequential; native parent launches each case in a fresh process. A selected native case can already be invoked with `runtime_native_adapter_harness.exe <case> off|on`; Zoom takes `zoom-in|zoom-out off|on`. Ordinary executables can be run manually, but CMD has no supported selective build/run path and their binaries may be stale outside a full run. CTest supports selection of its five registered entries, not the missing 58 executables.

Config persistence uses PID-scoped directories; placement uses PID-scoped temporary paths; native settings API uses PID-scoped INI names. Native memory patch fixtures operate inside their own processes. These are good candidates for bounded process parallelism after a shared-resource inventory. Timing-sensitive gate/serialization tests wait 30 ms for intermediate checks and use bounded waits; oversubscription can cause false timeouts. Keep native cases process-isolated and initially preserve their serial parent orchestration. Do not parallelize CMD compilation into its shared /Fo directory. Do not run two full suites sharing generated/temp/output paths.

Failure logs currently stream to console, with scenario names for native children. The evidence wrapper retains command/output/exit logs. CTest adds per-test result/duration logs, but a replacement still needs stable raw compiler logs, scenario output, crash exit/timeout evidence and immutable build-input identity.

### Configure / archival-script caveats

The production JSON registry is read with `file(READ ...)`; there is no explicit `CMAKE_CONFIGURE_DEPENDS` declaration for that external JSON. The current wrapper's unconditional configure compensates for this. Removing configure from the warm path requires an explicit manifest-trigger contract first; do not assume Ninja automatically tracks arbitrary configure-time file reads.

The historical N06 validator's `BuildProbe` branch still parses a source block from `build.cmd`, whereas the active build uses the JSON registry. Its dependency-reuse mode, and N06-M01's `-ReuseDependencies`, verify object presence rather than complete source/header/flag identity. These are additional archival-tool maintenance risks, not reasons to include game-image hashing in the canonical suite. Neither script was executed or changed in this audit.

## Appendix A — complete Test → Sources → Compiler → Executable → Runner → Scenarios → Dependencies matrix

Compiler IDs C01–C61 refer to Appendix B; line numbers refer to current `test.cmd`. All executables below use `build-artifacts/tests/<basename>.exe`. Runner references are exact CMD line/arguments. Direct includes are listed separately from compiled sources; their transitive closure is rebuilt unconditionally by CMD. The native 59-source set ASET is expanded after the table.

| Test source | Compiled sources | Compiler | Executable | Runner | Scenarios | Direct dependencies |
| --- | --- | --- | --- | --- | --- | --- |
| `tests/plugin/runtime_native_adapter_harness.cpp` | `tests/plugin/runtime_native_adapter_harness.cpp`; `tests/plugin/runtime_owner_binding_probe.cpp`; ASET | C01 (L38); C61 (L288) | `runtime_native_adapter_harness.exe` | L278; L290 | 102 listed cases × off/on; repeated twice | `../../src/platform/win32/viewport.hpp`; `../../src/platform/win32/memory.hpp`; `../../src/gameplay/gameplay_camera.hpp`; `../../src/plugin/camera_writer_transaction.hpp`; `../../src/plugin/runtime.cpp`; `runtime_native_evidence_cases.hpp` |
| `tests/plugin/n06_m01_zoom_game_derived_harness.cpp` | `tests/plugin/n06_m01_zoom_game_derived_harness.cpp`; `tests/plugin/runtime_owner_binding_probe.cpp`; ASET | C02 (L41) | `n06_m01_zoom_game_derived_harness.exe` | L280 zoom-in off; L282 zoom-in on; L284 zoom-out off; L286 zoom-out on | zoom-in/out × off/on; native loads, branch outcomes and continuation | `runtime_native_adapter_harness.cpp`; `n06_m01_zoom_game_derived_fixtures.hpp` |
| `tests/lifecycle/worker_lifecycle_harness.cpp` | `tests/lifecycle/worker_lifecycle_harness.cpp`; `src/plugin/worker_lifecycle.cpp` | C03 (L44) | `worker_lifecycle_harness.exe` | L161 | TestHotkeyPlusDiagnosticWorkerCapacity; TestNormalStartupAndControlledStop; TestPartialStartupFailure; TestStopBeforeStartup | `../../src/plugin/worker_lifecycle.hpp` |
| `tests/feature_status/feature_status_harness.cpp` | `tests/feature_status/feature_status_harness.cpp`; `src/plugin/feature_status.cpp` | C04 (L46) | `feature_status_harness.exe` | L163 | TestDialogueLifecycleCapabilityMatrix; TestDisabledConfigurationRemainsValid; TestDisabledGameplayCanRetainRecoveryObservation; TestDisabledIsDistinct; TestFeatureFailureDoesNotInvalidatePeers | `../../src/plugin/feature_status.hpp` |
| `tests/plugin/optional_overlay_startup_harness.cpp` | `tests/plugin/optional_overlay_startup_harness.cpp` | C05 (L48) | `optional_overlay_startup_harness.exe` | L165 | Inline main assertions/checks (no separate runner scenarios) | `../../src/plugin/optional_overlay_startup.hpp` |
| `tests/plugin/camera_correction_gate_harness.cpp` | `tests/plugin/camera_correction_gate_harness.cpp`; `src/dialogue/dialogue_state.cpp` | C06 (L50) | `camera_correction_gate_harness.exe` | L167 | Inline main assertions/checks (no separate runner scenarios) | `../../src/plugin/camera_correction_gate.hpp`; `../../src/dialogue/dialogue_state.hpp` |
| `tests/plugin/camera_setting_transaction_harness.cpp` | `tests/plugin/camera_setting_transaction_harness.cpp`; `src/dialogue/dialogue_state.cpp` | C07 (L52) | `camera_setting_transaction_harness.exe` | L169 | Inline main assertions/checks (no separate runner scenarios) | `../../src/plugin/camera_writer_transaction.hpp` |
| `tests/plugin/dialogue_boundary_transaction_harness.cpp` | `tests/plugin/dialogue_boundary_transaction_harness.cpp`; `src/dialogue/dialogue_state.cpp`; `src/dialogue/dialogue_fov.cpp` | C08 (L54) | `dialogue_boundary_transaction_harness.exe` | L185 | Inline main assertions/checks (no separate runner scenarios) | `../../src/plugin/dialogue_boundary_transaction.hpp`; `../../src/plugin/camera_writer_transaction.hpp` |
| `tests/plugin/camera_writer_transaction_harness.cpp` | `tests/plugin/camera_writer_transaction_harness.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/cinematics/cinematic_selection.cpp`; `src/cinematics/cinematic_aspect.cpp`; `src/hooks/signature_scanner.cpp`; `src/gameplay/horplus_gameplay.cpp`; `src/gameplay/gameplay_state.cpp`; `src/gameplay/gameplay_camera.cpp`; `src/hooks/instruction_validator.cpp`; `external/safetyhook/Zydis.c`; `src/gameplay/aspect_policy.cpp`; `src/camera/horplus.cpp`; `src/camera/fov_observation.cpp`; `src/camera/gameplay_baseline.cpp`; `src/camera/gameplay_aspect_restoration.cpp`; `src/overlay/presentation/camera_integration.cpp`; `src/dialogue/dialogue_state.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C09 (L56) | `camera_writer_transaction_harness.exe` | L171 | CheckCameraStatePublicationAfterWriterChange; CheckGameplayPolicyUsesValidatedViewportAspect; CheckRuntimeCombineProductionTransaction; CheckWriterOffOrder | `../../src/plugin/camera_writer_transaction.hpp`; `../../src/overlay/presenter/presenter_runtime_policy.hpp`; `../../src/plugin/cinematic_lifecycle_transaction.hpp`; `../../src/camera/camera_state_publication.hpp`; `../../src/gameplay/horplus_gameplay.hpp`; `../../src/gameplay/gameplay_state.hpp`; `../../src/overlay/presentation/camera_integration.hpp` |
| `tests/plugin/cinematic_lifecycle_transaction_harness.cpp` | `tests/plugin/cinematic_lifecycle_transaction_harness.cpp`; `src/cinematics/cinematic_selection.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/cinematics/cinematic_aspect.cpp`; `src/hooks/signature_scanner.cpp`; `src/hooks/instruction_validator.cpp`; `external/safetyhook/Zydis.c`; `src/gameplay/horplus_gameplay.cpp`; `src/gameplay/gameplay_state.cpp`; `src/gameplay/gameplay_camera.cpp`; `src/gameplay/aspect_policy.cpp`; `src/camera/horplus.cpp`; `src/camera/fov_observation.cpp`; `src/camera/gameplay_baseline.cpp`; `src/camera/gameplay_aspect_restoration.cpp`; `src/dialogue/dialogue_state.cpp`; `src/dialogue/dialogue_fov.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C10 (L58) | `cinematic_lifecycle_transaction_harness.exe` | L173 | Inline main assertions/checks (no separate runner scenarios) | `../../src/plugin/cinematic_lifecycle_transaction.hpp`; `../../src/plugin/dialogue_boundary_transaction.hpp`; `../../src/gameplay/horplus_gameplay.hpp`; `../../src/diagnostics/diagnostic_runtime.hpp` |
| `tests/overlay/startup_timeline_harness.cpp` | `tests/overlay/startup_timeline_harness.cpp` | C11 (L60) | `startup_timeline_harness.exe` | L175 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/startup_timeline.hpp` |
| `tests/diagnostics/startup_journal_harness.cpp` | `tests/diagnostics/startup_journal_harness.cpp` | C12 (L62) | `startup_journal_harness.exe` | L177 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/startup_journal.hpp` |
| `tests/config/config_persistence_harness.cpp` | `tests/config/config_persistence_harness.cpp`; `src/config/feature_config.cpp`; `src/config/config_repository.cpp`; `src/config/config_template.cpp` | C13 (L64) | `config_persistence_harness.exe` | L179 | TestCanonicalInitialConfigDocumentation; TestCinematicFovModeConfig; TestCinematicMigrationRoundTrips; TestCommentOwnershipPreservesUserNotes; TestDiagnosticsConfig; TestDialogueMigrationRoundTrips; TestDuplicateManagedOccurrencesConverge; TestHotkeyBindingRegistryRoundTrip; TestHotkeyValidationAndPersistence; TestLegacyDocumentationMigration; TestLegacyOverlayToggleMigration; TestLegacyStyleNormalizationPreservesSettings; TestModifiedLegacyBlockIsPreserved; TestNormalPersistence; TestNormalTemplateSynchronization; TestOverlayEnabledConfig; TestOverlayFontSizeConfig; TestOverlayLocaleCodeConfig; TestReplacementFailurePreservesOriginal; TestStagingFailurePreservesOriginal; TestTemplateRepairPreservesValuesAndIsIdempotent; TestTemplateReplacementFailurePreservesOriginal; TestTemplateStagingFailurePreservesOriginal | `../../src/config/config_repository.hpp`; `../../src/config/config_template.hpp`; `canonical_ini_documentation.hpp` |
| `tests/config/gameplay_mode_harness.cpp` | `tests/config/gameplay_mode_harness.cpp`; `src/config/feature_config.cpp` | C14 (L66) | `gameplay_mode_harness.exe` | L181 | Inline main assertions/checks (no separate runner scenarios) | `../../src/config/feature_config.hpp` |
| `tests/config/runtime_settings_api_harness.cpp` | `tests/config/runtime_settings_api_harness.cpp`; `src/config/feature_config.cpp` | C15 (L68) | `runtime_settings_api_harness.exe` | L183 | Inline main assertions/checks (no separate runner scenarios) | `../../src/plugin/runtime_settings.hpp` |
| `tests/gameplay/aspect_policy_harness.cpp` | `tests/gameplay/aspect_policy_harness.cpp`; `src/gameplay/aspect_policy.cpp` | C16 (L70) | `aspect_policy_harness.exe` | L187 | Inline main assertions/checks (no separate runner scenarios) | `../../../src/gameplay/aspect_policy.hpp` |
| `tests/gameplay/horplus_gameplay_harness.cpp` | `tests/gameplay/horplus_gameplay_harness.cpp`; `src/gameplay/horplus_gameplay.cpp`; `src/gameplay/aspect_policy.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/camera/horplus.cpp`; `src/camera/gameplay_baseline.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C17 (L72) | `horplus_gameplay_harness.exe` | L189 | Inline main assertions/checks (no separate runner scenarios) | `../../src/gameplay/horplus_gameplay.hpp`; `../../src/camera/horplus.hpp`; `../../src/cinematics/cinematic_fov.hpp` |
| `tests/gameplay/zoom_transition_harness.cpp` | `tests/gameplay/zoom_transition_harness.cpp`; `src/gameplay/horplus_gameplay.cpp`; `src/gameplay/aspect_policy.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/camera/horplus.cpp`; `src/camera/gameplay_baseline.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C18 (L74) | `zoom_transition_harness.exe` | L191 | Inline main assertions/checks (no separate runner scenarios) | `../../src/gameplay/horplus_gameplay.hpp` |
| `tests/gameplay/gameplay_mode_transition_harness.cpp` | `tests/gameplay/gameplay_mode_transition_harness.cpp`; `src/gameplay/gameplay_state.cpp`; `src/cinematics/cinematic_selection.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/camera/presentation_state.cpp`; `src/camera/horplus.cpp`; `src/camera/gameplay_baseline.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C19 (L76) | `gameplay_mode_transition_harness.exe` | L193 | Inline main assertions/checks (no separate runner scenarios) | `../../src/gameplay/gameplay_state.hpp`; `../../src/camera/horplus.hpp`; `../../src/cinematics/cinematic_selection.hpp`; `../../src/cinematics/cinematic_fov.hpp`; `../../src/camera/fov_semantics.hpp` |
| `tests/regression/module_ownership_harness.cpp` | `tests/regression/module_ownership_harness.cpp`; `src/gameplay/gameplay_state.cpp`; `src/cinematics/cinematic_selection.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/dialogue/dialogue_fov.cpp`; `src/dialogue/dialogue_state.cpp`; `src/camera/presentation_state.cpp`; `src/camera/horplus.cpp`; `src/camera/gameplay_baseline.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C20 (L78) | `module_ownership_harness.exe` | L195 | Inline main assertions/checks (no separate runner scenarios) | `../../src/camera/horplus.hpp`; `../../src/camera/presentation_state.hpp`; `../../src/cinematics/cinematic_fov.hpp`; `../../src/cinematics/cinematic_selection.hpp`; `../../src/dialogue/dialogue_fov.hpp`; `../../src/dialogue/dialogue_state.hpp`; `../../src/gameplay/gameplay_state.hpp` |
| `tests/gameplay/gameplay_camera_safety_harness.cpp` | `tests/gameplay/gameplay_camera_safety_harness.cpp`; `src/gameplay/gameplay_camera.cpp`; `src/hooks/instruction_validator.cpp`; `external/safetyhook/Zydis.c` | C21 (L80) | `gameplay_camera_safety_harness.exe` | L197 | Inline main assertions/checks (no separate runner scenarios) | `../../src/gameplay/gameplay_camera.hpp` |
| `tests/platform/memory_harness.cpp` | `tests/platform/memory_harness.cpp`; `src/platform/win32/memory.cpp` | C22 (L82) | `memory_harness.exe` | L199 | TestExecuteOnlyPageIsRefused; TestGuardPageIsRefused; TestNoAccessPageIsRefused; TestReadablePage; TestWritableProtectionContracts | `../../src/platform/win32/memory.hpp` |
| `tests/platform/viewport_harness.cpp` | `tests/platform/viewport_harness.cpp`; `src/platform/win32/viewport.cpp`; `src/platform/win32/window.cpp` | C23 (L84) | `viewport_harness.exe` | L201 | Inline main assertions/checks (no separate runner scenarios) | `../../src/platform/win32/viewport.hpp` |
| `tests/platform/signature_scanner_harness.cpp` | `tests/platform/signature_scanner_harness.cpp`; `src/hooks/signature_scanner.cpp` | C24 (L86) | `signature_scanner_harness.exe` | L203 | CheckEnterWildcards; CheckMalformedInputTerminates; CheckMixedTokens; CheckPattern | `../../src/hooks/signature_scanner.hpp`; `../../src/hooks/signatures/signature_definitions.hpp` |
| `tests/platform/instruction_validator_harness.cpp` | `tests/platform/instruction_validator_harness.cpp`; `src/hooks/instruction_validator.cpp`; `external/safetyhook/Zydis.c` | C25 (L88) | `instruction_validator_harness.exe` | L205 | Inline main assertions/checks (no separate runner scenarios) | `../../src/hooks/instruction_validator.hpp` |
| `tests/cinematics/cinematic_initialization_harness.cpp` | `tests/cinematics/cinematic_initialization_harness.cpp`; `src/cinematics/cinematic_initialization.cpp`; `src/plugin/feature_status.cpp` | C26 (L90) | `cinematic_initialization_harness.exe` | L207 | CheckAspectFailure; CheckCommitFailure; CheckFovFailure; CheckNativeBypass; CheckSuccess | `../../src/cinematics/cinematic_initialization.hpp` |
| `tests/cinematics/coordinator_recovery_harness.cpp` | `tests/cinematics/coordinator_recovery_harness.cpp`; `src/gameplay/gameplay_state.cpp`; `src/camera/presentation_state.cpp` | C27 (L92) | `coordinator_recovery_harness.exe` | L209 | CheckGameplayAvailable; CheckGameplayDisabled; CheckGameplayInitializationFailure; CheckHorPlusMode; CheckHorPlusRecoveryActions | `../../src/gameplay/gameplay_state.hpp` |
| `tests/cinematics/cinematic_exit_authority_harness.cpp` | `tests/cinematics/cinematic_exit_authority_harness.cpp`; `src/gameplay/horplus_gameplay.cpp`; `src/gameplay/aspect_policy.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/camera/horplus.cpp`; `src/camera/gameplay_baseline.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C28 (L94) | `cinematic_exit_authority_harness.exe` | L211 | CheckAcceptedNoAssociationStillFailsClosed; CheckDuplicateExitSequence; CheckRapidExitEnterRemainsValid; CheckRejectedExitIsObservationOnly | `../../src/cinematics/cinematic_exit_authority.hpp`; `../../src/gameplay/horplus_gameplay.hpp` |
| `tests/cinematics/horplus_post_exit_recovery_harness.cpp` | `tests/cinematics/horplus_post_exit_recovery_harness.cpp`; `src/gameplay/gameplay_state.cpp`; `src/camera/presentation_state.cpp`; `src/gameplay/horplus_gameplay.cpp`; `src/gameplay/aspect_policy.cpp`; `src/camera/horplus.cpp`; `src/camera/gameplay_baseline.cpp`; `src/camera/fov_observation.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/dialogue/dialogue_state.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C29 (L96) | `horplus_post_exit_recovery_harness.exe` | L213 | CheckAlreadyNativeRecovery; CheckCompletedRecoveryReleasesGameplayWriterGate; CheckGenerationScopedRecoveryOwner; CheckRecoveryEvidenceGates; CheckWriterChangeRetiresRecoveryBeforeGameplayDispatch; TestRecordedRecoveryInterpolationRemainsInHorPlusSpace | `../../src/camera/gameplay_baseline.hpp`; `../../src/camera/horplus.hpp`; `../../src/cinematics/cinematic_fov.hpp`; `../../src/cinematics/cinematic_recovery_retirement.hpp`; `../../src/dialogue/dialogue_state.hpp`; `../../src/gameplay/gameplay_state.hpp`; `../../src/gameplay/horplus_gameplay.hpp` |
| `tests/cinematics/cinematic_selection_harness.cpp` | `tests/cinematics/cinematic_selection_harness.cpp`; `src/cinematics/cinematic_selection.cpp` | C30 (L98) | `cinematic_selection_harness.exe` | L215 | Inline main assertions/checks (no separate runner scenarios) | `../../src/cinematics/cinematic_selection.hpp` |
| `tests/dialogue/post_cinematic_exclusion_harness.cpp` | `tests/dialogue/post_cinematic_exclusion_harness.cpp`; `src/dialogue/dialogue_state.cpp` | C31 (L100) | `post_cinematic_exclusion_harness.exe` | L217 | Inline main assertions/checks (no separate runner scenarios) | `../../src/dialogue/dialogue_state.hpp` |
| `tests/dialogue/recovery_rearm_harness.cpp` | `tests/dialogue/recovery_rearm_harness.cpp`; `src/dialogue/dialogue_state.cpp` | C32 (L102) | `recovery_rearm_harness.exe` | L219 | Inline main assertions/checks (no separate runner scenarios) | `../../src/dialogue/dialogue_state.hpp`; `../../src/camera/fov_semantics.hpp` |
| `tests/dialogue/candidate_hardening_harness.cpp` | `tests/dialogue/candidate_hardening_harness.cpp`; `src/dialogue/dialogue_state.cpp` | C33 (L104) | `candidate_hardening_harness.exe` | L221 | Inline main assertions/checks (no separate runner scenarios) | `../../src/dialogue/dialogue_state.hpp` |
| `tests/dialogue/policy_snapshot_harness.cpp` | `tests/dialogue/policy_snapshot_harness.cpp`; `src/dialogue/dialogue_state.cpp` | C34 (L106) | `policy_snapshot_harness.exe` | L223 | Inline main assertions/checks (no separate runner scenarios) | `../../src/dialogue/dialogue_state.hpp` |
| `tests/camera/camera_state_snapshot_harness.cpp` | `tests/camera/camera_state_snapshot_harness.cpp`; `src/camera/camera_state_snapshot.cpp`; `src/camera/gameplay_baseline.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C35 (L108) | `camera_state_snapshot_harness.exe` | L225 | Inline main assertions/checks (no separate runner scenarios) | `../../src/camera/camera_state_snapshot.hpp`; `../../src/camera/camera_state_publication.hpp`; `../../src/camera/gameplay_baseline.hpp` |
| `tests/camera/fov_observation_harness.cpp` | `tests/camera/fov_observation_harness.cpp`; `src/camera/fov_observation.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C36 (L110) | `fov_observation_harness.exe` | L227 | Inline main assertions/checks (no separate runner scenarios) | `../../src/camera/fov_observation.hpp` |
| `tests/camera/gameplay_baseline_harness.cpp` | `tests/camera/gameplay_baseline_harness.cpp`; `src/camera/fov_observation.cpp`; `src/camera/gameplay_baseline.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C37 (L112) | `gameplay_baseline_harness.exe` | L229 | Inline main assertions/checks (no separate runner scenarios) | `../../src/camera/gameplay_baseline.hpp` |
| `tests/camera/gameplay_aspect_restoration_harness.cpp` | `tests/camera/gameplay_aspect_restoration_harness.cpp`; `src/camera/fov_observation.cpp`; `src/camera/gameplay_aspect_restoration.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C38 (L114) | `gameplay_aspect_restoration_harness.exe` | L231 | Inline main assertions/checks (no separate runner scenarios) | `../../src/camera/gameplay_aspect_restoration.hpp` |
| `tests/cinematics/cinematic_fov_harness.cpp` | `tests/cinematics/cinematic_fov_harness.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/camera/horplus.cpp`; `src/camera/fov_observation.cpp`; `src/camera/gameplay_baseline.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp`; `src/diagnostics/cinematic_continuation_trace.cpp` | C39 (L116) | `cinematic_fov_harness.exe` | L233 | Inline main assertions/checks (no separate runner scenarios) | `../../src/cinematics/cinematic_fov.hpp`; `../../src/diagnostics/cinematic_continuation_trace.hpp`; `../../src/diagnostics/cinematic_baseline_provenance.hpp` |
| `tests/cinematics/cinematic_aspect_harness.cpp` | `tests/cinematics/cinematic_aspect_harness.cpp`; `src/cinematics/cinematic_aspect.cpp`; `src/hooks/signature_scanner.cpp`; `src/hooks/instruction_validator.cpp`; `external/safetyhook/Zydis.c` | C40 (L118) | `cinematic_aspect_harness.exe` | L235 | Inline main assertions/checks (no separate runner scenarios) | `../../src/cinematics/cinematic_aspect.hpp` |
| `tests/dialogue/dialogue_fov_harness.cpp` | `tests/dialogue/dialogue_fov_harness.cpp`; `src/dialogue/dialogue_fov.cpp` | C41 (L120) | `dialogue_fov_harness.exe` | L237 | Inline main assertions/checks (no separate runner scenarios) | `../../src/dialogue/dialogue_fov.hpp` |
| `tests/diagnostics/diagnostics_gate_harness.cpp` | `tests/diagnostics/diagnostics_gate_harness.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C42 (L122) | `diagnostics_gate_harness.exe` | L239 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/diagnostic_runtime.hpp`; `../../src/diagnostics/camera_log_probe.hpp` |
| `tests/diagnostics/dialogue_source_provenance_harness.cpp` | `tests/diagnostics/dialogue_source_provenance_harness.cpp`; `src/diagnostics/dialogue_source_provenance.cpp` | C43 (L124) | `dialogue_source_provenance_harness.exe` | L241 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/dialogue_source_provenance.hpp` |
| `tests/diagnostics/camera_writer_identity_harness.cpp` | `tests/diagnostics/camera_writer_identity_harness.cpp` | C44 (L126) | `camera_writer_identity_harness.exe` | L243 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/camera_writer_identity.hpp` |
| `tests/diagnostics/cinematic_lifecycle_context_harness.cpp` | `tests/diagnostics/cinematic_lifecycle_context_harness.cpp` | C45 (L128) | `cinematic_lifecycle_context_harness.exe` | L245 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/cinematic_lifecycle_context.hpp`; `../../src/cinematics/cinematic_exit_authority.hpp` |
| `tests/diagnostics/performance_telemetry_harness.cpp` | `tests/diagnostics/performance_telemetry_harness.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C46 (L130) | `performance_telemetry_harness.exe` | L247 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/diagnostic_runtime.hpp`; `../../src/diagnostics/performance_telemetry.hpp` |
| `tests/diagnostics/language_state_transition_harness.cpp` | `tests/diagnostics/language_state_transition_harness.cpp`; `src/diagnostics/language_state/language_state_transition.cpp` | C47 (L132) | `language_state_transition_harness.exe` | L249 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/language_state/language_state_transition.hpp` |
| `tests/diagnostics/matchgameplay_prediction_harness.cpp` | `tests/diagnostics/matchgameplay_prediction_harness.cpp` | C48 (L134) | `matchgameplay_prediction_harness.exe` | L251 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/matchgameplay_prediction.hpp` |
| `tests/regression/runtime_evidence_replay_harness.cpp` | `tests/regression/runtime_evidence_replay_harness.cpp`; `src/gameplay/horplus_gameplay.cpp`; `src/gameplay/aspect_policy.cpp`; `src/gameplay/gameplay_state.cpp`; `src/camera/presentation_state.cpp`; `src/cinematics/cinematic_fov.cpp`; `src/camera/horplus.cpp`; `src/camera/fov_observation.cpp`; `src/camera/gameplay_baseline.cpp`; `src/camera/gameplay_aspect_restoration.cpp`; `src/dialogue/dialogue_state.cpp`; `src/diagnostics/performance_telemetry.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C49 (L136) | `runtime_evidence_replay_harness.exe` | L253 | Inline main assertions/checks (no separate runner scenarios) | `../fixtures/runtime/gameplay/horplus_evidence.hpp`; `../fixtures/runtime/transitions/restoration_evidence.hpp`; `../fixtures/runtime/cinematics/cinematic_evidence.hpp`; `../fixtures/runtime/dialogue/candidate_evidence.hpp`; `../../src/camera/gameplay_aspect_restoration.hpp`; `../../src/camera/gameplay_baseline.hpp`; `../../src/cinematics/cinematic_fov.hpp`; `../../src/dialogue/dialogue_state.hpp`; `../../src/gameplay/horplus_gameplay.hpp`; `../../src/gameplay/gameplay_state.hpp` |
| `tests/gameplay/gameplay_camera_resolver_harness.cpp` | `tests/gameplay/gameplay_camera_resolver_harness.cpp`; `src/gameplay/gameplay_camera.cpp`; `src/hooks/instruction_validator.cpp`; `external/safetyhook/Zydis.c` | C50 (L138) | `gameplay_camera_resolver_harness.exe` | L255 | Inline main assertions/checks (no separate runner scenarios) | `../../src/gameplay/gameplay_camera.hpp`; `../../src/hooks/signatures/signature_definitions.hpp` |
| `tests/overlay/composition_presenter_state_harness.cpp` | `tests/overlay/composition_presenter_state_harness.cpp`; `src/overlay/presenter/composition_presenter_state.cpp`; `src/overlay/presenter/renderer_state.cpp`; `src/overlay/input/input_state.cpp`; `src/config/feature_config.cpp` | C51 (L140) | `composition_presenter_state_harness.exe` | L257 | TestCameraStatePublicationSchedulesVisibleRefresh; TestCommittedSurfaceRetiresOldResourceOnlyAfterCommit; TestCoreAndWindowReadinessAreIndependent; TestDamageTrackingPreservesRetainedSurfacePixels; TestDeviceLossRecoveryIsBounded; TestDeviceRecoveryPreservesVisibleAndHiddenInputOwnership; TestExplicitDpiCoordinateAndFontContract; TestGenerationPublicationAndResizeRetirement; TestInputEventsScheduleOneOwnerWake; TestInputLifecycleAndNotificationRedraw; TestMinimizeRestoreKeepsGeneration; TestProductionPresenterRoutingAcrossRestore; TestSemanticPresenterPacingStates; TestStartupHintPublicationRequiresPresenterAndSettings; TestStartupHintWaitsForAcceptedAutoLocaleSync; TestTimerIdentityDrivesAutonomousProgression; TestToastLifetimeRequiresSubmittedVisibleGeneration | `../../src/overlay/presenter/composition_presenter_state.hpp`; `../../src/overlay/presenter/renderer_state.hpp`; `../../src/overlay/presenter/presenter_runtime_policy.hpp`; `../../src/overlay/input/input_state.hpp`; `../../src/overlay/presentation/presentation_contracts.hpp` |
| `tests/overlay/activation_trace_harness.cpp` | `tests/overlay/activation_trace_harness.cpp`; `src/diagnostics/diagnostic_runtime.cpp` | C52 (L142) | `activation_trace_harness.exe` | L259 | Inline main assertions/checks (no separate runner scenarios) | `../../src/diagnostics/diagnostic_runtime.hpp`; `../../src/diagnostics/overlay_activation_trace.hpp` |
| `tests/overlay/overlay_layout_metrics_harness.cpp` | `tests/overlay/overlay_layout_metrics_harness.cpp`; `src/overlay/presentation/overlay_layout_metrics.cpp` | C53 (L144) | `overlay_layout_metrics_harness.exe` | L261 | Inline main assertions/checks (no separate runner scenarios) | `../../src/overlay/presentation/overlay_layout_metrics.hpp` |
| `tests/overlay/selector_tooltip_state_harness.cpp` | `tests/overlay/selector_tooltip_state_harness.cpp` | C54 (L146) | `selector_tooltip_state_harness.exe` | L263 | Inline main assertions/checks (no separate runner scenarios) | `../../src/overlay/presentation/selector_tooltip_state.hpp` |
| `tests/overlay/input_state_harness.cpp` | `tests/overlay/input_state_harness.cpp`; `src/overlay/input/input_state.cpp`; `src/config/feature_config.cpp` | C55 (L148) | `input_state_harness.exe` | L265 | Inline main assertions/checks (no separate runner scenarios) | `../../src/overlay/input/input_state.hpp`; `../../src/overlay/presentation/hotkey_binding_presentation.hpp` |
| `tests/overlay/camera_integration_harness.cpp` | `tests/overlay/camera_integration_harness.cpp`; `src/overlay/presentation/camera_integration.cpp`; `src/camera/horplus.cpp` | C56 (L150) | `camera_integration_harness.exe` | L267 | Inline main assertions/checks (no separate runner scenarios) | `../../src/overlay/presentation/camera_integration.hpp`; `../../src/overlay/presentation/aspect_ratio_format.hpp` |
| `tests/overlay/placement_config_harness.cpp` | `tests/overlay/placement_config_harness.cpp`; `src/overlay/presentation/placement_config.cpp`; `src/config/config_repository.cpp`; `src/config/feature_config.cpp`; `src/config/config_template.cpp` | C57 (L152) | `placement_config_harness.exe` | L269 | TestConcurrentSettingAndPlacementWrites; TestFailureLeavesExistingConfigUntouched; TestLegacyMigrationAndFollowingSettingWrite; TestLoadAndSave; TestPlacementSaveDisposition | `../../src/config/config_repository.hpp`; `../../src/overlay/presentation/placement_config.hpp`; `../../src/overlay/presenter/placement_save_state.hpp` |
| `tests/overlay/feature_presentation_harness.cpp` | `tests/overlay/feature_presentation_harness.cpp`; `src/overlay/presentation/feature_presentation.cpp`; `src/config/feature_config.cpp` | C58 (L154) | `feature_presentation_harness.exe` | L271 | Inline main assertions/checks (no separate runner scenarios) | `../../src/overlay/presentation/feature_presentation.hpp` |
| `tests/overlay/semantic_snapshot_harness.cpp` | `tests/overlay/semantic_snapshot_harness.cpp` | C59 (L156) | `semantic_snapshot_harness.exe` | L273 | Inline main assertions/checks (no separate runner scenarios) | `../../src/plugin/runtime_settings.hpp` |
| `tests/overlay/localization_harness.cpp` | `tests/overlay/localization_harness.cpp`; `src/overlay/localization/localization_catalog.cpp`; `src/overlay/localization/localization_formatter.cpp`; `src/overlay/localization/localization_validator.cpp`; `src/overlay/localization/localization_manager.cpp`; `src/overlay/localization/localization_font.cpp`; `src/overlay/localization/localization_keys.cpp`; `external/imgui/imgui.cpp`; `external/imgui/imgui_draw.cpp`; `external/imgui/imgui_tables.cpp`; `external/imgui/imgui_widgets.cpp` | C60 (L158) | `localization_harness.exe` | L275 | Inline main assertions/checks (no separate runner scenarios) | `../../src/overlay/localization/localization_catalog.hpp`; `../../src/overlay/localization/localization_formatter.hpp`; `../../src/overlay/localization/localization_manager.hpp`; `../../src/overlay/localization/localization_validator.hpp`; `../../src/overlay/localization/localization_font.hpp`; `../../src/overlay/presentation/inline_hotkey_layout.hpp`; `../../src/overlay/localization/game_language_reader.hpp`; `../../src/overlay/presentation/setting_tooltip_content.hpp`; `../../src/overlay/presentation/selector_examples_geometry.hpp`; `../../src/config/feature_config.hpp`; `canonical_ini_documentation.hpp` |

### ASET — registry-derived dependencies

- `src/plugin/runtime_settings.cpp`
- `src/plugin/worker_lifecycle.cpp`
- `src/plugin/feature_status.cpp`
- `src/config/feature_config.cpp`
- `src/config/config_repository.cpp`
- `src/config/config_template.cpp`
- `src/diagnostics/diagnostic_runtime.cpp`
- `src/diagnostics/cinematic_continuation_trace.cpp`
- `src/diagnostics/dialogue_source_provenance.cpp`
- `src/diagnostics/performance_telemetry.cpp`
- `src/hooks/signature_scanner.cpp`
- `src/hooks/instruction_validator.cpp`
- `src/hooks/hook_set.cpp`
- `src/gameplay/gameplay_state.cpp`
- `src/gameplay/gameplay_camera.cpp`
- `src/gameplay/aspect_policy.cpp`
- `src/gameplay/horplus_gameplay.cpp`
- `src/cinematics/cinematic_fov.cpp`
- `src/cinematics/cinematic_selection.cpp`
- `src/cinematics/cinematic_aspect.cpp`
- `src/cinematics/cinematic_initialization.cpp`
- `src/dialogue/dialogue_fov.cpp`
- `src/dialogue/dialogue_state.cpp`
- `src/camera/camera_state_snapshot.cpp`
- `src/camera/fov_observation.cpp`
- `src/camera/gameplay_baseline.cpp`
- `src/camera/gameplay_aspect_restoration.cpp`
- `src/camera/presentation_state.cpp`
- `src/camera/horplus.cpp`
- `src/platform/win32/memory.cpp`
- `src/platform/win32/sha256.cpp`
- `src/platform/win32/window.cpp`
- `src/platform/win32/viewport.cpp`
- `src/overlay/presentation/camera_integration.cpp`
- `src/overlay/presentation/feature_presentation.cpp`
- `src/overlay/presentation/camera_state_view.cpp`
- `src/overlay/presentation/selector_documentation_view.cpp`
- `src/overlay/presentation/overlay_layout_metrics.cpp`
- `src/overlay/presentation/placement_config.cpp`
- `src/overlay/localization/localization_catalog.cpp`
- `src/overlay/localization/localization_formatter.cpp`
- `src/overlay/localization/localization_validator.cpp`
- `src/overlay/localization/localization_manager.cpp`
- `src/overlay/localization/localization_font.cpp`
- `src/overlay/localization/localization_keys.cpp`
- `src/overlay/localization/game_language_reader.cpp`
- `src/overlay/composition/composition_runtime.cpp`
- `src/overlay/presenter/composition_presenter_state.cpp`
- `src/overlay/input/input_state.cpp`
- `src/overlay/presenter/renderer_state.cpp`
- `src/overlay/presenter/renderer_runtime.cpp`
- `src/overlay/presenter/imgui_d3d11_renderer.cpp`
- `external/safetyhook/safetyhook.cpp`
- `external/safetyhook/Zydis.c`
- `external/imgui/imgui.cpp`
- `external/imgui/imgui_draw.cpp`
- `external/imgui/imgui_tables.cpp`
- `external/imgui/imgui_widgets.cpp`
- `external/imgui/backends/imgui_impl_win32.cpp`

### Native matrix case inventory

Each case below runs in its own child process twice per matrix parent (OFF/ON). The canonical suite launches the matrix parent twice.

- `writer-wide`
- `writer-native`
- `writer-combine`
- `writer-combine-readonly`
- `writer-combine-invalid-viewport`
- `writer-aspect`
- `writer-off`
- `writer-gate-off`
- `writer-stopping`
- `writer-unreadable`
- `aspect-wide`
- `aspect-off`
- `aspect-readonly`
- `aspect-unreadable`
- `lifecycle`
- `lifecycle-off`
- `lifecycle-gate-off`
- `lifecycle-chain`
- `dialogue-active`
- `dialogue-off`
- `dialogue-cinematic`
- `dialogue-unreadable`
- `zoom`
- `zoom-unreadable`
- `zoom-in-throw`
- `zoom-out-throw`
- `settings-wake`
- `serialization`
- `writer-invalid-fov`
- `writer-combine-readback-failure`
- `historical-target`
- `serialization-dialogue`
- `serialization-enter`
- `serialization-exit`
- `serialization-aspect`
- `serialization-settings`
- `serialization-reader`
- `recovery`
- `settings-api`
- `dialogue-exiting`
- `dialogue-rearm-source-change`
- `dialogue-disabled-in-flight`
- `lifecycle-foreign-association`
- `recovery-foreign-source`
- `cinematic-initial-gameplay-baseline`
- `cinematic-initial-foreign-source`
- `cinematic-initial-stale-generation`
- `cinematic-initial-missing-baseline`
- `cinematic-initial-invalid-baseline`
- `restoration-logging`
- `cinematic-continuation-baseline`
- `cinematic-continuation-combine`
- `cinematic-continuation-chain`
- `cinematic-continuation-foreign-source`
- `cinematic-continuation-foreign-writer`
- `cinematic-continuation-stale-generation`
- `cinematic-continuation-no-association`
- `cinematic-continuation-invalid-baseline`
- `cinematic-continuation-native-mode`
- `cinematic-continuation-target-change`
- `cinematic-continuation-outside-low`
- `cinematic-continuation-outside-high`
- `cinematic-continuation-correction-off`
- `cinematic-continuation-exit-departure`
- `cinematic-continuation-no-validated-exit`
- `cinematic-continuation-policy-change`
- `evidence-dialogue-identities`
- `evidence-dialogue-invalid-fov`
- `evidence-dialogue-invalid-target`
- `evidence-dialogue-source-change`
- `evidence-dialogue-reset`
- `evidence-dialogue-policy-snapshot`
- `evidence-dialogue-reenable`
- `evidence-dialogue-exclusion`
- `evidence-writer-identities`
- `evidence-writer-invalid-fields`
- `evidence-writer-ads`
- `evidence-runtime-resize`
- `evidence-cinematic-invalid-enter`
- `evidence-cinematic-rejected-exit`
- `evidence-cinematic-indexed-exit`
- `evidence-cinematic-cancel`
- `evidence-cinematic-aspect-targets`
- `evidence-settings-kinds`
- `evidence-settings-lock-order`
- `evidence-persistence-kinds`
- `evidence-reset-empty`
- `evidence-exception-boundary`
- `evidence-nested-deferral`
- `evidence-native-trampoline`
- `evidence-zoom-native-continuation`
- `evidence-safetyhook-lifetime-faults`
- `evidence-safetyhook-failed-create`
- `evidence-safetyhook-disable-postwrite`
- `evidence-reset-self-join`
- `evidence-reset-native-hook`
- `evidence-install-transaction-order`
- `diagnostic-work`
- `startup-fatal-early`
- `startup-fatal-partial`
- `startup-fatal-cleanup-throws`
- `startup-fatal-retained-worker`

## Appendix B — exact canonical compiler invocations

COMMON is expanded below. Native RSP contents are ASET above; generated-header INCLUDE and VS environment are described above. These commands are the originals, not the timing-only replay additions.

### C01 — test.cmd:38

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui /DNDEBUG /DSAFETYHOOK_TEST_VIRTUAL_PROTECT /DOVERLAY_PRODUCTION /DOVERLAY_SETTINGS_FRONTEND /DOVERLAY_COMBINED /DOVERLAY_STARTUP_JOURNAL /Iexternal\imgui\backends tests\plugin\runtime_native_adapter_harness.cpp tests\plugin\runtime_owner_binding_probe.cpp @build-artifacts\tests\runtime_adapter_sources.rsp /link user32.lib psapi.lib bcrypt.lib dxgi.lib d3d11.lib dcomp.lib d3dcompiler.lib build-artifacts\tests\localization_resources.res /OUT:build-artifacts\tests\runtime_native_adapter_harness.exe
```

### C02 — test.cmd:41

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui /DNDEBUG /DSAFETYHOOK_TEST_VIRTUAL_PROTECT /DOVERLAY_PRODUCTION /DOVERLAY_SETTINGS_FRONTEND /DOVERLAY_COMBINED /DOVERLAY_STARTUP_JOURNAL /Iexternal\imgui\backends tests\plugin\n06_m01_zoom_game_derived_harness.cpp tests\plugin\runtime_owner_binding_probe.cpp @build-artifacts\tests\runtime_adapter_sources.rsp /link user32.lib psapi.lib bcrypt.lib dxgi.lib d3d11.lib dcomp.lib d3dcompiler.lib build-artifacts\tests\localization_resources.res /OUT:build-artifacts\tests\n06_m01_zoom_game_derived_harness.exe
```

### C03 — test.cmd:44

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\lifecycle\worker_lifecycle_harness.cpp src\plugin\worker_lifecycle.cpp /link user32.lib /OUT:build-artifacts\tests\worker_lifecycle_harness.exe
```

### C04 — test.cmd:46

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\feature_status\feature_status_harness.cpp src\plugin\feature_status.cpp /link /OUT:build-artifacts\tests\feature_status_harness.exe
```

### C05 — test.cmd:48

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\plugin\optional_overlay_startup_harness.cpp /link /OUT:build-artifacts\tests\optional_overlay_startup_harness.exe
```

### C06 — test.cmd:50

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\plugin\camera_correction_gate_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\camera_correction_gate_harness.exe
```

### C07 — test.cmd:52

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\plugin\camera_setting_transaction_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\camera_setting_transaction_harness.exe
```

### C08 — test.cmd:54

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\plugin\dialogue_boundary_transaction_harness.cpp src\dialogue\dialogue_state.cpp src\dialogue\dialogue_fov.cpp /link /OUT:build-artifacts\tests\dialogue_boundary_transaction_harness.exe
```

### C09 — test.cmd:56

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\plugin\camera_writer_transaction_harness.cpp src\cinematics\cinematic_fov.cpp src\cinematics\cinematic_selection.cpp src\cinematics\cinematic_aspect.cpp src\hooks\signature_scanner.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\gameplay_state.cpp src\gameplay\gameplay_camera.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c src\gameplay\aspect_policy.cpp src\camera\horplus.cpp src\camera\fov_observation.cpp src\camera\gameplay_baseline.cpp src\camera\gameplay_aspect_restoration.cpp src\overlay\presentation\camera_integration.cpp src\dialogue\dialogue_state.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\camera_writer_transaction_harness.exe
```

### C10 — test.cmd:58

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\plugin\cinematic_lifecycle_transaction_harness.cpp src\cinematics\cinematic_selection.cpp src\cinematics\cinematic_fov.cpp src\cinematics\cinematic_aspect.cpp src\hooks\signature_scanner.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c src\gameplay\horplus_gameplay.cpp src\gameplay\gameplay_state.cpp src\gameplay\gameplay_camera.cpp src\gameplay\aspect_policy.cpp src\camera\horplus.cpp src\camera\fov_observation.cpp src\camera\gameplay_baseline.cpp src\camera\gameplay_aspect_restoration.cpp src\dialogue\dialogue_state.cpp src\dialogue\dialogue_fov.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\cinematic_lifecycle_transaction_harness.exe
```

### C11 — test.cmd:60

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\startup_timeline_harness.cpp /link psapi.lib /OUT:build-artifacts\tests\startup_timeline_harness.exe
```

### C12 — test.cmd:62

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui /DOVERLAY_STARTUP_JOURNAL tests\diagnostics\startup_journal_harness.cpp /link /OUT:build-artifacts\tests\startup_journal_harness.exe
```

### C13 — test.cmd:64

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\config\config_persistence_harness.cpp src\config\feature_config.cpp src\config\config_repository.cpp src\config\config_template.cpp /link user32.lib /OUT:build-artifacts\tests\config_persistence_harness.exe
```

### C14 — test.cmd:66

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\config\gameplay_mode_harness.cpp src\config\feature_config.cpp /link user32.lib /OUT:build-artifacts\tests\gameplay_mode_harness.exe
```

### C15 — test.cmd:68

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\config\runtime_settings_api_harness.cpp src\config\feature_config.cpp /link user32.lib /OUT:build-artifacts\tests\runtime_settings_api_harness.exe
```

### C16 — test.cmd:70

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\gameplay\aspect_policy_harness.cpp src\gameplay\aspect_policy.cpp /link /OUT:build-artifacts\tests\aspect_policy_harness.exe
```

### C17 — test.cmd:72

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\gameplay\horplus_gameplay_harness.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\aspect_policy.cpp src\cinematics\cinematic_fov.cpp src\camera\horplus.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\horplus_gameplay_harness.exe
```

### C18 — test.cmd:74

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\gameplay\zoom_transition_harness.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\aspect_policy.cpp src\cinematics\cinematic_fov.cpp src\camera\horplus.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\zoom_transition_harness.exe
```

### C19 — test.cmd:76

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\gameplay\gameplay_mode_transition_harness.cpp src\gameplay\gameplay_state.cpp src\cinematics\cinematic_selection.cpp src\cinematics\cinematic_fov.cpp src\camera\presentation_state.cpp src\camera\horplus.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\gameplay_mode_transition_harness.exe
```

### C20 — test.cmd:78

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\regression\module_ownership_harness.cpp src\gameplay\gameplay_state.cpp src\cinematics\cinematic_selection.cpp src\cinematics\cinematic_fov.cpp src\dialogue\dialogue_fov.cpp src\dialogue\dialogue_state.cpp src\camera\presentation_state.cpp src\camera\horplus.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\module_ownership_harness.exe
```

### C21 — test.cmd:80

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\gameplay\gameplay_camera_safety_harness.cpp src\gameplay\gameplay_camera.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c /link /OUT:build-artifacts\tests\gameplay_camera_safety_harness.exe
```

### C22 — test.cmd:82

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\platform\memory_harness.cpp src\platform\win32\memory.cpp /link /OUT:build-artifacts\tests\memory_harness.exe
```

### C23 — test.cmd:84

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\platform\viewport_harness.cpp src\platform\win32\viewport.cpp src\platform\win32\window.cpp /link user32.lib /OUT:build-artifacts\tests\viewport_harness.exe
```

### C24 — test.cmd:86

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui /DSIGNATURE_SCANNER_TEST tests\platform\signature_scanner_harness.cpp src\hooks\signature_scanner.cpp /link /OUT:build-artifacts\tests\signature_scanner_harness.exe
```

### C25 — test.cmd:88

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\platform\instruction_validator_harness.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c /link /OUT:build-artifacts\tests\instruction_validator_harness.exe
```

### C26 — test.cmd:90

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\cinematics\cinematic_initialization_harness.cpp src\cinematics\cinematic_initialization.cpp src\plugin\feature_status.cpp /link /OUT:build-artifacts\tests\cinematic_initialization_harness.exe
```

### C27 — test.cmd:92

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\cinematics\coordinator_recovery_harness.cpp src\gameplay\gameplay_state.cpp src\camera\presentation_state.cpp /link /OUT:build-artifacts\tests\coordinator_recovery_harness.exe
```

### C28 — test.cmd:94

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\cinematics\cinematic_exit_authority_harness.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\aspect_policy.cpp src\cinematics\cinematic_fov.cpp src\camera\horplus.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\cinematic_exit_authority_harness.exe
```

### C29 — test.cmd:96

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\cinematics\horplus_post_exit_recovery_harness.cpp src\gameplay\gameplay_state.cpp src\camera\presentation_state.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\aspect_policy.cpp src\camera\horplus.cpp src\camera\gameplay_baseline.cpp src\camera\fov_observation.cpp src\cinematics\cinematic_fov.cpp src\dialogue\dialogue_state.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\horplus_post_exit_recovery_harness.exe
```

### C30 — test.cmd:98

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\cinematics\cinematic_selection_harness.cpp src\cinematics\cinematic_selection.cpp /link /OUT:build-artifacts\tests\cinematic_selection_harness.exe
```

### C31 — test.cmd:100

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\dialogue\post_cinematic_exclusion_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\post_cinematic_exclusion_harness.exe
```

### C32 — test.cmd:102

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\dialogue\recovery_rearm_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\recovery_rearm_harness.exe
```

### C33 — test.cmd:104

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\dialogue\candidate_hardening_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\candidate_hardening_harness.exe
```

### C34 — test.cmd:106

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\dialogue\policy_snapshot_harness.cpp src\dialogue\dialogue_state.cpp /link /OUT:build-artifacts\tests\policy_snapshot_harness.exe
```

### C35 — test.cmd:108

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\camera\camera_state_snapshot_harness.cpp src\camera\camera_state_snapshot.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\camera_state_snapshot_harness.exe
```

### C36 — test.cmd:110

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\camera\fov_observation_harness.cpp src\camera\fov_observation.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\fov_observation_harness.exe
```

### C37 — test.cmd:112

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\camera\gameplay_baseline_harness.cpp src\camera\fov_observation.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\gameplay_baseline_harness.exe
```

### C38 — test.cmd:114

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\camera\gameplay_aspect_restoration_harness.cpp src\camera\fov_observation.cpp src\camera\gameplay_aspect_restoration.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\gameplay_aspect_restoration_harness.exe
```

### C39 — test.cmd:116

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\cinematics\cinematic_fov_harness.cpp src\cinematics\cinematic_fov.cpp src\camera\horplus.cpp src\camera\fov_observation.cpp src\camera\gameplay_baseline.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp src\diagnostics\cinematic_continuation_trace.cpp /link /OUT:build-artifacts\tests\cinematic_fov_harness.exe
```

### C40 — test.cmd:118

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\cinematics\cinematic_aspect_harness.cpp src\cinematics\cinematic_aspect.cpp src\hooks\signature_scanner.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c /link /OUT:build-artifacts\tests\cinematic_aspect_harness.exe
```

### C41 — test.cmd:120

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\dialogue\dialogue_fov_harness.cpp src\dialogue\dialogue_fov.cpp /link /OUT:build-artifacts\tests\dialogue_fov_harness.exe
```

### C42 — test.cmd:122

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\diagnostics\diagnostics_gate_harness.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\diagnostics_gate_harness.exe
```

### C43 — test.cmd:124

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\diagnostics\dialogue_source_provenance_harness.cpp src\diagnostics\dialogue_source_provenance.cpp /link /OUT:build-artifacts\tests\dialogue_source_provenance_harness.exe
```

### C44 — test.cmd:126

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\diagnostics\camera_writer_identity_harness.cpp /link /OUT:build-artifacts\tests\camera_writer_identity_harness.exe
```

### C45 — test.cmd:128

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\diagnostics\cinematic_lifecycle_context_harness.cpp /link /OUT:build-artifacts\tests\cinematic_lifecycle_context_harness.exe
```

### C46 — test.cmd:130

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\diagnostics\performance_telemetry_harness.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\performance_telemetry_harness.exe
```

### C47 — test.cmd:132

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\diagnostics\language_state_transition_harness.cpp src\diagnostics\language_state\language_state_transition.cpp /link /OUT:build-artifacts\tests\language_state_transition_harness.exe
```

### C48 — test.cmd:134

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\diagnostics\matchgameplay_prediction_harness.cpp /link /OUT:build-artifacts\tests\matchgameplay_prediction_harness.exe
```

### C49 — test.cmd:136

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\regression\runtime_evidence_replay_harness.cpp src\gameplay\horplus_gameplay.cpp src\gameplay\aspect_policy.cpp src\gameplay\gameplay_state.cpp src\camera\presentation_state.cpp src\cinematics\cinematic_fov.cpp src\camera\horplus.cpp src\camera\fov_observation.cpp src\camera\gameplay_baseline.cpp src\camera\gameplay_aspect_restoration.cpp src\dialogue\dialogue_state.cpp src\diagnostics\performance_telemetry.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\runtime_evidence_replay_harness.exe
```

### C50 — test.cmd:138

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\gameplay\gameplay_camera_resolver_harness.cpp src\gameplay\gameplay_camera.cpp src\hooks\instruction_validator.cpp external\safetyhook\Zydis.c /link /OUT:build-artifacts\tests\gameplay_camera_resolver_harness.exe
```

### C51 — test.cmd:140

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\composition_presenter_state_harness.cpp src\overlay\presenter\composition_presenter_state.cpp src\overlay\presenter\renderer_state.cpp src\overlay\input\input_state.cpp src\config\feature_config.cpp /link user32.lib /OUT:build-artifacts\tests\composition_presenter_state_harness.exe
```

### C52 — test.cmd:142

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\activation_trace_harness.cpp src\diagnostics\diagnostic_runtime.cpp /link /OUT:build-artifacts\tests\activation_trace_harness.exe
```

### C53 — test.cmd:144

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\overlay_layout_metrics_harness.cpp src\overlay\presentation\overlay_layout_metrics.cpp /link /OUT:build-artifacts\tests\overlay_layout_metrics_harness.exe
```

### C54 — test.cmd:146

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\selector_tooltip_state_harness.cpp /link /OUT:build-artifacts\tests\selector_tooltip_state_harness.exe
```

### C55 — test.cmd:148

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\input_state_harness.cpp src\overlay\input\input_state.cpp src\config\feature_config.cpp /link user32.lib /OUT:build-artifacts\tests\input_state_harness.exe
```

### C56 — test.cmd:150

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\camera_integration_harness.cpp src\overlay\presentation\camera_integration.cpp src\camera\horplus.cpp /link /OUT:build-artifacts\tests\camera_integration_harness.exe
```

### C57 — test.cmd:152

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\placement_config_harness.cpp src\overlay\presentation\placement_config.cpp src\config\config_repository.cpp src\config\feature_config.cpp src\config\config_template.cpp /link user32.lib /OUT:build-artifacts\tests\placement_config_harness.exe
```

### C58 — test.cmd:154

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\feature_presentation_harness.cpp src\overlay\presentation\feature_presentation.cpp src\config\feature_config.cpp /link user32.lib /OUT:build-artifacts\tests\feature_presentation_harness.exe
```

### C59 — test.cmd:156

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\semantic_snapshot_harness.cpp /link /OUT:build-artifacts\tests\semantic_snapshot_harness.exe
```

### C60 — test.cmd:158

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui tests\overlay\localization_harness.cpp src\overlay\localization\localization_catalog.cpp src\overlay\localization\localization_formatter.cpp src\overlay\localization\localization_validator.cpp src\overlay\localization\localization_manager.cpp src\overlay\localization\localization_font.cpp src\overlay\localization\localization_keys.cpp external\imgui\imgui.cpp external\imgui\imgui_draw.cpp external\imgui\imgui_tables.cpp external\imgui\imgui_widgets.cpp /link build-artifacts\tests\localization_resources.res /OUT:build-artifacts\tests\localization_harness.exe
```

### C61 — test.cmd:288

```text
cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /Fobuild-artifacts\tests\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui /DNDEBUG /DSAFETYHOOK_TEST_VIRTUAL_PROTECT /DOVERLAY_PRODUCTION /DOVERLAY_SETTINGS_FRONTEND /DOVERLAY_COMBINED /DOVERLAY_STARTUP_JOURNAL /Iexternal\imgui\backends tests\plugin\runtime_native_adapter_harness.cpp tests\plugin\runtime_owner_binding_probe.cpp @build-artifacts\tests\runtime_adapter_sources.rsp /link user32.lib psapi.lib bcrypt.lib dxgi.lib d3d11.lib dcomp.lib d3dcompiler.lib build-artifacts\tests\localization_resources.res /OUT:build-artifacts\tests\runtime_native_adapter_harness.exe
```

## Scope / acceptance

Only the three audit reports are intended tracked changes. Existing staged/unstaged source changes are preserved. Timing artifacts are ignored, under `build-artifacts/test-infrastructure-audit/`. No game, production build/publication, source splits, test/assertion edits, Git index writes or N03–N06 status changes are part of this task. Audit findings are recommendations, not implementation approval.

Final preservation result: 436 initial files match their original SHA-256 hashes; Git index hash is unchanged. Compared with initial status, the only additional paths are the three requested reports. All two canonical full runs, two instrumented full runs and two five-entry CMake subset runs returned 0. Full timing distributions and raw-log hashes are in the performance baseline. No migration was started.

