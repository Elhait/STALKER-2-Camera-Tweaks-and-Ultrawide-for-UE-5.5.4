# Final v2 repository architecture and maintainability audit

Date: 2026-09-25  
Scope: canonical repository, source-level Phase 1 only  
Status: audit, **not** a repair plan or release certification

## Executive summary

The repository is substantially understandable and defensible to an experienced C++ developer. Its central `RuntimeState` is large because it coordinates guarded hooks, game-specific transition state, restoration, and evidence provenance; splitting it for appearance alone would make ownership harder to verify. Domain modules, fail-closed resolution, read-only semantic snapshots, dedicated overlay discovery/rendering lifecycles, embedded localization, and numerous focused harnesses are real architectural strengths.

Three concrete cross-boundary issues deserve attention before calling the architecture release-stable: the settings API is published through ordinary function-pointer fields after a parallel overlay worker has started; overlay placement and other settings write the same INI through independent unsynchronized paths; and the UI's `Forced21x9` assessment uses `21/9` while production applies `3440/1440`. The first two are source-established concurrency/authority hazards, **not** observed crashes or lost writes. The third is a source-established difference with a deterministic false-mismatch implication at 3440×1440, not an in-game observation. The remaining findings concern extension pressure, test brittleness, and contradictory build-evidence wording.

This audit inspected the production tree, configuration, localization resources, build scripts, test runners/harnesses, project contracts, and selected research evidence. The supplemental code-clarity review below evaluates naming, contracts, traceability, comments and abstraction justification without treating code size or style as defects. It did not execute the game, tests, builds, Git, or performance measurements. Findings use current source as authority; historical reports are context only.

## Repository architecture map

| Area | Responsibility and authority | Principal flow/dependency |
| --- | --- | --- |
| DLL entry and plugin runtime | Process-resident integration/lifetime owner; initializes config, validates native boundaries, installs hooks, coordinates feature state and workers | `src/plugin/dll_entry.cpp` → `InitializeThread`/`Initialize` in `src/plugin/runtime.cpp`; domain and platform modules below it |
| Hooks/resolvers/platform | Find and validate executable boundaries; establish guarded hooks and Windows memory/viewport services | `src/hooks/`, `src/platform/win32/`; failure must leave affected feature pass-through/disabled, never guess an RVA |
| Gameplay | Camera writer observation, HorPlus policy, baseline/restoration and mode transitions | `src/gameplay/` with `src/camera/` evidence; plugin callback supplies validated native context |
| Cinematics | Aspect policy, ENTER/EXIT FOV, selection and lifecycle | `src/cinematics/`; plugin owns live hook callbacks, active selection and native writes |
| Dialogue | FOV/zoom transformation, phase and recovery rules | `src/dialogue/`; plugin owns hook boundary and passes validated observations |
| Camera state/evidence | Snapshot, FOV observation, baseline, restoration and presentation coordination | `src/camera/`; observational facts are not configuration authority |
| Config/settings/hotkeys | Parse defaults and persisted config; expose typed runtime mutations and snapshots; persist changes | `src/config/` → plugin desired/active state; overlay and worker hotkeys call `RuntimeSettingsApi` rather than editing camera state |
| Overlay | DXGI/D3D12 discovery, renderer/device lifecycle, Win32 input, status projection, notifications | `src/overlay/discovery_runtime.cpp` and `renderer_runtime.cpp`; reads snapshots and issues mutations, should not own production camera semantics |
| Localization/fonts | Locale registry, 18 embedded catalogs, fonts/atlas and guarded one-shot Auto language read | `src/localization/`, `src/overlay/localization_*`, `.rc`/font resources; Auto read occurs at overlay open, not polling |
| Diagnostics | Optional gated traces, logs and separate diagnostic artifacts | `src/diagnostics/`; observations must not become an alternative production state owner |
| Validation/build | Production `build.cmd`; `test.cmd` harnesses and catalog/resource audit; separate diagnostic builds | Production and diagnostic outputs have different scope; static audit does not validate runtime behavior |

Core data flow: persisted `FeatureConfig` → plugin requested atomics and active transition snapshots → validated hook callbacks/native observations → camera/domain evidence → plugin read-only semantic snapshot → overlay presentation. UI mutations flow back through `RuntimeSettingsApi` to plugin state and persistence. Render/device and game-hook lifecycles are separate; their synchronization is the issue in F-01, not their separation itself.

## Findings by subsystem

### Runtime settings / overlay startup

#### F-01 — Unsynchronized publication of runtime-settings callbacks

- **Priority / confidence:** P0, source-established possible interleaving; actual crash or race occurrence not observed.
- **Location:** `src/plugin/runtime.cpp:4984-4989,4693-4716`; `src/plugin/runtime_settings.hpp:245-255,283-305`; `src/overlay/discovery_runtime.cpp:262-265,322-337,825-831`; `src/overlay/renderer_runtime.cpp:849-861`.
- **Current responsibility:** `RuntimeSettingsApi` exposes mutation, settings snapshot, semantic snapshot and persistence callbacks. The overlay discovery worker installs input/render hooks and consumes those callbacks.
- **Concrete problem / evidence:** `InitializeThread` starts `StartOverlayDiscovery` before `Initialize`; discovery immediately creates its own thread. `Initialize` later writes three ordinary handler/user-data pairs; `Snapshot`, `Apply` and `Persist` read those fields without synchronization. Only the semantic snapshot pair uses atomics. WindowProc and renderer can call `Snapshot` once discovery succeeds. Nothing in the inspected path orders their reads after all callback writes. Concurrent read/write of a plain pointer is a C++ data race; even if a particular startup usually completes initialization first, the API has no publication contract.
- **Why it matters:** A new early Present or input event can expose partially initialized settings API or undefined behavior. It makes overlay lifecycle safety depend on game timing.
- **Proposed direction:** Make callback publication an explicit readiness transition: publish one immutable callback bundle with release/acquire semantics, or finish settings initialization before overlay discovery can consume it. A minimal ordering gate may be preferable to a new subsystem. Preserve separate renderer/discovery lifecycles.
- **Behavior to preserve:** Optional overlay; fail-closed missing callbacks; process-resident lifetime; existing startup/load timing as far as safely possible; no UI ownership of runtime state.
- **Refactor risk:** High.
- **Dependencies:** Address before extending `RuntimeSettingsApi` (F-04). Coordinate with F-02 only if common startup sequencing changes.
- **Validation after future refactor:** Deterministic concurrent publication/read harness; startup with early Present and early input; overlay unavailable/failed-discovery path; repeated load/unload-as-supported checks (do not invent dynamic unload support); production build and in-game smoke test.

### Cinematic aspect / overlay presentation

#### F-02 — Two definitions of `Forced21x9` give a false mismatch

- **Priority / confidence:** P1, arithmetic and data flow established by source; visible game behavior still requires runtime validation.
- **Location:** `src/plugin/runtime.cpp:109,1198-1207`; `src/cinematics/cinematic_aspect.cpp:71-80`; `src/overlay/camera_integration.cpp:14-34,63-71,97-100`; `src/overlay/camera_integration.hpp:46-48`; `tests/overlay/camera_integration_harness.cpp:72-100`.
- **Current responsibility:** Production applies cinematic aspect policy; the overlay projects aspect/FOV compatibility into Camera Transition status.
- **Concrete problem / evidence:** Production passes `3440.0f/1440.0f` (≈2.38889) for `Forced21x9`; overlay resolves the same enum to `21.0f/9.0f` (≈2.33333). Difference ≈0.05556 exceeds overlay tolerance 0.01. At a 3440×1440 viewport the projection therefore marks `AspectAssessment::Mismatch` and `NotAligned`, even though the production forced aspect equals that viewport. Existing harness tests a clearly nonmatching 32:9 viewport but not this matching production value.
- **Why it matters:** UI diagnosis contradicts applied behavior and can mislead users about configuration correctness.
- **Proposed direction:** Have projection consume an authoritative resolved production aspect fact or a shared semantically named policy value. Do not silently replace production's 3440/1440 aspect with exact 21/9 merely to make the UI agree.
- **Behavior to preserve:** Current cinematic aspect write, `Auto`/`Native` semantics, provenance and `Waiting`/`NotAligned` classifications when data is genuinely unavailable or different.
- **Refactor risk:** Medium.
- **Dependencies:** None for diagnosis; if adding a semantic snapshot field, sequence after F-01's publication contract.
- **Validation after future refactor:** Pure projection cases for 3440×1440 match, 32:9 mismatch, missing viewport, `Auto` and `Native`; existing cinematic aspect harness; in-game 3440×1440 status comparison.

### Configuration / persistence

#### F-03 — Main INI has two uncoordinated writers

- **Priority / confidence:** P1, independent write paths established; an actual lost update is not demonstrated.
- **Location:** `src/plugin/runtime.cpp:4419-4429,4431-4509`; `src/config/config_repository.cpp:359-425`; `src/overlay/renderer_runtime.cpp:119-160,1321-1345`.
- **Current responsibility:** Plugin saves runtime settings through the config repository; renderer saves/migrates window placement. Both target `STALKER2CameraTweaks.ini`.
- **Concrete problem / evidence:** Runtime-setting writes take `g_configPersistenceMutex`, read the whole INI and atomically replace it via `.tmp`/`MoveFileExW`. Renderer placement uses `WritePrivateProfileStringW` directly and never takes that mutex; X/Y/migration marker are separate writes. Atomic replacement protects one repository write from partial file replacement, but cannot serialize an unrelated writer or make X/Y a transaction. A setting save and drag-end save can race; which value survives is not governed by a single owner.
- **Why it matters:** Position/config may be lost, torn across X/Y, or interact unpredictably with migration and Windows INI caching. This is architectural authority duplication, not merely a formatting choice.
- **Proposed direction:** One config persistence owner and serializable update operation for all keys, including placement pair and migration marker; preserve original file on failure. A small command/transaction API is enough—do not move UI state into the game-state machine.
- **Behavior to preserve:** Existing INI location and keys, non-destructive old-overlay-INI migration, placement clamping, save-on-drag-end, customized settings, and fail-closed persistence.
- **Refactor risk:** Medium–High.
- **Dependencies:** Prefer after F-01 if the settings API is used as the path; independently test migration.
- **Validation after future refactor:** Concurrent setting/placement save test, X/Y atomicity test, migration and repeated-start test, malformed/read-only INI handling, reopen position, existing config harness.

### Config / settings / input extension model

#### F-04 — Binding and setting identity metadata is distributed across layers

- **Priority / confidence:** P2, source-established maintenance pressure, not a current functional failure.
- **Location:** `src/config/feature_config.hpp:16-86`; `src/config/feature_config.cpp:108-116,210-350`; `src/config/config_repository.cpp` loading; `src/config/config_template.cpp` managed keys; `src/plugin/runtime_settings.hpp:121-181,256-283`; `src/plugin/runtime.cpp:4118-4255,4427-4509,4513-4590`; `src/overlay/discovery_runtime.cpp:204-230`; `src/overlay/renderer_runtime.cpp:1370-1590`.
- **Current responsibility:** Config owns persisted defaults/names; plugin handles typed mutations, active behavior and persistence; input maps key actions; renderer presents controls and options.
- **Concrete problem / evidence:** A sixth hotkey would need coordinated edits to fixed five-binding conflict array, config parsing/default/template, mutation enum/variant validation, apply/persist switches, input action mapping and UI rows. Mode names/cycles also span config and UI. The duplication is identity/metadata, not the actual runtime transition logic.
- **Why it matters:** Extensions can compile while one mapping, conflict rule, persistence key or control is missed. Reviewers must trace many files to answer what one setting means.
- **Proposed direction:** Introduce a bounded descriptor/registry only for stable identities and declarative metadata (binding action, config key/default, display option identity). Keep typed mutations and game-specific apply/transition switches explicit. Avoid a generic settings framework that obscures native side effects.
- **Behavior to preserve:** Existing enum values/names, canonical selector naming, defaults, INI compatibility, hotkey conflict rules, manual/Auto locale semantics, notification behavior and per-feature transition logic.
- **Refactor risk:** Medium.
- **Dependencies:** F-01 before expanding callback API; F-03 if persistence ownership is touched; F-05 should be improved first to permit safe implementation-level changes.
- **Validation after future refactor:** Table completeness/uniqueness compile-time or harness checks; round-trip parse/serialize for every binding/mode; conflict matrix; mutation acceptance; UI option coverage; production in-game hotkey and settings regression.

## Cross-cutting findings

F-01 crosses plugin startup, overlay discovery and the settings API. F-02 crosses production cinematic policy and read-only UI projection. F-03 crosses runtime persistence, renderer placement and migration. These are boundary/authority problems rather than a reason to flatten domain modules or remove defensive checks.

### F-06 — Supported-build wording has conflicting authority

- **Priority / confidence:** P2, confirmed documentation inconsistency; exact supported-build claim needs human evidence review.
- **Location:** `docs/SUPPORTED_BUILD_MANIFEST.md:3-16`; `TESTING_AND_RESEARCH.md:95-101,233-240,442-444`.
- **Current responsibility:** Manifest states current runtime target and resolver evidence; testing document records production runtime validation.
- **Concrete problem / evidence:** The manifest's **Current runtime target** says Steam 2.0.5 while the testing document calls 2.0.6 the current production runtime-validation basis. The manifest does separately document a 2.0.6 Auto reader. Those statements may describe different subsystems, but the heading makes repository-wide support ambiguous.
- **Why it matters:** A developer cannot confidently distinguish full production support, static cross-patch resolver evidence and the narrower Auto-reader validation from the top-level manifest wording.
- **Proposed direction:** After human review of actual release evidence, make one build-support matrix authoritative with per-feature validation scopes and date/hash, and link other documents to it. Do not infer 2.0.5 or 2.0.6 support solely from this audit.
- **Behavior to preserve:** Hash as evidence rather than universal allowlist; unique signature/structural validation; fail-closed unknown builds.
- **Refactor risk:** Low (documentation), but incorrect claims have product risk.
- **Dependencies:** Evidence review; no code dependency.
- **Validation after future refactor:** Compare wording with current runtime logs, validated executable hashes, release artifact and resolver records; technical review before publishing support claim.

## 10. Code clarity and maintainability

This review applies the same concrete-harm and evidence threshold as the architecture findings. A large file, explicit switch or unusual hook callback is not a clarity defect on its own. The conclusions below are based on current production source and its build contract, not a preferred C++ style.

| Criterion | Source-level assessment |
| --- | --- |
| Naming and semantic vocabulary | Requested settings (`g_runtime...`), active cinematic/dialogue policy (`g_active...`), observational camera facts and overlay projections are generally distinguishable in `src/plugin/runtime.cpp` and `src/plugin/runtime_settings.hpp`. `Forced21x9` currently has two different numeric interpretations (F-02). The production overlay's `POC` build flag is the one additional concrete naming issue (F-07). No broad rename is justified. |
| Responsibility and organization | The private runtime integration owner connects config, native hooks and transition lifetimes, as `docs/ARCHITECTURE.md` explicitly contracts. Splitting `runtime.cpp` by length would scatter a traceable owner. Overlay discovery, renderer lifecycle, camera evidence and domain policies have meaningful boundaries. Renderer-owned placement persistence crosses the config boundary (F-03); this is a responsibility issue, not evidence that every renderer helper should move. |
| Contracts and boundaries | `RuntimeSettingsApi` provides typed mutation and readback; semantic facts carry validity, provenance and freshness. F-01 is a missing *publication/readiness* contract rather than proof that a new interface layer is needed. A full settings snapshot used by a consumer for one key could be narrowed, but no concrete cost or ownership error was established here; **needs investigation**, not a finding. |
| Data shapes and types | `RuntimeSettingMutation`, `RuntimeSettingsSnapshot` and `OverlaySemanticSnapshot` name recurring cross-boundary data. Desired, active and observed values remain distinct. `overlay::Lifecycle` (discovery) and `RendererLifecycle` (D3D12 resources) are separate real lifetimes, not gratuitous wrapper proliferation. No additional type is proposed solely for symmetry. |
| Cognitive complexity and traceability | Entry→effect paths can be followed through `InitializeThread` → `Initialize` → domain hook callbacks and through overlay input → typed mutation → runtime apply/persist. F-01 and F-03 require tracing across those boundaries; documenting or tightening those contracts improves reasoning more than mechanically extracting short helpers. Domain-specific transition switches remain explicit. |
| Abstraction levels | Guarded resolver primitives are separated from policy calculations and UI projection. The numeric policy leak into UI (F-02) is a real abstraction-boundary error. No wrapper-on-wrapper chain was found that can be shown, from source alone, to obscure a runtime effect enough to justify removal. |
| Comments and documented invariants | Comments at `src/plugin/runtime.cpp:108,124-127,680-683,1480-1497,1753-1755,3841-3843` explain validated aspect framing, ownership, fail-closed hooks, restoration timing and diagnostic-only behavior—valuable reasons, not restatements to delete. The `Forced21x9` comment makes production's 3440×1440 intent explicit and strengthens F-02. No stale production invariant comment was established in this pass. |
| Local conventions and abstraction justification | The repository consistently keeps reusable domain calculations apart from translation-unit-private integration state. A generic settings framework, blanket interface extraction or repository-wide naming/formatting convention would add indirection without demonstrated benefit. Preserve existing conventions unless a bounded finding requires change. |

### F-07 — Production overlay remains named as a rendering POC in build gates

- **Priority / confidence:** P3, confirmed naming/contract ambiguity; no demonstrated runtime defect.
- **Location:** `build.cmd:47-48`; `src/overlay/discovery_runtime.cpp:5-12,78-82,787-789`; `src/overlay/renderer_runtime.cpp:26-31,1029-1033,1301-1303`; `docs/ARCHITECTURE.md:25-26,44-58`.
- **Current responsibility:** Build flags select overlay rendering, settings frontend and combined ASI code paths. `build.cmd` is the documented production artifact entry point.
- **Concrete problem / evidence:** Production defines `OVERLAY_RENDERING_POC`, `OVERLAY_SETTINGS_FRONTEND` and `OVERLAY_COMBINED`, while architecture documentation says this combined overlay is production. `OVERLAY_RENDERING_POC` gates the actual renderer and input hook. A reader who selects profiles by flag name can misidentify production rendering as diagnostic/experimental. Empty guarded blocks at `discovery_runtime.cpp:99-100` and `renderer_runtime.cpp:1301-1302` add small amounts of stale scaffolding but are not the basis of the finding.
- **Why it matters:** The name is part of build-profile comprehension: it obscures which code is shipped and complicates review of production versus diagnostic behavior. This is a concrete contract-clarity issue, not a preference for prettier identifiers.
- **Proposed direction:** First inventory every build entry point using these flags. Then, in a bounded build-configuration change, use a production-semantic flag name or explicit compatibility alias and document the profile matrix. Remove empty guards only after confirming they have no alternate-profile role; do not turn this into a renderer rewrite.
- **Behavior to preserve:** Same compiled production code, optional overlay and fail-closed discovery, compatibility build entry points and separate diagnostic artifacts.
- **Refactor risk:** Low–Medium because preprocessor gates can silently exclude code.
- **Dependencies:** None for runtime fixes; coordinate with F-06 support/build documentation and F-05 source-pattern audit before renaming.
- **Validation after future refactor:** Compare preprocessor definitions and source lists for every supported build profile, compile production and diagnostic targets, run `test.cmd` and localization audit, inspect shipped ASI contents and perform an overlay startup smoke test.

## Extension pressure map

| Addition | Current change surface / cause | Assessment |
| --- | --- | --- |
| Gameplay mode | Config enum/parser/cycle, runtime apply/worker, gameplay coordinator and overlay options | Runtime policy branches are inherently domain-specific; metadata duplication is F-04. Do not table-drive camera transitions blindly. |
| Cinematic mode or aspect policy | Config, runtime selection/aspect resolver, cinematic tests, UI projection/options | F-02 shows duplicated numeric policy meaning; active ENTER/EXIT semantics are deliberate. |
| Dialogue mode | Config enum/parser, dialogue state/transform, runtime mutation/worker, UI options | Some cross-file edits are necessary for real behavior; option identity can be cataloged without flattening recovery logic. |
| Language | Locale registry, embedded JSON, font profile/resource and audits | Relatively well-contained declarative design; 18 catalogs and glyph audit are strengths, not a rewrite candidate. |
| Hotkey | Config fields/conflict array, runtime setting/persist/hotkey worker, input rebind, UI, notifications | Highest identity-metadata pressure (F-04). |
| Overlay control / runtime setting | Typed snapshot/mutation, handler, persistence, renderer and localized strings | Explicit type and behavior wiring is useful, but callback-publication and persistence boundaries must be sound first. |
| Notification type | Notification enum/action, publisher formatting and renderer/localization | Multiple switches are expected because each notification has behavior and copy; no confirmed high-value generic abstraction. |

## State / authority map

| Concept | Current intended authority | Replicas / ambiguity |
| --- | --- | --- |
| Persisted configuration | Main INI through `config::LoadFeatureConfig` and repository persistence | Renderer placement writes same file independently (F-03). Legacy overlay INI is migration input, not ongoing authority. |
| Requested/desired runtime setting | Plugin `g_config` for loaded/hotkey fields and typed runtime desired atomics, updated by `RuntimeSettingsApi` | Settings snapshot copies are read-only. Mutation/persist ordering should remain explicit. |
| Active cinematic/dialogue/gameplay transition | Plugin active policy snapshots, domain lifecycle/coordinator state, and validated hook callbacks | Desired != active during transitions by design; do not merge these states. |
| Observed native state | Camera FOV/baseline/restoration stores, camera snapshot and viewport resolver with provenance | May be stale/unavailable; not authority over configuration. |
| Diagnostic/presentation state | Gated diagnostics and overlay semantic projection | Overlay's second `Forced21x9` definition is unintended duplicate interpretation (F-02); otherwise snapshots are intentional observational copies. |
| Auto overlay locale | Configured Auto flag; one-shot guarded native reader at closed→open; normalized registry value for effective locale/font | Reader is not a language setter or event owner. Manual locale remains separate override. |
| Rendering/input lifecycle | Discovery worker, WindowProc, renderer/swap-chain state | Independent of plugin feature lifecycle, but settings callback publication lacks readiness ordering (F-01). |

## Legacy / dead-code candidates — needs investigation, not approved deletions

| Candidate | Evidence | Why no deletion recommendation yet |
| --- | --- | --- |
| POC-era macro names | `build.cmd:47` defines `OVERLAY_RENDERING_POC` and `OVERLAY_SETTINGS_FRONTEND` for production; corresponding branches in discovery/renderer | F-07 identifies a naming contract issue, **not** confirmed dead code. Alternate build profiles may still depend on branches; prove target matrix before any rename/removal. |
| Historical `Hotkeys.OverlayToggle` / separate overlay placement INI | `src/config/config_repository.cpp:288`; `src/overlay/renderer_runtime.cpp:110-160` | These are intentional backward-compatible migrations; do not delete without a documented support horizon and migration tests. |
| Diagnostic-only watcher and build script | `build-language-state-watcher.cmd` compiles a separate ASI and a harness outside `test.cmd` | Research artifact is intentionally not part of production. Classify/label its test scope, not as dead production code. |
| Large private `RuntimeState` and callback aliases | `src/plugin/runtime.cpp`; documented in `docs/ARCHITECTURE.md` | Hook/lifetime integration owner is deliberate. Size alone gives no evidence that another manager or file split improves correctness. |

No candidate above is confirmed safe to delete. Supported-build alternative signatures, restoration branches, provenance checks and fail-closed paths are explicitly **not** legacy merely because a current test run may not exercise them.

## Test architecture findings

### F-05 — Catalog audit mixes behavioral checks with exact renderer-source patterns

- **Priority / confidence:** P2, source-established refactor friction; existing checks also provide valuable coverage.
- **Location:** `tests/runner/localization_catalog_audit.ps1:191-231,252-275`; `test.cmd:20-24,102-108`; `tests/overlay/camera_integration_harness.cpp:72-100`.
- **Current responsibility:** The PowerShell audit checks registry/catalog parity, embedded resources, glyph coverage and selected UI/Auto-language implementation conventions; C++ harnesses test pure projections and state machines.
- **Concrete problem / evidence:** Audit success requires exact renderer tokens/formatting, including `SeparatorText(transitionHeader.c_str())`, `SetNextItemWidth(commonSelectWidth)`, specific `ImGuiTableFlags` and absence of identifiers associated with a prior visual pass. A behavior-preserving renderer refactor can fail despite identical output. Separately, the camera integration harness lacks the matching 3440/1440 `Forced21x9` case (F-02). `test.cmd` invokes 41 named harnesses; its runner audit verifies listed compile/run pairs, not that every repository harness is included. The diagnostic watcher harness is separately built by design.
- **Why it matters:** Tests can block safe refactoring while missing a high-value cross-boundary invariant. A green pure-component suite does not establish startup concurrency or renderer/config writer safety.
- **Proposed direction:** Keep genuine catalog parity/glyph/resource checks; replace exact UI source patterns where feasible with small behavioral/layout-model assertions or narrowly labeled source-policy checks. Add cross-boundary golden cases for production numeric policy and bounded concurrency tests for F-01/F-03. Do not build a full game simulator.
- **Behavior to preserve:** All 18 locale coverage, embedded-resource and glyph guarantees, Auto read on overlay open, current accepted notification/layout behavior, and existing domain regression corpus.
- **Refactor risk:** Low–Medium.
- **Dependencies:** Add F-02 regression before F-02 correction; F-01/F-03 may need test seams. Otherwise independent.
- **Validation after future refactor:** Confirm audits still detect removed locale keys, missing glyphs/resources and invalid Auto behavior; run all `test.cmd` harnesses and production build; inspect a representative in-game UI screenshot. No tests/build were run during this audit.

### Coverage boundaries and justified complexity

The suite has valuable focused contracts for resolver validation, state transitions, camera evidence, dialogue recovery, overlay lifecycle, input, localization, and config. It cannot by itself prove native game hook correctness, DXGI callback timing, FOV restoration in every game state, or D3D12 device lifetime. Those require the existing bounded runtime-validation discipline. `src/plugin/runtime.cpp` has extensive game-specific branching; there is insufficient evidence to call its state flags redundant or its hook-gate duplication accidental. Formal state diagrams/invariants may help review, but a broad state-machine rewrite would have high regression risk and is not recommended here.

## Risk matrix

| ID | Area | Priority | Refactor risk | Evidence status |
| --- | --- | --- | --- | --- |
| F-01 | Startup/settings callback publication | P0 | High | Concurrent read/write path possible by source; not reproduced |
| F-02 | Production/UI forced-aspect meaning | P1 | Medium | Numeric divergence and projected false mismatch established by source |
| F-03 | Main INI writer authority | P1 | Medium–High | Unsynchronized writers established; actual lost write not observed |
| F-04 | Setting/binding metadata distribution | P2 | Medium | Extension pressure established by source |
| F-05 | Test implementation coupling/coverage | P2 | Low–Medium | Exact-pattern requirements and missing cross-boundary case established |
| F-06 | Supported-build evidence wording | P2 | Low | Documents conflict in their current-target wording |
| F-07 | Production overlay POC flag naming | P3 | Low–Medium | Build flag and gated renderer/input paths established; alternate-profile use needs investigation |

## Recommended refactor order — proposal only

1. **Evidence/guardrail batch:** clarify F-06 with release evidence; add deterministic regression cases for F-02 and concurrency seams for F-01/F-03 without changing behavior. Preserve meaningful catalog/glyph checks while narrowing F-05's brittle patterns.
2. **Startup safety batch:** resolve F-01 with an explicit publication/readiness contract; validate early Present/input and failed overlay discovery.
3. **Presentation consistency batch:** resolve F-02 using production's authoritative aspect meaning; retain the existing actual cinematic write.
4. **Persistence authority batch:** resolve F-03 with one serialized INI owner and migration/placement transaction coverage.
5. **Post-stabilization maintainability batch:** consider F-04 only if adding settings/modes/hotkeys or if review shows repeated omissions; do not convert transition switches mechanically. Consider F-07 with an explicit build-profile inventory, after F-05's source-pattern checks are made safe for a flag rename.

These are proposed bounded batches for human review, not authorization to implement. Performance investigation is separate.

## Final audit summary — explicit answers

1. **Most important problems:** F-01 callback publication, F-02 duplicate aspect semantics, F-03 split INI writers, F-04 distributed identity metadata, F-05 tests coupled to renderer spelling.
2. **What not to refactor for neatness:** Guarded signature/hook validation, fail-closed behavior, native-observation provenance, distinct desired/active/observed states, cinematic/dialogue recovery and restoration, separate discovery/renderer lifetimes, and the process-resident `RuntimeState` integration owner. A smaller file or more wrapper types are not objectives.
3. **Current sources of truth:** Main INI/config for persisted choice; plugin typed desired/active state for live policy; domain/camera stores for observed native evidence; overlay snapshot/projection for presentation only; locale registry/catalog plus guarded UE reader for Auto overlay locale.
4. **Duplicated or ambiguous authority:** `Forced21x9` numeric meaning (F-02); main INI writers (F-03); startup readiness of settings callbacks (F-01). Read-only snapshot replication is intentional, not duplicated ownership.
5. **Best benefit at lowest runtime risk:** Correct the build-evidence wording, add the matching aspect regression, and decouple a few test assertions from exact renderer syntax. F-02's projection-only correction is also bounded once guarded by tests; F-01/F-03 require more care.
6. **Release blockers:** F-01 should be treated as a runtime-safety release blocker until its startup ordering is proven safe or fixed. F-02 is a user-visible truthfulness blocker for 3440×1440 status if that configuration is supported. F-03 warrants bounded concurrency validation before sign-off; the audit does not establish observed config loss. This report alone neither approves nor rejects release.
7. **Can wait until after v2:** F-04's metadata consolidation, most of F-05's implementation-coupling cleanup, and F-07's build-flag naming; F-06 wording should be corrected before support claims are published.
8. **Presentable to another strong C++ developer?** Yes, as a serious reverse-engineering/modding codebase with explicit constraints and substantial validation, **provided** the above boundary risks and support-evidence ambiguity are disclosed. It is not yet defensible to call the startup/persistence integration fully settled without the targeted follow-up.

**Stop condition reached:** report only. No source, test, build, resource, config, or product-documentation changes were made; no runtime, tests, build, Git or performance audit was launched.
