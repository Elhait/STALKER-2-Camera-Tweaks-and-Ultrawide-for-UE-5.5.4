# Final v2 architecture repair plan

Date: 2026-09-25  
Status: Batches 0–7 implementation complete; Batch 8 read-only closure audit complete. Final verdict: ARCHITECTURE CLOSED WITH DOCUMENTED LIMITATIONS; performance audit remains out of scope. See [closure report](../reports/FINAL_V2_ARCHITECTURE_CLOSURE_AUDIT_2026-09-26.md).
Source of findings: `research/reports/FINAL_V2_REPOSITORY_ARCHITECTURE_AUDIT_2026-09-25.md` plus current-source reinspection before Batch 0.

## Goal and global invariants

Establish regression protection, then repair accepted findings F-01 through F-07 in bounded changes before a closure review. This plan does not include performance work.

Across every batch preserve guarded/validated hooks, fail-closed behavior, evidence provenance, configured/desired/active/observed state distinctions, gameplay/cinematic/dialogue transition and restoration semantics, process-resident `RuntimeState`, separate overlay discovery and renderer lifecycles, current INI compatibility and localization/catalog/glyph guarantees. Test code must protect behavior and contracts, not implementation spelling. No batch may opportunistically reformat or clean unrelated code.

The repository currently contains many pre-existing staged changes. Work must preserve them; only the paths listed under the active batch may be edited. Recheck the final changed-path list before each batch closes.

## Batch 0 — Regression guardrails for F-01, F-02 and F-03 (this task)

### Current state and scope

- **F-01 locations:** `src/plugin/runtime_settings.hpp` ordinary `SetHandler`/`SetSnapshotHandler`/`SetPersistHandler` fields and reads; `src/plugin/runtime.cpp` starts `StartOverlayDiscovery` before `Initialize`, then installs callbacks later; `tests/config/runtime_settings_api_harness.cpp` tests only absent and fully installed states.
- **F-02 locations:** production `kCinemaAspect = 3440.0f/1440.0f` in `src/plugin/runtime.cpp`, policy resolution in `src/cinematics/cinematic_aspect.cpp`; overlay uses `21.0f/9.0f` in `src/overlay/camera_integration.cpp`; `tests/overlay/camera_integration_harness.cpp` lacks the production 3440×1440 case.
- **F-03 locations:** `config::PersistConfigValue` in `src/config/config_repository.cpp` is mutex-protected at plugin call sites; overlay placement load/migration and X/Y writes are in static functions in `src/overlay/renderer_runtime.cpp` and use Win32 profile APIs directly; `tests/config/config_persistence_harness.cpp` covers repository persistence/failure and legacy hotkey migration but not placement path.

### Intended work

1. **F-02A:** Add an explicit intended-semantic test for `Forced21x9 + 3440/1440`. Keep the existing different 32:9 case and add/retain `Auto`, `Native`, unavailable viewport cases. Report the current 3440 case as an explicit expected failure (XFAIL) only if the harness can distinguish that exact known mismatch from unexpected outcomes. The assertion's expected value is production's 3440/1440 semantic, never 21/9. Do not alter production or overlay implementation.
2. **F-01B:** Extend `runtime_settings_api_harness.cpp` with deterministic sequential characterization: empty API fails closed; partial handler installation is observable; after complete installation consumers work; absent individual callback remains fail-closed. This records the partial-publication gap without adding a publication mechanism. A deterministic concurrent “never observe a half bundle” contract test must be implemented with Batch 1, when the publication unit/readiness API exists; current API has no bundle state to assert. Do not make a flaky race test a pass criterion.
3. **F-03C:** Add behavior-level tests for the actual placement load/save/migration route. If the existing static renderer functions cannot be tested without starting D3D12/ImGui, extract only the placement INI operations into a small overlay-owned module and route the existing calls through it unchanged. Keep settings persistence on its existing path: no shared owner/lock/transaction in Batch 0. Test normal read/write, X/Y pair, migration and failure preservation. Add a bounded concurrent exercise against `PersistConfigValue` and placement persistence; classify any lost update as expected exposure and retain the result, not as justification to fix it now. If the current Win32/profile and whole-file writer implementations cannot be interleaved deterministically, explicitly record that limit and reserve deterministic controlled-interleaving coverage for Batch 3 rather than inventing a generic injection framework.

### Expected files

- `research/active/FINAL_V2_ARCHITECTURE_REPAIR_PLAN.md` — this plan.
- `tests/overlay/camera_integration_harness.cpp` — F-02 desired semantic regression, explicit expected-failure handling.
- `tests/config/runtime_settings_api_harness.cpp` — F-01 partial-publication characterization.
- `tests/config/config_persistence_harness.cpp` and, if necessary, a focused placement harness plus `test.cmd` registration — F-03 behavior contracts.
- `src/overlay/renderer_runtime.cpp`, new `src/overlay/placement_config.hpp/.cpp`, `tests/overlay/placement_config_harness.cpp`, `test.cmd` and `build.cmd` — extract the existing placement Win32 calls to a directly testable helper. This is a behavior-neutral seam; no synchronization or semantics changes.

### Validation, risks, rollback, stop

- **Before:** Reinspect callback/API and both INI writers; inspect current `test.cmd` compile/run/error handling and localization audit; retain existing regression coverage.
- **After:** Run each targeted new/changed harness, then full `test.cmd`. Preserve and report exact XFAIL/expected exposure separately from unexpected failure or toolchain/infrastructure failure. No full production build is required for test-only changes; if a behavior-neutral production test seam is extracted, compile the production profile too.
- **Runtime validation:** Not required for pure semantic/API/INI harnesses. Do not launch game in Batch 0. Mark startup timing and OS-level concurrent file behavior as untested where not deterministically represented.
- **Dependencies:** None. This batch must precede F-01, F-02 and F-03 implementation.
- **Rollback/failure:** If a new test requires changing runtime semantics, expanding API beyond the minimum seam, or weakening existing tests, stop and document. An exposed known defect remains an expected failure; never “fix” it in Batch 0.
- **Non-goals:** Callback synchronization/publication, aspect projection correction, persistence unification, generic concurrency framework, broad F-05 cleanup, performance or runtime investigation.
- **Stop condition:** Targeted tests and full `test.cmd` have run (or a specific environment failure is reported), expected defects are clearly classified, and Batch 0 report names every changed file. Then stop for human review.

## Batch 1 — F-01 runtime-settings callback publication/readiness

- **Current locations:** `src/plugin/runtime_settings.hpp` callback fields and `RuntimeSettingsApi` calls; `src/plugin/runtime.cpp::InitializeThread` starts discovery before `Initialize`, later installs handlers; `src/overlay/discovery_runtime.cpp` WindowProc and `src/overlay/renderer_runtime.cpp` consume snapshots/mutations.
- **Current behavior:** Overlay discovery runs concurrently with runtime initialization; callback fields default null but are independently assigned as ordinary pointers/data. Missing callbacks currently fail closed. Semantic snapshot already has separate atomic publication.
- **Problem:** No coherent readiness point orders consumer reads after complete settings callback installation; the partial state can be visible and concurrent access is a data race.
- **Intended architecture:** Publish one immutable callback/user-data bundle through a single readiness transition. Keep discovery and renderer lifecycles separate.
- **Invariants:** Before publication every operation fails closed; after publication each call uses a complete matching callback/context pair; no callback sees a destroyed context; semantic snapshot behavior unchanged.
- **Expected files:** `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.cpp`, test harness from Batch 0, possibly small startup/discovery test seam. No broad runtime decomposition.
- **Before tests:** Batch 0 characterization passes; add the deterministic publication contract test against the selected bundle/readiness seam before wiring consumers.
- **After tests:** Empty/partial/full/unavailable behavior; concurrent publish/read stress with explicit no-half-bundle invariant; full `test.cmd`; production build.
- **Runtime validation:** One startup scenario with early Present/input pressure and one normal overlay startup; failed discovery remains isolated. Static review of publication memory ordering and callback lifetime.
- **Dependencies:** Batch 0. Precedes any RuntimeSettingsApi expansion in F-04.
- **Rollback/failure:** Restore old publication route if bundle lifetime or caller ordering cannot be proven; keep consumers fail-closed. No new scheduler or dynamic unload support.
- **Non-goals:** Reorder all plugin startup, merge graphics/game lifecycles, change settings semantics.
- **Stop condition:** Complete bundle readiness is demonstrably race-free and targeted plus full tests/build match stated evidence. Record the prescribed runtime startup smoke test separately if it is not exercised.

## Batch 2 — F-02 authoritative `Forced21x9` semantic

- **Current locations:** `src/plugin/runtime.cpp` production constant and `ResolveCinematicAspect`; `src/cinematics/cinematic_aspect.cpp`; `src/overlay/camera_integration.cpp/.hpp`; camera integration harness.
- **Current behavior:** Production `Forced21x9` applies 3440/1440, while overlay projection estimates 21/9 and compares with 0.01 tolerance.
- **Problem:** UI can report confirmed mismatch at 3440×1440 despite production applying the matching 3440/1440 value.
- **Intended architecture:** Make projection consume authoritative resolved production semantics or a shared policy-owned value with an explicit name. Do not change the native applied aspect unless separate evidence authorizes it.
- **Invariants:** Production write unchanged; Auto/Native and status distinctions unchanged; unavailable data stays unavailable; provenance/freshness remain meaningful.
- **Expected files:** cinematic aspect policy owner and semantic snapshot projection, `src/overlay/camera_integration.*`, `tests/overlay/camera_integration_harness.cpp`; only necessary API/test integration.
- **Before tests:** Batch 0 expected-failure for 3440/1440 plus Auto/Native/unavailable/mismatched viewport cases.
- **After tests:** Convert the exact XFAIL to PASS; add canonical policy parity assertions; run cinematic/aspect and overlay harnesses, full `test.cmd`, production build.
- **Runtime validation:** Supported 3440×1440 matching path and clearly different 32:9 mismatch in one bounded overlay status check if available.
- **Dependencies:** Batch 0; if semantic field relies on API publication, Batch 1 first. No dependency to alter camera writer.
- **Rollback/failure:** Revert only projection/snapshot addition if it alters runtime camera value or reports alignment without evidence.
- **Non-goals:** Aspect policy redesign, changing “21:9” UI/config identifiers, FOV-path redesign.
- **Stop condition:** New tests prove UI assessment equals production's effective policy and no camera write behavior changed.

## Batch 3 — F-03 one serialized persistence authority

- **Current locations:** `src/plugin/runtime.cpp` persistence mutex and callbacks; `src/config/config_repository.cpp::PersistConfigValue`; `src/overlay/renderer_runtime.cpp` placement load/migration/direct Win32 writes; placement tests/seam from Batch 0.
- **Current behavior:** Runtime settings rewrite the whole INI atomically under a plugin mutex; renderer placement writes X/Y/migration keys through separate profile calls without that mutex.
- **Problem:** Operations against one config file have unrelated serialization and failure semantics; X/Y are separate writes.
- **Intended architecture:** One config-owned serialized update boundary for setting keys and placement pair/migration, with last-known-good file preservation. Overlay supplies placement values; config layer need not know camera feature internals.
- **Invariants:** Existing path/keys/user values, legacy migration behavior, X/Y as one logical placement, clamp/save-on-drag-end, runtime settings, failure does not corrupt/delete prior file.
- **Expected files:** `src/config/config_repository.hpp/.cpp`, `src/plugin/runtime.cpp`, `src/overlay/renderer_runtime.cpp`, focused config/placement harnesses, `test.cmd` registration.
- **Before tests:** Batch 0 sequential and concurrent characterization; ensure test can force the relevant interleaving or document what runtime evidence covers it.
- **After tests:** Deterministic two-writer update, pair atomicity, migration/idempotence, normal persistence, malformed/read-only/replace failure preservation; full `test.cmd`, production build.
- **Runtime validation:** Move overlay, alter/save a setting near the same time, restart and verify both values; legacy migration fixture on a disposable test config.
- **Dependencies:** Batch 0; use Batch 1 settings publication contract where API path is changed.
- **Rollback/failure:** On any failed write preserve old config and report error; never fallback to destructive truncation. Keep old migration inputs readable.
- **Non-goals:** INI schema redesign, changing config location/keys, deletion of old files, unrelated template cleanup.
- **Stop condition:** Single serialization owner covers all writes to main INI and tests prove values/migration survive failure and overlap.

## Batch 4 — F-05 remaining test architecture cleanup

- **Current locations:** `tests/runner/localization_catalog_audit.ps1`, `tests/runner/test_cmd_audit.ps1`, `test.cmd`, affected overlay/localization harnesses.
- **Current behavior:** Valuable parity/resource/glyph validations coexist with exact renderer-token checks; test runner audits declared compile/run pairs but does not discover every repository harness.
- **Problem:** Some renderer refactors can fail tests due to source spelling; declared test coverage and filesystem coverage are not the same contract.
- **Intended architecture:** Preserve real catalog/resource/glyph checks. Replace only source-pattern assertions that represent observable behavior with testable semantic/layout seams or narrowly documented source policy. Make harness inventory contract explicit without broad runner rewrite.
- **Invariants:** 18 locale parity, embedded resources, glyph/profile support, Auto synchronization, accepted UI semantics and every behavioral harness stay protected.
- **Expected files:** localization audit, test runner audit, `test.cmd`, focused overlay/localization tests and any minimal pure helper.
- **Before tests:** Batch 0 all new guardrails and Batch 1–3 tests are baseline. Classify each regex as behavioral evidence or intentional structural policy before replacing.
- **After tests:** Deliberately break catalog key/glyph/resource and verify audit catches it; behavior tests survive semantics-preserving renderer rearrangement; full `test.cmd` and production build audit.
- **Runtime validation:** None unless an observable UI behavior contract changes; this batch is test-only by default.
- **Dependencies:** Batches 0–3 so tests can encode repaired contracts; precedes F-04 metadata edits.
- **Rollback/failure:** Restore any guard that proves uniquely capable of detecting a real contract violation; do not delete catalog audits to make a refactor green.
- **Non-goals:** Test framework migration, wholesale snapshot/golden redesign, removing diagnostic harnesses.
- **Stop condition:** Each retained/removed source check has documented purpose and full validation passes with invariant coverage intact.

## Batch 5 — F-04 bounded setting/binding metadata consolidation

- **Current locations:** `src/config/feature_config.*`, `config_repository.*`, `config_template.cpp`, `src/plugin/runtime_settings.hpp`, apply/persist/hotkey routing in `src/plugin/runtime.cpp`, `src/overlay/discovery_runtime.cpp`, `src/overlay/renderer_runtime.cpp`.
- **Current behavior:** Stable identities/defaults/keys/options/binding relationships are repeated across switches and fixed arrays; effectful runtime transitions remain explicit.
- **Problem:** Adding one binding or setting can miss an unrelated mapping or persistence/UI path.
- **Intended architecture:** Small descriptor data for stable identity/default/config key/display option/binding route only where it removes demonstrated duplication. Retain explicit typed handlers and state-machine code.
- **Invariants:** Canonical identifiers and INI compatibility; key conflict semantics; typed mutation validation; desired/active behavior; manual and Auto locale/font semantics; notifications and fail-closed native features.
- **Expected files:** Only metadata owners and directly consuming config/runtime/input/overlay modules plus targeted harnesses.
- **Before tests:** Batch 4 clarifies behavioral test seams; add completeness/uniqueness and round-trip tests before replacing repeated mappings.
- **After tests:** Every descriptor maps once to parser, mutation, persistence and UI; mode and hotkey behavior matrix; full `test.cmd`, production build.
- **Runtime validation:** One representative rebind and one setting/mode selection if changed execution path reaches input/UI.
- **Dependencies:** Batches 1–4; F-03 serialization API should be stable before moving setting keys.
- **Rollback/failure:** Drop registry portion that increases indirection or obscures native side effects; keep hand-wired explicit branch when semantic behavior differs.
- **Non-goals:** Generic reflection/settings framework; converting every switch into a table; moving state ownership out of Runtime.
- **Stop condition:** Extension metadata has fewer omission points without hiding per-feature behavior; all existing semantics validate.

## Batch 6 — F-06 supported-build evidence authority

- **Current locations:** `docs/SUPPORTED_BUILD_MANIFEST.md`, `TESTING_AND_RESEARCH.md`, current validated runtime logs/hash records and release artifact metadata.
- **Current behavior:** Manifest headline names Steam 2.0.5; research/test record states production runtime validation on 2.0.6 and separately narrows evidence for resolver portability/Auto reader.
- **Problem:** Scope of “current target” can be read as contradictory across subsystems.
- **Intended architecture:** Human-reviewed per-feature support matrix distinguishes static resolver evidence, full production runtime validation, and the standalone reader's narrower validation. One canonical statement, linked elsewhere.
- **Invariants:** Never infer build support from an unrelated feature's hash; retain fail-closed resolver requirements and runtime evidence provenance.
- **Expected files:** Manifest, testing/research index and perhaps a concise evidence table only.
- **Before tests:** Verify all referenced current artifacts, logs, hashes and evidence documents; no source behavior change.
- **After tests:** Link/consistency audit for every version/hash claim; review links and scope wording.
- **Runtime validation:** None; evidence review only. Do not run game to manufacture evidence within this documentation batch.
- **Dependencies:** Technical repairs may proceed independently; do after runtime batches so support statement reflects current final code/artifact.
- **Rollback/failure:** If evidence does not support one scope, state unknown/unvalidated rather than broadening claims.
- **Non-goals:** Changing resolver compatibility, SHA gating, or historical records.
- **Stop condition:** One unambiguous support matrix is traceable to existing evidence with uncertainty preserved.

## Batch 7 — F-07 production-semantic overlay build naming

- **Current locations:** `build.cmd`, `src/overlay/discovery_runtime.cpp`, `src/overlay/renderer_runtime.cpp`, compatibility build scripts under `tools/build/`, build docs and test source-policy audit.
- **Current behavior:** Production flags included `OVERLAY_RENDERING_POC` and `OVERLAY_SETTINGS_FRONTEND`; the POC-named gate selected production renderer/input integration. Compatibility script names may intentionally remain.
- **Problem:** Build selection and source gates imply production overlay code is experimental; alternate profiles may use the same flag.
- **Intended architecture:** Inventory all supported compile profiles, replace production-inaccurate flag with a production-semantic definition, retain compatibility aliases only where externally useful, and remove empty guards only when proven redundant.
- **Invariants:** Production and diagnostic artifact contents remain distinct; production overlay remains included; optional discovery/failure isolation unchanged.
- **Expected files:** build scripts, guarded source files, `test.cmd` source-audit assumptions, build/architecture docs.
- **Before tests:** Complete profile/source-list inventory; ensure F-05 no longer encodes flag names as behavior.
- **After tests:** Compile production and each supported diagnostic profile, test suite and localization audit, compare artifact source/resource inputs.
- **Runtime validation:** Production overlay startup smoke test; no need to launch every diagnostic profile unless behavior is changed.
- **Dependencies:** F-05 before rename; F-06 for build/support documentation alignment.
- **Rollback/failure:** Restore a compatibility define when an existing supported target requires it; do not remove alternate profile branches without evidence.
- **Non-goals:** Overlay redesign, moving renderer logic, deleting historic research artifacts.
- **Stop condition:** Production code is gated by accurate names, all supported profiles still compile, release artifact contents are verified.

## Batch 7 execution record — 2026-09-25

### Implementation and profile inventory

- `build.cmd` — production profile; now defines `OVERLAY_PRODUCTION` with the existing `OVERLAY_SETTINGS_FRONTEND` and `OVERLAY_COMBINED` gates. Production no longer defines the POC-named macro.
- `src/overlay/discovery_runtime.cpp` — shipped renderer/input branches compile for `OVERLAY_PRODUCTION`; the old `OVERLAY_RENDERING_POC` gate remains accepted only for the standalone historical POC source profile.
- `tools/build/build-diagnostic.cmd` — supported diagnostic ASI profile; retains all pre-existing baseline diagnostic definitions and optional feature diagnostic defines.
- `tools/build/build-overlay-settings.cmd` — compatibility alias to the production build; its entry point is retained.
- `tools/build/build-overlay-poc.cmd`, `tools/build/build-overlay-discovery.cmd` and research watcher scripts — inventoried and documented as retired standalone research scripts, not supported package profiles. No broad repair/removal was done.
- `README.md`, `docs/ARCHITECTURE.md` — document the semantic production gate, supported production/diagnostic/compatibility profiles, and retirement status of isolated research scripts.

### Validation and disposition

- Full `test.cmd`: PASS; 43/43 repository harnesses compiled and executed; 18 locale catalogs, 162 keys, embedded resources, glyph/profile coverage and localization audits PASS.
- `build.cmd`: PASS for the production ASI with the new semantic gate.
- The exact production artifact emitted by the final compatibility-alias build has SHA-256 `1AFF9A14D5A55B9E08FE1D3E3418ABDC6CA5C39B046B5DD787620850CC7A843D`; the diagnostic artifact from the all-options profile has SHA-256 `0E9E6CE57733B987AC8DACC0FF0F8CA5FC1E1BBD34A7B4F47955B8DBEC0EBA11`. These identify local build outputs only and are not runtime-validation claims.
- `build-diagnostic.cmd`: PASS for the supported diagnostic artifact, both with default diagnostic definitions and with all optional diagnostic defines enabled together.
- `build-overlay-settings.cmd`: PASS as production compatibility alias.
- Supported production/diagnostic source and resource lists remain identical; only the existing diagnostic definitions and output artifact name differ. No renderer/runtime source list or behavior changed.
- Legacy research script probes confirmed they are not valid supported build profiles in the current tree: `build-overlay-poc.cmd` fails compiling current `renderer_runtime.cpp` because the isolated target omits localization/settings frontend dependencies; `build-overlay-discovery.cmd` fails link due to omitted `diagnostic_runtime.cpp`. They remain documented as retired and were not repaired as part of the production-semantic gate fix.
- Diagnostic-all-options compilation reported `C4456` shadow warnings in diagnostic-only runtime blocks, in addition to the known vendored Zydis `C4201` and ImGui backend `C4189` warnings. Production build had no project-owned warnings.
- No game launch or overlay smoke test was performed. A production overlay startup smoke test using the exact artifact under review remains an optional runtime follow-up; this batch does not claim the new artifact runtime-validated.

### Batch gate

F-07 is **CLOSED** for the production build-profile naming ambiguity: the production gate is semantic, supported production/diagnostic/compatibility paths compile, and the legacy source gate remains available without being defined by production. The retired standalone research scripts are explicitly outside the supported profile matrix. Stop after Batch 7; do not start Batch 8 or performance work.

## Batch 8 closure record — 2026-09-26

The subsequent explicit Batch 8 request authorized this read-only closure audit. It is recorded in [`FINAL_V2_ARCHITECTURE_CLOSURE_AUDIT_2026-09-26.md`](../reports/FINAL_V2_ARCHITECTURE_CLOSURE_AUDIT_2026-09-26.md). F-01–F-07 remain closed, with the runtime/evidence limits stated there. Fresh `test.cmd` passed 43/43 with zero XFAIL; localization/resource/glyph audit passed for 18 locales and 162 keys. No game was launched, no production source changed, and the current local production ASI is not runtime-validated. Stop here; no performance audit or release packaging is authorized by this closure task.

## Batch 8 — Architecture closure review

- **Current locations:** All changed paths from Batches 0–7; audit report and this plan; production build/test entry points and required runtime evidence records.
- **Current behavior:** Repairs are individually validated but not yet reviewed as one final repository state.
- **Problem:** Local passes do not prove no cross-batch regressions or that findings were resolved/closed with evidence.
- **Intended architecture:** Read-only closure review maps every F-01–F-07 to fixed, explicitly accepted limitation, or still open, with tests and runtime evidence tied to the final artifact.
- **Invariants:** All global invariants in this plan; no performance claims or unapproved scope expansion.
- **Expected files:** Audit report/closure evidence only if requested by the accepted closeout workflow; no production changes during review.
- **Before tests:** Reinspect complete diff and current contracts; verify profile and artifact identity.
- **After tests:** Full `test.cmd`, production build, all required one-pass runtime scenarios for hook/startup/aspect/config/UI contracts; confirm localization/resource audits.
- **Runtime validation:** Use bounded, named scenarios and the exact production artifact/game build; report diagnostic vs production evidence separately.
- **Dependencies:** Batches 0–7 complete or individually dispositioned.
- **Rollback/failure:** Reopen only the dependent batch when concrete evidence contradicts its contract; do not hide unresolved gaps in a global PASS.
- **Non-goals:** Performance audit, speculative cleanup, expanding supported game versions.
- **Stop condition:** Each accepted finding is resolved or explicitly dispositioned with evidence and no mandatory validation gap is described as PASS. Then await human release decision.

## Evidence-based ordering notes

The requested order is retained. Batch 4 (test architecture) precedes F-04 so metadata work can use behavioral rather than source-spelling checks. Batch 1 publication correctness precedes any new snapshot fields or mutation/API metadata. Batch 3 owns all main-INI writes before F-04 can alter persistence mapping. F-07 waits for F-05 because current source-policy checks inspect rendering implementation and could otherwise make a naming-only build change look like a behavior failure. F-06 is scheduled after code repairs so its matrix can name the final evidence scope. No source evidence justifies silently changing this order.

## Batch 1 execution record — 2026-09-25

### Implementation

- `src/plugin/runtime_settings.hpp` — replaced four independently published callback routes with a single `CallbackBundle` and one-shot publication state (`Unpublished → Publishing → Published`). The winning publisher copies the complete bundle before a release-store of `Published`; every consumer performs an acquire-load and only then reads the immutable bundle. Competing/repeated publishers are rejected. Missing callbacks within an intentionally partial bundle remain individually fail-closed.
- `src/plugin/runtime.cpp` — publishes all four production callbacks together after configuration-derived runtime state is initialized. An unexpected repeat/failure is logged; it does not couple optional overlay readiness to camera hook installation. Discovery still starts before initialization, so early calls safely see unavailable state until publication.
- `tests/config/runtime_settings_api_harness.cpp` — converted the Batch 0 partial-publication XFAIL into PASS coverage: empty/pre-publication fail-closed behavior, full publication, mutation/snapshot/persist access, unavailable callback behavior, repeated publication rejection, and concurrent readers racing two complete publishers with distinct callback-context generations. The test asserts exactly one publisher wins and no callback from the losing generation is observed.
- `tests/overlay/semantic_snapshot_harness.cpp` — publishes its semantic reader as a bundle, preserving the existing provenance/validity assertions.

### Review and validation

- Publication uses a single atomic state as the readiness contract; callback fields are non-atomic but are written only by the winning publisher before the release-store and never modified afterward. Acquire readers cannot access the fields while the state is `Unpublished` or `Publishing`. The API is a process-resident `RuntimeState` member, so the callback bundle and production contexts outlive all consumers under the documented lifetime contract.
- No camera/gameplay/cinematic/dialogue behavior or discovery/renderer lifecycle ordering was changed. Unexpected duplicate publication leaves the API unavailable rather than exposing a partial callback set.
- Targeted `runtime_settings_api_harness.exe`: PASS. Related `semantic_snapshot_harness.exe` and `overlay_lifecycle_harness.exe`: PASS.
- Concurrency regression repeat: 100 consecutive `runtime_settings_api_harness.exe` runs PASS.
- Full `test.cmd`: exit 0; runner audit 42/42 sources/compile/run entries; all harnesses passed except the explicitly expected Batch 0 XFAILs: F-02 `Forced21x9` projection and F-03 concurrent INI lost-update exposure. Localization/resource audit: 18 locales, 162 keys, parity, placeholders, embedded resources, and glyph/profile audits PASS.
- Production `build.cmd`: exit 0; `STALKER2CameraTweaks.asi` built. Warnings were confined to vendored Zydis (`C4201`) and ImGui DX12 backend (`C4189`).
- Runtime startup smoke (early Present/input pressure plus normal startup): **not run**. Offline tests prove the publication/readiness contract; actual in-game startup behavior remains unobserved in this batch.

### Batch gate

F-01 is **CLOSED for the demonstrated publication/readiness defect**: a logically partial set of independently installed callbacks is no longer publishable or readable, the XFAIL is now a normal passing regression, and the production artifact builds. The planned in-game startup smoke remains an evidence gap, not a claim of runtime validation. Stop here for human review before Batch 2.

## Batch 2 execution record — 2026-09-25

### Implementation

- `src/cinematics/cinematic_aspect.hpp` — established the single named `cinematics::Forced21x9Aspect` policy value (`3440/1440`) with a comment documenting why it intentionally differs from mathematical `21/9`.
- `src/plugin/runtime.cpp` — production cinematic resolution now consumes that shared value in place of its private `kCinemaAspect`; effective aspect and native camera writes are unchanged.
- `src/overlay/camera_integration.cpp` — read-only projection consumes the same cinematic policy value instead of independently resolving `21/9`.
- `tests/cinematics/cinematic_aspect_harness.cpp` — explicitly guards the established production invariant: `Forced21x9` still resolves to `3440/1440`.
- `tests/overlay/camera_integration_harness.cpp` — removed the F-02 XFAIL, added direct production-value parity and 3440×1440 alignment coverage, retained 32:9 mismatch/unavailable/Auto/Native cases, and added a Forced16x9 behavior assertion. Existing Forced32x9 coverage remains.

### Review and validation

- The authoritative value is owned by the existing cinematic aspect policy module; production and presentation both consume it. No camera hook, Apply/store, FOV, ENTER/EXIT, gameplay, dialogue, config identifier, label, tolerance, or validity/provenance/freshness behavior changed.
- Targeted camera integration and cinematic aspect harnesses: PASS. Full `test.cmd`: exit 0; runner audit 42/42 sources/compile/run entries; F-02 is PASS. F-03 concurrent INI lost-update is the only remaining expected XFAIL. Localization/resource audit: all 18 locales, 162 keys, parity, placeholders, embedded resources, glyph and profile checks PASS.
- Production `build.cmd`: exit 0; `STALKER2CameraTweaks.asi` built. Warnings remain limited to vendored Zydis (`C4201`) and ImGui DX12 backend (`C4189`).
- Runtime validation was not performed. Record for the final runtime regression: on a supported 3440×1440 path, `Forced21x9` should report aligned; a clearly different 32:9 viewport should report mismatch; actual cinematic aspect behavior should remain unchanged.

### Batch gate

F-02 is **CLOSED for the demonstrated duplicated/conflicting presentation semantic**. The original expected mismatch is now covered as a passing alignment regression, mismatching viewport detection remains covered, and production output remains `3440/1440`. Runtime behavior is not claimed as exercised. Stop here for human review before Batch 3.

## Batch 0 execution record — 2026-09-25

### Files changed

- `tests/overlay/camera_integration_harness.cpp` — test-only F-02 semantic assertion; current known wrong projection is reported as XFAIL.
- `tests/config/runtime_settings_api_harness.cpp` — test-only F-01 partial-publication characterization; current early mutation acceptance is reported as XFAIL.
- `src/overlay/placement_config.hpp/.cpp` — extracted the existing Win32 placement load/save/migration calls into a directly testable overlay module. Same path names, keys, defaults, write ordering, migration marker and return behavior; no synchronization added.
- `src/overlay/renderer_runtime.cpp` — delegates the former in-file placement calls to the extracted helper; renderer lifecycle, drag-end timing and error log remain unchanged.
- `tests/overlay/placement_config_harness.cpp` — covers normal placement, migration then settings persistence, failure path, and bounded concurrent setting/placement updates.
- `test.cmd` — registers and runs the placement harness.
- `build.cmd` — includes the behavior-neutral placement helper in the production ASI source list.

### Guardrail outcomes

- **F-01:** Empty API fails closed; complete handler setup works; incomplete handler setup exposes the mutation callback while snapshot/persistence callbacks are unavailable. Reported `XFAIL` for the documented gap. A deterministic concurrent whole-bundle publication test is deferred to Batch 1, when a real publication/readiness contract exists; no callback synchronization was added here.
- **F-02:** Coverage includes production `3440/1440`, clearly different 32:9, invalid/missing viewport, Auto and Native. The known current result (`21/9`, mismatch) is an explicit `XFAIL`, not the expected semantic. Remaining projection and state cases pass.
- **F-03:** Actual extracted placement path passes load/save, X/Y pair, migration, subsequent config setting write and invalid-path failure checks. Concurrent exercise ran 24 synchronized starts per harness invocation. The full suite reproduced an update loss in **24/24** iterations: setting value missing in 17 and X placement missing in 7; Y was retained in those iterations. A targeted rerun reproduced loss in 24/24 again (setting missing in 14, X missing in 10). These are `XFAIL` evidence for the accepted existing race, not a production fix. The run cannot establish which interleavings occur in-game or cover every filesystem/Windows cache behavior; Batch 3 must make the test use the unified writer contract.

### Validation

- Targeted `runtime_settings_api_harness.exe`: PASS with expected F-01 XFAIL.
- Targeted `camera_integration_harness.exe`: PASS with expected F-02 XFAIL.
- Targeted `placement_config_harness.exe`: PASS with expected F-03 XFAIL and normal placement/migration/failure checks passing.
- Full `test.cmd`: PASS; runner audit reports 42 sources/compiles/runs equal, localization audit reports 18 locales/catalog parity/resources/glyph checks PASS, and all C++ harnesses pass with the documented XFAIL output.
- `build.cmd`: production `STALKER2CameraTweaks.asi` built, including `placement_config.cpp`. Compiler emitted warnings in vendored Zydis (`C4201`) and ImGui DX12 backend (`C4189`); no warning was reported from the new project helper.
- Not executed: game/runtime visual validation, startup concurrency runtime scenario, performance work.

### Batch gate

No intentional production runtime behavior change. Production source changed only to extract the existing overlay placement INI calls into a testable module; the production build and harness verify linkage and the exercised semantics. F-01 and F-02 repairs remain untouched. Batch 0 is complete; stop here for human review before Batch 1.

## Batch 3 execution record — 2026-09-25

### Implementation

- `src/config/config_repository.hpp/.cpp` — established one config-owned serialized update authority. Settings updates, placement updates, placement migration (including its completion marker), initial INI creation, and managed-template synchronization all participate in the same mutex. Placement X/Y and migration marker are rewritten as one staged whole-file update; replacement is atomic and never truncates the prior valid file as a fallback.
- `src/config/config_template.cpp` — uses the config-owned serialization guard around managed-template read/modify/replace, closing the additional main-INI writer discovered during the required inventory.
- `src/overlay/placement_config.cpp` — removed direct Win32 profile writes; renderer-facing placement seam now delegates loading/migration and X/Y save operations to config repository.
- `tests/overlay/placement_config_harness.cpp` — converted concurrent writer exposure from XFAIL characterization into a required PASS with zero lost setting/X/Y updates. Existing 24 synchronized starts per run are retained.

### Review and validation

- Production main-INI writer inventory covered first-run config creation, managed-template synchronization/migration, `PersistConfigValue`, placement load migration/marker, and placement X/Y save. All mutating routes now serialize at the config-owned boundary; overlay code no longer writes the main INI directly.
- Targeted/full `test.cmd`: PASS (exit 0). Runner audit: 42/42 source/compile/run entries; localization/resource/glyph audit: 18 locales, 162 keys, parity/placeholders/resources/profiles PASS. Config persistence cases (normal, failure preservation, template idempotence, locale/font/hotkey behavior) and placement load/save, migration/idempotence and preservation all PASS.
- Concurrent persistence regression: 24 synchronized setting+placement starts per run, zero losses; 10 consecutive independent harness runs PASS (240 synchronized pairs total).
- Production `build.cmd`: PASS; `STALKER2CameraTweaks.asi` produced. Warnings remain confined to vendored Zydis (`C4201`) and ImGui DX12 backend (`C4189`).
- No game runtime was launched. Existing runtime follow-up remains: move overlay and persist a setting near the same time, restart, and confirm both values; verify legacy migration using a disposable user config. Offline serialization and failure-preservation contracts passed.
- No camera/gameplay/cinematic/dialogue runtime semantics were intentionally changed.

### Batch gate

F-03 is **CLOSED for the demonstrated same-process concurrent-writer lost-update defect**. Settings, placement pair, migration marker, template update and initial file creation are serialized by config ownership, while successful writes preserve unrelated settings and failed replacement leaves the previous file intact. Proceed to Batch 4.

## Batch 4 execution record — 2026-09-25

### Implementation

- `tests/runner/test_cmd_audit.ps1` — runner/source/compile/run parity now also compares `test.cmd` against the discovered repository inventory of every `tests/**/*_harness.cpp` file. This exposed a 43rd A/B/A diagnostic evidence harness that had only been run by its standalone diagnostic build; it can no longer silently sit outside the normal validation inventory.
- `test.cmd` — added `language_state_transition_harness.cpp` and its production-independent classifier source to the ordinary suite.
- `src/overlay/game_language_reader.hpp/.cpp`, `src/overlay/discovery_runtime.cpp`, `tests/overlay/localization_harness.cpp` — extracted the small `visible && Auto-configured` synchronization decision as a constexpr policy used by the existing WindowProc/read path and directly tested all four input combinations. The HWND post/message route remains an intentional source-level boundary check.
- `tests/runner/localization_catalog_audit.ps1` — documented the remaining source checks by purpose: registry/resource identity wiring, embedded-font/glyph source policy, and renderer/UI source policies where a headless ImGui run is not a reliable visual oracle. Behavioral catalog, font-profile, status, layout-policy and manager behavior continues to be guarded by the registered C++ harnesses and the catalog/resource audit.

### Classification and validation

- Preserved data/resource checks: strict UTF-8 and JSON loading, canonical key parity, 18-locale parity, placeholder/technical token parity, locale descriptor uniqueness, resource script/ID mappings, embedded font/profile/glyph coverage and prohibition on external font/catalog loading.
- Retained as deliberate source policies: no legacy per-language catalog routes, no filesystem locale loading, resource/ID wiring, embedded-font-only loading, typed localization use/no obsolete keys, localized copy requirements, accepted notification and original-layout contracts, and direct UI literal/POC exclusions. Status and feature semantics are additionally exercised by behavioral harnesses. These checks constrain architecture/UI source because the immediate-mode renderer has no dependable offline visual-text oracle; no blanket regex removal was performed.
- Replaced the Auto-sync boolean source proxy with direct policy behavior coverage; retained only the structural Windows post/message route check needed to ensure the tested policy remains connected to the bounded overlay-open path.
- `test_cmd_audit.ps1` positive check: 43 repository harnesses = 43 compiled = 43 executed, all errorlevel checks present. Deliberately added an unregistered temporary harness: audit failed with that exact filename; removed the temporary file afterward.
- Localization/resource mutation check: temporarily changed `IDR_FONT_CYRILLIC` from 103 to 903; audit failed with `embedded_resources=False`; restored the original ID and reran successfully.
- Full `test.cmd`: PASS; all 43 harnesses pass. Localization/resource/glyph audit: 18 locales, 162 keys and all parity/placeholder/resource/profile checks PASS.
- Production `build.cmd`: PASS; canonical ASI built. Warnings remain confined to vendored Zydis (`C4201`) and ImGui DX12 backend (`C4189`). No runtime launch.

### Batch gate

F-05 is **CLOSED**: meaningful source policies remain classified, behavior proxies have direct test coverage where a small seam is practical, and harness repository inventory cannot silently diverge from normal runner coverage. No camera/gameplay/cinematic/dialogue runtime behavior changed. Proceed to Batch 5.

## Batch 5 execution record — 2026-09-25

### Implementation

- `src/config/feature_config.hpp/.cpp` — added one bounded five-entry hotkey binding registry for semantic identity, INI section/key, and default key. `FeatureConfig` defaults derive from it; parser dispatch, uniqueness repair, and conflict membership now consume registry identities while preserving the former conflict-priority order (Overlay Toggle first).
- `src/plugin/runtime_settings.hpp` — snapshot defaults consume the same binding metadata.
- `src/plugin/runtime.cpp` — a single explicit RuntimeSettingKind-to-binding identity mapper is reused for mutation validation/application and persistence. Effectful state updates remain explicit; persistence obtains its INI path from the registry instead of a second key ternary.
- `src/overlay/input_state.hpp` — `HotkeyAction` now aliases the config-owned binding identity, removing a parallel set of five action identities.
- `src/overlay/hotkey_binding_presentation.hpp`, `src/overlay/renderer_runtime.cpp` — added the small UI-facing label/group/runtime-field mapping and made the existing overlay/hotkey rows render from it. UI order/group and action-specific behavior remain unchanged.
- `tests/config/config_persistence_harness.cpp`, `tests/overlay/input_state_harness.cpp` — added completeness, uniqueness, default parity, parser/persistence round-trip, conflict-membership, UI field/label and input-rebind identity coverage.
- `tests/runner/localization_catalog_audit.ps1` — updated its intentional source-policy assertion to guard registry-driven UI rendering rather than the removed hand-wired row spelling.

### Review and validation

- Consolidation was limited to the demonstrated hotkey metadata duplication. Other setting parsing/persistence and all camera/gameplay/cinematic/dialogue effects remain explicit; no generic settings framework was introduced.
- The registry preserves the old default values, INI names/schema, legacy `[Hotkeys] OverlayToggle` migration behavior and duplicate-key recovery priority. Existing runtime behavior and notification mappings remain as before.
- Targeted config persistence and input harnesses: PASS, including all five binding round trips and cross-route identity checks. Full `test.cmd`: PASS; 43/43 repository harnesses compile and execute. Localization/resource/glyph audit: 18 locales, 162 keys, all checks PASS.
- Production `build.cmd`: PASS; canonical ASI produced with only existing vendored Zydis (`C4201`) and ImGui DX12 backend (`C4189`) warnings.
- No game runtime was launched. Runtime smoke follow-up: perform one representative rebinding and one settings/mode selection to confirm visible row, saved key and notification still agree.
- No camera/gameplay/cinematic/dialogue runtime semantics were intentionally changed.

### Batch gate

F-04 is **CLOSED for the demonstrated hotkey metadata omission points**: config parsing/defaults/conflicts/persistence, runtime snapshot, input identity and UI presentation now share and test a complete five-binding contract without abstracting their effects. Proceed to Batch 6.

## Batch 6 execution record — 2026-09-25

### Evidence review and documentation

- `docs/SUPPORTED_BUILD_MANIFEST.md` is now the authoritative per-build matrix, separating static resolver portability, Steam 2.0.5 production runtime evidence, Steam 2.0.6 combined production runtime evidence, and the narrower 2.0.6 Auto language-reader validation.
- Cross-patch static evidence is scoped to the historical v0.4.0 resolver contracts on identity-matched Steam 2.0.2/2.0.3/2.0.4 images; it does not establish the complete current resolver set. Runtime records identify Steam 2.0.5 game SHA `E7B481…` / mod SHA `69021D…`, and Steam 2.0.6 game SHA `61BC1E…` / logged mod SHA `19F2F3…`.
- Resolved the conflicting repository-wide “current build” language in `TESTING_AND_RESEARCH.md` and linked readers to the manifest. The v1.0.0 release-candidate ASI hash `D04A43…` differs from the mod hash in the combined 2.0.6 runtime record; binary identity is therefore explicitly UNKNOWN, and the runtime pass is not attributed to that exact release artifact.
- The current release-assets ASI was independently hashed during review (`3C6F95…`); no reviewed runtime record identifies it, so it is not called runtime-tested. Historical release notes remain historical and are not rewritten as current support evidence.

### Validation and disposition

- Reviewed the completed matching-image resolver evidence, Steam 2.0.5 runtime evidence record, Steam 2.0.6 combined runtime record, v1.0.0 compatibility claims, release-candidate hash, and current release-assets binary hash.
- Checked the matrix's relative links and searched the current support-summary documents for the contradictory “2.0.5 current target / 2.0.6 only separate reader” wording; the authoritative summary no longer contains that conflict.
- No source, runtime behavior, runtime evidence, or binary was changed by Batch 6.

### Batch gate

F-06 is **CLOSED as a documentation inconsistency**, with the exact 2.0.6 loaded-mod-to-release-candidate binary equivalence explicitly left UNKNOWN rather than inferred. Proceed to Batch 7.
