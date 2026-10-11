# SRC_STRUCTURE_CONSISTENCY — Full Source Tree Architecture Audit

Date: 2026-10-11  
Status: audit complete; four approved moves implemented, remaining recommendations unchanged  
Subject: current dirty working tree, HEAD `db67e4dfc56cfe06394c38413f5bb460f797d210`  
Scope: all 143 physical files under `src/`; no source, script, configuration, vendor or release edits.

## 1. Decision summary

The existing responsibility boundaries are substantially coherent. Do not repeat runtime decomposition, split large files, move native callbacks, or make every module resemble Overlay. Recommend only four mechanical file moves: group the three standalone language-state research files under `diagnostics/language_state/`, and place the common embedded-resource ID header beside the shared locale registry. No renames, deletions, merges, new APIs or abstractions.

Two bounded follow-ups are not part of mechanical implementation: review conditional experiment includes in the presenter, and decide separately whether the include-only `hook_set.cpp` placeholder is worth retaining. Neither finding establishes a production runtime defect.

B1–B7 remain complete according to their individual decisions. N01/N02 remain Fixed within recorded bounds; N03 Partially mitigated; N04/N05 Open; N06 Partially verified / Open. The retained exact-remediation-artifact live evidence gate remains Pending. This audit does not relabel later user gameplay reports or reopen closed architecture decisions.

## 2. Baseline, method and evidence limits

Enumerated physical files recursively, counted physical lines including comments/blanks, captured each file's SHA-256, parsed quoted includes and build inputs, inspected owners/interfaces and searched tests/tools references. Counts are not executable LOC. The per-file table records exact resolved project includes and direct basename references in tests/tools, not a compiler dependency trace. Conditional includes are a union, not a claim that every branch is active. Header self-sufficiency is statically reviewed, not compiler-tested.

Current `build.cmd` explicitly compiles 54 of 56 project C++ files; two other C++ files form the independent language-state watcher. The same build owns resource compilation. The supported diagnostic wrapper calls this source registry with extra defines; the settings wrapper is a compatibility alias. No active CMake or project-file target was found by filename inventory. The actual-adapter harness includes real `runtime.cpp` once; its source-list generator excludes the ordinary runtime TU and DLL entry.

Overlay's physical 51-file responsibility structure is already present: 50 paths moved and one common root header retained. Git still represents unstaged moves as deleted old paths plus untracked directories. Do not count both as source inventory. The earlier proposed Overlay reports are not the actual-tree authority; the implementation report and current files are.

The Overlay implementation report records production/diagnostic builds and 18-locale resource audit PASS, but placement persistence assertions FAIL and full `test.cmd` exit 2 (inventory discrepancy and language-feature compiler diagnostics). Causes remain undiagnosed. Those are inherited validation findings, not demonstrated directory-move regressions and not a green baseline. No tests or builds were rerun for this audit.

Read-only Git status/revision work; `git diff --check` again reports “this operation must be run in a work tree.” No staging/index workaround was attempted. Preserve dirty work and resolve review tooling before claiming rename-diff validation.

## 3. Complete actual tree

```text
src/
  camera/
    camera_state_publication.hpp
    camera_state_snapshot.cpp
    camera_state_snapshot.hpp
    fov_observation.cpp
    fov_observation.hpp
    fov_semantics.hpp
    gameplay_aspect_restoration.cpp
    gameplay_aspect_restoration.hpp
    gameplay_baseline.cpp
    gameplay_baseline.hpp
    horplus.cpp
    horplus.hpp
    presentation_state.cpp
    presentation_state.hpp
  cinematics/
    cinematic_aspect.cpp
    cinematic_aspect.hpp
    cinematic_exit_authority.hpp
    cinematic_fov.cpp
    cinematic_fov.hpp
    cinematic_initialization.cpp
    cinematic_initialization.hpp
    cinematic_recovery_retirement.hpp
    cinematic_selection.cpp
    cinematic_selection.hpp
  config/
    config_repository.cpp
    config_repository.hpp
    config_template.cpp
    config_template.hpp
    feature_config.cpp
    feature_config.hpp
  diagnostics/
    camera_log_probe.hpp
    camera_writer_identity.hpp
    cinematic_baseline_provenance.hpp
    cinematic_continuation_trace.cpp
    cinematic_continuation_trace.hpp
    cinematic_lifecycle_context.hpp
    diagnostic_runtime.cpp
    diagnostic_runtime.hpp
    dialogue_source_provenance.cpp
    dialogue_source_provenance.hpp
    language_state_transition.cpp
    language_state_transition.hpp
    language_state_watcher.cpp
    matchgameplay_prediction.hpp
    overlay_activation_trace.hpp
    performance_telemetry.cpp
    performance_telemetry.hpp
    startup_journal.hpp
    startup_timeline.hpp
  dialogue/
    dialogue_fov.cpp
    dialogue_fov.hpp
    dialogue_state.cpp
    dialogue_state.hpp
  gameplay/
    aspect_policy.cpp
    aspect_policy.hpp
    gameplay_camera.cpp
    gameplay_camera.hpp
    gameplay_state.cpp
    gameplay_state.hpp
    horplus_gameplay.cpp
    horplus_gameplay.hpp
  hooks/
    hook_set.cpp
    hook_set.hpp
    instruction_validator.cpp
    instruction_validator.hpp
    signature_scanner.cpp
    signature_scanner.hpp
    signatures/
      signature_definitions.hpp
  localization/
    locale_registry.hpp
  overlay/
    composition/
      composition_runtime.cpp
      composition_runtime.hpp
    input/
      input_state.cpp
      input_state.hpp
    localization/
      game_language_reader.cpp
      game_language_reader.hpp
      localization_catalog.cpp
      localization_catalog.hpp
      localization_font.cpp
      localization_font.hpp
      localization_formatter.cpp
      localization_formatter.hpp
      localization_keys.cpp
      localization_keys.hpp
      localization_manager.cpp
      localization_manager.hpp
      localization_resource_ids.h
      localization_resources.rc
      localization_validator.cpp
      localization_validator.hpp
    optional_overlay_boundary.hpp
    presentation/
      aspect_ratio_format.hpp
      camera_integration.cpp
      camera_integration.hpp
      camera_state_view.cpp
      camera_state_view.hpp
      feature_presentation.cpp
      feature_presentation.hpp
      hotkey_binding_presentation.hpp
      inline_hotkey_layout.hpp
      overlay_layout_metrics.cpp
      overlay_layout_metrics.hpp
      placement_config.cpp
      placement_config.hpp
      presentation_contracts.hpp
      selector_documentation_view.cpp
      selector_documentation_view.hpp
      selector_examples_geometry.hpp
      selector_tooltip_state.hpp
      setting_tooltip_content.hpp
      tooltip_row_layout.hpp
    presenter/
      composition_presenter_state.cpp
      composition_presenter_state.hpp
      imgui_d3d11_renderer.cpp
      imgui_d3d11_renderer.hpp
      placement_save_state.hpp
      presenter_runtime_policy.hpp
      renderer_runtime.cpp
      renderer_runtime.hpp
      renderer_state.cpp
      renderer_state.hpp
  platform/
    win32/
      memory.cpp
      memory.hpp
      sha256.cpp
      sha256.hpp
      viewport.cpp
      viewport.hpp
      window.cpp
      window.hpp
  plugin/
    camera_correction_gate.hpp
    camera_writer_transaction.hpp
    cinematic_lifecycle_transaction.hpp
    dialogue_boundary_transaction.hpp
    dll_entry.cpp
    feature_status.cpp
    feature_status.hpp
    optional_overlay_startup.hpp
    runtime_internal.hpp
    runtime_settings.cpp
    runtime_settings.hpp
    runtime.cpp
    runtime.hpp
    worker_lifecycle.cpp
    worker_lifecycle.hpp
```

## 4. Module metrics

| Module | cpp | hpp | Other | Physical lines | Three largest files |
| --- | ---: | ---: | ---: | ---: | --- |
| camera | 6 | 8 | 0 | 1565 | `camera_state_publication.hpp` (481); `camera_state_snapshot.cpp` (294); `camera_state_snapshot.hpp` (218) |
| cinematics | 4 | 6 | 0 | 908 | `cinematic_fov.cpp` (383); `cinematic_fov.hpp` (211); `cinematic_aspect.cpp` (83) |
| config | 3 | 3 | 0 | 2180 | `config_template.cpp` (1075); `config_repository.cpp` (528); `feature_config.cpp` (401) |
| diagnostics | 6 | 13 | 0 | 2612 | `language_state_watcher.cpp` (628); `startup_timeline.hpp` (427); `performance_telemetry.cpp` (222) |
| dialogue | 2 | 2 | 0 | 516 | `dialogue_state.cpp` (296); `dialogue_state.hpp` (137); `dialogue_fov.cpp` (74) |
| gameplay | 4 | 4 | 0 | 565 | `gameplay_state.cpp` (184); `gameplay_state.hpp` (114); `gameplay_camera.cpp` (101) |
| hooks | 3 | 4 | 0 | 307 | `signature_scanner.cpp` (106); `signature_definitions.hpp` (64); `instruction_validator.cpp` (57) |
| localization | 0 | 1 | 0 | 75 | `locale_registry.hpp` (75) |
| overlay | 19 | 30 | 2 | 8490 | `renderer_runtime.cpp` (2424); `composition_runtime.cpp` (944); `imgui_d3d11_renderer.cpp` (535) |
| platform | 4 | 4 | 0 | 215 | `sha256.cpp` (58); `viewport.cpp` (49); `memory.cpp` (47) |
| plugin | 5 | 10 | 0 | 10351 | `runtime.cpp` (6803); `camera_writer_transaction.hpp` (1875); `cinematic_lifecycle_transaction.hpp` (484) |
| **Total** | **56** | **85** | **2** | **27784** | Other: one .h and one .rc |

### Responsibilities, interfaces and owners

| Module | Responsibility / public boundary | Private or mutable owner | Hooks / workers / lifecycle | Vendor/platform and validation surfaces |
| --- | --- | --- | --- | --- |
| camera | FOV samples, snapshot/freshness/publication, baseline and restoration stores; HorPlus and semantic tolerance | Store classes contain mutex/state; instances and publication coordination belong to RuntimeState, not module-wide alternative camera owners | No hook installation or worker entry | Standard C++; diagnostics counters; camera harnesses, writer/lifecycle and replay tests |
| cinematics | Aspect/selection/FOV policy, continuation/recovery helpers; transactional installer helper | Selection and FOV contexts supplied/owned by runtime; helper rollback callback boundary | Resolver validates aspect instructions; native ENTER/EXIT/aspect callbacks and installation stay runtime | Win32, hooks decoder/scanner; Cinematic and lifecycle harnesses |
| config | feature_config.hpp model; config_repository.hpp persistence; config_template.hpp template API | Repository serialization protects persistence; effective runtime settings are not owned by INI | No camera hooks or workers | Win32/filesystem; generated canonical_ini_documentation.hpp; config and placement tests |
| diagnostics | Diagnostic mode, performance counters and trace/prediction/provenance models | diagnostic_runtime.cpp atomics; performance_telemetry.cpp counters/TLS; inline journal/trace state has explicit shared linkage | Production observations only; language watcher has separate DLL/worker/log state, excluded from production ASI | Win32/Psapi/intrinsics; diagnostic harnesses; watcher build tool |
| dialogue | dialogue_fov.hpp policy, dialogue_state.hpp lifecycle | Runtime-owned state/context, no second global coordinator | Native register boundary and reset/install remain runtime | Standard C++; config policy; Dialogue and transaction tests |
| gameplay | aspect and HorPlus decisions, state/recovery helpers; validated writer resolver | Runtime-owned replay/coordinator/source association, caller-held contexts | Native writer, zoom installation and recovery orchestration stay runtime | Win32 and Zydis indirectly through hooks; Gameplay/ownership/replay/adapter tests |
| hooks | Scanner/validator API, signatures and HookSet bundle | HookSet is a value member of runtime; Zoom hooks additionally reside directly in RuntimeState | No independent installer manager; runtime controls enable/rollback/reset | SafetyHook and Zydis; scanner/validator and native evidence fixtures |
| localization | locale_registry.hpp shared canonical identities and 18 descriptors | inline constexpr data; no mutable locale manager | None | Currently imports Overlay resource macros; config, Overlay, catalog audit, font generator |
| overlay | composition_runtime.hpp external startup/wake facade; input/presenter contracts; semantic presentation helpers; localization manager | composition_runtime.cpp discovery, renderer pointer, window continuations, wake gates and pacing handles; Renderer owns private D3D11/DComp/ImGui resources; InputState/bridge accessors own their instances; LocalizationManager is presenter-owned | One UI/GPU owner, pacing signals only; window/input subclass continuations, no game DXGI/Present hooks | ImGui, private D3D11/DComp, Win32; Overlay harnesses and localization/resource/glyph audits |
| platform/win32 | Memory validation, SHA, window discovery and viewport reads | Infrastructure functions; no correction/lifecycle authority | None | Win32/bcrypt; memory/viewport tests and production startup |
| plugin | runtime.hpp public lifecycle/settings/readback; runtime_settings.hpp typed API; feature_status.hpp capability; worker primitive | RuntimeState definition and resident allocation in runtime.cpp; runtime_internal.hpp only forward-declares owner; gate/HookSet/workers and logger belong to owner | Native adapters, startup/cleanup, install ordering, source/generation/recovery and readiness; dll_entry.cpp is loader boundary | SafetyHook/Zydis/spdlog and platform; actual adapters, owner binding, transactions, lifecycle and fault fixtures |

`runtime.cpp` (6,803 lines), `composition_runtime.cpp` (944) and `renderer_runtime.cpp` (2,424) are integration/composition files, not size-only extraction candidates. `camera_writer_transaction.hpp` (1,875) is a header-only transaction policy boundary exercised directly and via actual adapters; do not split it as formatting work.

## 5. Dependency map

The quoted-include graph has **no multi-file strongly connected component**. Module aggregation is not a layering proof: reciprocal edges exist without a header cycle.

| Consumer module | Direct other src modules |
| --- | --- |
| camera | diagnostics |
| cinematics | camera, config, hooks, plugin |
| config | localization |
| diagnostics | camera, cinematics |
| dialogue | camera, config |
| gameplay | camera, config, hooks |
| hooks | None |
| localization | overlay |
| overlay | camera, cinematics, config, diagnostics, localization, plugin |
| platform | None |
| plugin | camera, cinematics, config, diagnostics, dialogue, gameplay, hooks, localization, overlay, platform |

Material boundary observations:

- `config/feature_config.hpp:4 → localization/locale_registry.hpp:3 → overlay/localization/localization_resource_ids.h`: core configuration reaches into the optional frontend for constant IDs. Moving only the macro header to shared localization removes this misleading edge without changing IDs, locale fields or resource ownership.
- `cinematics/cinematic_initialization.hpp:3` returns `plugin::FeatureStatus`. This is an intentional integration helper, not a pure math module. Keep the pair and document it; introducing another capability enum or moving native installation would reopen settled decisions.
- Camera's instrumentation imports diagnostics; some diagnostics models import camera/Cinematics. These are observation/data dependencies, not competing authorities.
- Plugin and Overlay import each other's facades for settings and readiness/wake. Only runtime owns effective settings and correction. Overlay maintains its own UI/GPU lifecycle, not camera state.
- `runtime.hpp` transitively includes workers, feature status and settings. `runtime_settings.hpp` includes camera/config fact shapes. Possible include minimization requires a separately approved compiler/isolation task; forward-declaring value members is not a mechanical substitute.
- `locale_registry.hpp` being a one-file module is justified: identity is shared by config, persistence, UI and font/audit tooling. `hooks/signatures/` is a one-file data boundary, not useless nesting. `platform/win32/` exposes OS specificity. Keep each.
- Overlay's localization implementation and shared locale registry are distinct responsibilities, not duplicate localization owners. Placement persistence in presentation delegates to config; do not move or duplicate the INI owner.

## 6. Structural findings

| ID | Classification / confidence | Evidence | Recommendation / limitation |
| --- | --- | --- | --- |
| S01 | Non-blocking dependency/location improvement; high | Registry resource-ID edge above; IDs shared with font loader and RC | Move the unchanged macro header to src/localization. Update only three includes plus audit path lookup. Keep resource payload script in Overlay. |
| S02 | Non-blocking profile organization; high | language_state_watcher.cpp has its own DLL/worker/state; only watcher builder compiles it; transition.cpp also appears in test.cmd | Group the 3 files in diagnostics/language_state. Keep research purpose and nonproduction profile explicit. Not dead source. |
| S03 | Conditional source gap; high existence, unknown supported relevance | renderer_runtime.cpp:24–29 imports two experiment headers absent from current source tree; macros not enabled by canonical build | Review experiment gates separately. Ordinary production is unaffected by inactive includes. No removal or replacement authorized. |
| S04 | Legacy/placeholder candidate, not dead-code proof; high | hooks/hook_set.cpp consists only of include hook_set.hpp; build.cmd explicitly compiles it | Retain pending separate removal decision; maintain hook bundle and vendor lifetime. No functional duplicate implementation found. |
| S05 | Documentation drift; high | docs/ARCHITECTURE.md:148 and README.md:170 mention nonexistent build-overlay-poc.cmd; discovery script is absent too | Separate documentation-only correction if approved. Do not recreate obsolete targets or assert they ran. startup_timeline remains an actual opt-in source path. |
| S06 | Validation gap inherited from Overlay task; high report evidence | implementation report:64–68 records placement FAIL, full runner exit2, unavailable Git rename diff | Establish reproducible baseline before later implementation. Static source audit does not resolve persistence or compiler environment failures. |
| S07 | Accepted cross-domain integration; high | Cinematic initialization helper uses plugin FeatureStatus; runtime transaction headers import several domains | Document exceptions; no API move, new manager or reopening B1–B7. |
| S08 | Accepted header-state linkage; bounded static evidence | startup_journal/startup_timeline/overlay_activation_trace use inline variables; camera stores are class instances; runtime_internal.hpp has no owner definition | No concrete per-TU duplicate-owner or ODR defect established. Preserve inline/template linkage and macro uniformity. Compiler isolation/ODR testing not performed. |
| S09 | Naming/style variation; high | matchgameplay_prediction uses canonical MatchGameplay term; Windows.h/windows.h/Psapi.h spellings differ; RC includes root-relative source path | Keep established filenames and RC convention; Windows-target case variation is not a demonstrated defect. No mass normalization. |
| S10 | Historical/active-record path drift; high | old flat Overlay paths remain in earlier research/plans; physical tree and build inputs have moved | Preserve dated evidence; update only still-active operational instructions during separately approved implementation. Do not mass-rewrite history. |

No justified mass naming changes. All filenames are lowercase underscore names; `matchgameplay` is a compound domain term. Implemented cpp/hpp pairs share basename. Header-only helpers are intentional; DLL entry and watcher entry have no required same-name header.

No project quoted include was unresolved except the generated INI header and two inactive experiment headers. `canonical_ini_documentation.hpp` is generated before compile by `tools/generate_ini_documentation.ps1` into `build-artifacts/generated/`, and build.cmd adds it to INCLUDE; do not move it into src. RC root-relative include resolves through the repository-root invocation. External angle-bracket dependencies are resolved by toolchain and build include paths, not this graph.

## 7. Dead / legacy assessment

| File / group | References / build participation | Purpose / confidence | Disposition |
| --- | --- | --- | --- |
| diagnostics/language_state_watcher.cpp | tools/build/build-language-state-watcher.cmd; no build.cmd input | Standalone memory-state research ASI, high | Move/group, not delete |
| diagnostics/language_state_transition.cpp/.hpp | watcher builder; tests/diagnostics/language_state_transition_harness.cpp; test.cmd line129 | Reusable research classifier with active offline harness, high | Move/group, not delete |
| hooks/hook_set.cpp | Direct canonical build input; only includes the bundle header | Empty implementation placeholder, high | Legacy candidate; retain |
| diagnostics/startup_timeline.hpp | dll_entry/runtime includes, startup_timeline_harness, CAMERA_TWEAKS_STARTUP_TIMELINE build define | Opt-in forensic probe, high | Keep; old terminology does not establish dead source |
| renderer_runtime.cpp gated experiment sections | No matching experiment headers/active builder found | Optional experiment remnants, relevance unknown | Review containing file, not remove production renderer |
| All other cpp files | Direct build.cmd inputs | 54 total direct project C++ inputs, including placeholder | Keep unless one of the explicit moves above |
| Remaining headers | Reachable from build inputs, or research classifier; tests add direct consumers | Reachability alone does not prove every exported function is used | No file-level deletion recommendation |

No safe function-level dead-code deletion or duplicate-helper removal established. This is source organization audit, not link-time dead code analysis.

## 8. Full per-file inventory and reference ledger

Each physical file appears exactly once. Reference search covers tests/tools sources and scripts by basename; generated response-file consumers additionally derive sources from build.cmd. `Direct build input` is a literal current registry/resource input. `Reachable include/resource` is static reachability, not all-profile execution.

| File | Lines | Build participation | Resolved quoted src dependencies | Direct tests/tools reference files |
| --- | ---: | --- | --- | --- |
| `src/camera/camera_state_publication.hpp` | 481 | Reachable include/resource | `src/camera/camera_state_snapshot.hpp`<br>`src/camera/fov_observation.hpp` | `tests/camera/camera_state_snapshot_harness.cpp`<br>`tests/plugin/camera_writer_transaction_harness.cpp` |
| `src/camera/camera_state_snapshot.cpp` | 294 | Direct build input | `src/camera/camera_state_snapshot.hpp` | No direct basename reference in tests/tools |
| `src/camera/camera_state_snapshot.hpp` | 218 | Reachable include/resource | None within src | `tests/camera/camera_state_snapshot_harness.cpp` |
| `src/camera/fov_observation.cpp` | 64 | Direct build input | `src/camera/fov_observation.hpp`<br>`src/diagnostics/performance_telemetry.hpp` | No direct basename reference in tests/tools |
| `src/camera/fov_observation.hpp` | 133 | Reachable include/resource | None within src | `tests/camera/fov_observation_harness.cpp` |
| `src/camera/fov_semantics.hpp` | 16 | Reachable include/resource | None within src | `tests/dialogue/recovery_rearm_harness.cpp`<br>`tests/gameplay/gameplay_mode_transition_harness.cpp` |
| `src/camera/gameplay_aspect_restoration.cpp` | 81 | Direct build input | `src/camera/gameplay_aspect_restoration.hpp`<br>`src/diagnostics/performance_telemetry.hpp` | No direct basename reference in tests/tools |
| `src/camera/gameplay_aspect_restoration.hpp` | 86 | Reachable include/resource | `src/camera/fov_observation.hpp` | `tests/camera/gameplay_aspect_restoration_harness.cpp`<br>`tests/regression/runtime_evidence_replay_harness.cpp` |
| `src/camera/gameplay_baseline.cpp` | 89 | Direct build input | `src/camera/gameplay_baseline.hpp`<br>`src/diagnostics/performance_telemetry.hpp` | No direct basename reference in tests/tools |
| `src/camera/gameplay_baseline.hpp` | 55 | Reachable include/resource | `src/camera/fov_observation.hpp` | `tests/camera/camera_state_snapshot_harness.cpp`<br>`tests/camera/gameplay_baseline_harness.cpp`<br>`tests/cinematics/horplus_post_exit_recovery_harness.cpp`<br>`tests/regression/runtime_evidence_replay_harness.cpp` |
| `src/camera/horplus.cpp` | 13 | Direct build input | `src/camera/horplus.hpp` | No direct basename reference in tests/tools |
| `src/camera/horplus.hpp` | 6 | Reachable include/resource | None within src | `tests/cinematics/horplus_post_exit_recovery_harness.cpp`<br>`tests/gameplay/gameplay_mode_transition_harness.cpp`<br>`tests/gameplay/horplus_gameplay_harness.cpp`<br>`tests/regression/module_ownership_harness.cpp` |
| `src/camera/presentation_state.cpp` | 14 | Direct build input | `src/camera/presentation_state.hpp` | No direct basename reference in tests/tools |
| `src/camera/presentation_state.hpp` | 15 | Reachable include/resource | None within src | `tests/regression/module_ownership_harness.cpp` |
| `src/cinematics/cinematic_aspect.cpp` | 83 | Direct build input | `src/cinematics/cinematic_aspect.hpp`<br>`src/hooks/instruction_validator.hpp`<br>`src/hooks/signature_scanner.hpp` | No direct basename reference in tests/tools |
| `src/cinematics/cinematic_aspect.hpp` | 44 | Reachable include/resource | `src/config/feature_config.hpp` | `tests/cinematics/cinematic_aspect_harness.cpp` |
| `src/cinematics/cinematic_exit_authority.hpp` | 32 | Reachable include/resource | `src/camera/presentation_state.hpp` | `tests/cinematics/cinematic_exit_authority_harness.cpp`<br>`tests/diagnostics/cinematic_lifecycle_context_harness.cpp` |
| `src/cinematics/cinematic_fov.cpp` | 383 | Direct build input | `src/cinematics/cinematic_fov.hpp`<br>`src/camera/fov_semantics.hpp`<br>`src/camera/horplus.hpp` | No direct basename reference in tests/tools |
| `src/cinematics/cinematic_fov.hpp` | 211 | Reachable include/resource | `src/camera/gameplay_baseline.hpp`<br>`src/camera/presentation_state.hpp`<br>`src/config/feature_config.hpp` | `tests/cinematics/cinematic_fov_harness.cpp`<br>`tests/cinematics/horplus_post_exit_recovery_harness.cpp`<br>`tests/gameplay/gameplay_mode_transition_harness.cpp`<br>`tests/gameplay/horplus_gameplay_harness.cpp`<br>`tests/regression/module_ownership_harness.cpp`<br>`tests/regression/runtime_evidence_replay_harness.cpp` |
| `src/cinematics/cinematic_initialization.cpp` | 28 | Direct build input | `src/cinematics/cinematic_initialization.hpp` | No direct basename reference in tests/tools |
| `src/cinematics/cinematic_initialization.hpp` | 15 | Reachable include/resource | `src/plugin/feature_status.hpp` | `tests/cinematics/cinematic_initialization_harness.cpp` |
| `src/cinematics/cinematic_recovery_retirement.hpp` | 40 | Reachable include/resource | `src/cinematics/cinematic_fov.hpp` | `tests/cinematics/horplus_post_exit_recovery_harness.cpp` |
| `src/cinematics/cinematic_selection.cpp` | 24 | Direct build input | `src/cinematics/cinematic_selection.hpp` | No direct basename reference in tests/tools |
| `src/cinematics/cinematic_selection.hpp` | 48 | Reachable include/resource | `src/config/feature_config.hpp` | `tests/cinematics/cinematic_selection_harness.cpp`<br>`tests/gameplay/gameplay_mode_transition_harness.cpp`<br>`tests/regression/module_ownership_harness.cpp` |
| `src/config/config_repository.cpp` | 528 | Direct build input | `src/config/config_repository.hpp`<br>`src/config/feature_config.hpp` | No direct basename reference in tests/tools |
| `src/config/config_repository.hpp` | 31 | Reachable include/resource | `src/config/feature_config.hpp` | `tests/config/config_persistence_harness.cpp`<br>`tests/overlay/placement_config_harness.cpp` |
| `src/config/config_template.cpp` | 1075 | Direct build input | `src/config/config_template.hpp`<br>`src/config/config_repository.hpp`<br>`src/config/feature_config.hpp` | No direct basename reference in tests/tools |
| `src/config/config_template.hpp` | 13 | Reachable include/resource | None within src | `tests/config/config_persistence_harness.cpp` |
| `src/config/feature_config.cpp` | 401 | Direct build input | `src/config/feature_config.hpp` | No direct basename reference in tests/tools |
| `src/config/feature_config.hpp` | 132 | Reachable include/resource | `src/localization/locale_registry.hpp` | `tests/config/gameplay_mode_harness.cpp`<br>`tests/overlay/localization_harness.cpp` |
| `src/diagnostics/camera_log_probe.hpp` | 25 | Reachable include/resource | None within src | `tests/diagnostics/diagnostics_gate_harness.cpp` |
| `src/diagnostics/camera_writer_identity.hpp` | 91 | Reachable include/resource | `src/cinematics/cinematic_fov.hpp` | `tests/diagnostics/camera_writer_identity_harness.cpp` |
| `src/diagnostics/cinematic_baseline_provenance.hpp` | 85 | Reachable include/resource | `src/camera/gameplay_baseline.hpp`<br>`src/cinematics/cinematic_fov.hpp` | `tests/cinematics/cinematic_fov_harness.cpp` |
| `src/diagnostics/cinematic_continuation_trace.cpp` | 208 | Direct build input | `src/diagnostics/cinematic_continuation_trace.hpp` | No direct basename reference in tests/tools |
| `src/diagnostics/cinematic_continuation_trace.hpp` | 122 | Reachable include/resource | None within src | `tests/cinematics/cinematic_fov_harness.cpp` |
| `src/diagnostics/cinematic_lifecycle_context.hpp` | 117 | Reachable include/resource | None within src | `tests/diagnostics/cinematic_lifecycle_context_harness.cpp` |
| `src/diagnostics/diagnostic_runtime.cpp` | 21 | Direct build input | `src/diagnostics/diagnostic_runtime.hpp` | No direct basename reference in tests/tools |
| `src/diagnostics/diagnostic_runtime.hpp` | 17 | Reachable include/resource | None within src | `tests/diagnostics/diagnostics_gate_harness.cpp`<br>`tests/diagnostics/performance_telemetry_harness.cpp`<br>`tests/overlay/activation_trace_harness.cpp`<br>`tests/plugin/cinematic_lifecycle_transaction_harness.cpp` |
| `src/diagnostics/dialogue_source_provenance.cpp` | 99 | Direct build input | `src/diagnostics/dialogue_source_provenance.hpp` | No direct basename reference in tests/tools |
| `src/diagnostics/dialogue_source_provenance.hpp` | 77 | Reachable include/resource | None within src | `tests/diagnostics/dialogue_source_provenance_harness.cpp` |
| `src/diagnostics/language_state_transition.cpp` | 37 | Research/test only | `src/diagnostics/language_state_transition.hpp` | `tools/build/build-language-state-watcher.cmd` |
| `src/diagnostics/language_state_transition.hpp` | 33 | Research/test only | None within src | `tests/diagnostics/language_state_transition_harness.cpp` |
| `src/diagnostics/language_state_watcher.cpp` | 628 | Research/test only | `src/diagnostics/language_state_transition.hpp` | `tools/build/build-language-state-watcher.cmd` |
| `src/diagnostics/matchgameplay_prediction.hpp` | 79 | Reachable include/resource | None within src | `tests/diagnostics/matchgameplay_prediction_harness.cpp` |
| `src/diagnostics/overlay_activation_trace.hpp` | 100 | Reachable include/resource | `src/diagnostics/diagnostic_runtime.hpp` | `tests/overlay/activation_trace_harness.cpp` |
| `src/diagnostics/performance_telemetry.cpp` | 222 | Direct build input | `src/diagnostics/performance_telemetry.hpp`<br>`src/diagnostics/diagnostic_runtime.hpp` | No direct basename reference in tests/tools |
| `src/diagnostics/performance_telemetry.hpp` | 72 | Reachable include/resource | None within src | `tests/diagnostics/performance_telemetry_harness.cpp` |
| `src/diagnostics/startup_journal.hpp` | 152 | Reachable include/resource | None within src | `tests/diagnostics/startup_journal_harness.cpp` |
| `src/diagnostics/startup_timeline.hpp` | 427 | Reachable include/resource | None within src | `tests/overlay/startup_timeline_harness.cpp` |
| `src/dialogue/dialogue_fov.cpp` | 74 | Direct build input | `src/dialogue/dialogue_fov.hpp` | No direct basename reference in tests/tools |
| `src/dialogue/dialogue_fov.hpp` | 9 | Reachable include/resource | None within src | `tests/dialogue/dialogue_fov_harness.cpp`<br>`tests/regression/module_ownership_harness.cpp` |
| `src/dialogue/dialogue_state.cpp` | 296 | Direct build input | `src/dialogue/dialogue_state.hpp`<br>`src/camera/fov_semantics.hpp` | No direct basename reference in tests/tools |
| `src/dialogue/dialogue_state.hpp` | 137 | Reachable include/resource | `src/config/feature_config.hpp` | `tests/cinematics/horplus_post_exit_recovery_harness.cpp`<br>`tests/dialogue/candidate_hardening_harness.cpp`<br>`tests/dialogue/policy_snapshot_harness.cpp`<br>`tests/dialogue/post_cinematic_exclusion_harness.cpp`<br>`tests/dialogue/recovery_rearm_harness.cpp`<br>`tests/plugin/camera_correction_gate_harness.cpp`<br>`tests/regression/module_ownership_harness.cpp`<br>`tests/regression/runtime_evidence_replay_harness.cpp` |
| `src/gameplay/aspect_policy.cpp` | 16 | Direct build input | `src/gameplay/aspect_policy.hpp` | No direct basename reference in tests/tools |
| `src/gameplay/aspect_policy.hpp` | 9 | Reachable include/resource | None within src | `tests/gameplay/aspect_policy_harness.cpp` |
| `src/gameplay/gameplay_camera.cpp` | 101 | Direct build input | `src/gameplay/gameplay_camera.hpp`<br>`src/hooks/instruction_validator.hpp`<br>`src/hooks/signatures/signature_definitions.hpp` | No direct basename reference in tests/tools |
| `src/gameplay/gameplay_camera.hpp` | 40 | Reachable include/resource | None within src | `tests/gameplay/gameplay_camera_resolver_harness.cpp`<br>`tests/gameplay/gameplay_camera_safety_harness.cpp`<br>`tests/plugin/runtime_native_adapter_harness.cpp` |
| `src/gameplay/gameplay_state.cpp` | 184 | Direct build input | `src/gameplay/gameplay_state.hpp`<br>`src/camera/fov_semantics.hpp` | No direct basename reference in tests/tools |
| `src/gameplay/gameplay_state.hpp` | 114 | Reachable include/resource | `src/config/feature_config.hpp`<br>`src/camera/presentation_state.hpp` | `tests/cinematics/coordinator_recovery_harness.cpp`<br>`tests/cinematics/horplus_post_exit_recovery_harness.cpp`<br>`tests/gameplay/gameplay_mode_transition_harness.cpp`<br>`tests/plugin/camera_writer_transaction_harness.cpp`<br>`tests/regression/module_ownership_harness.cpp`<br>`tests/regression/runtime_evidence_replay_harness.cpp` |
| `src/gameplay/horplus_gameplay.cpp` | 81 | Direct build input | `src/gameplay/horplus_gameplay.hpp`<br>`src/gameplay/aspect_policy.hpp`<br>`src/gameplay/gameplay_state.hpp`<br>`src/camera/horplus.hpp` | No direct basename reference in tests/tools |
| `src/gameplay/horplus_gameplay.hpp` | 20 | Reachable include/resource | None within src | `tests/cinematics/cinematic_exit_authority_harness.cpp`<br>`tests/cinematics/horplus_post_exit_recovery_harness.cpp`<br>`tests/gameplay/horplus_gameplay_harness.cpp`<br>`tests/gameplay/zoom_transition_harness.cpp`<br>`tests/plugin/camera_writer_transaction_harness.cpp`<br>`tests/plugin/cinematic_lifecycle_transaction_harness.cpp`<br>`tests/regression/runtime_evidence_replay_harness.cpp` |
| `src/hooks/hook_set.cpp` | 1 | Direct build input | `src/hooks/hook_set.hpp` | No direct basename reference in tests/tools |
| `src/hooks/hook_set.hpp` | 29 | Reachable include/resource | None within src | No direct basename reference in tests/tools |
| `src/hooks/instruction_validator.cpp` | 57 | Direct build input | `src/hooks/instruction_validator.hpp` | No direct basename reference in tests/tools |
| `src/hooks/instruction_validator.hpp` | 19 | Reachable include/resource | `src/hooks/signature_scanner.hpp` | `tests/platform/instruction_validator_harness.cpp` |
| `src/hooks/signature_scanner.cpp` | 106 | Direct build input | `src/hooks/signature_scanner.hpp` | No direct basename reference in tests/tools |
| `src/hooks/signature_scanner.hpp` | 31 | Reachable include/resource | None within src | `tests/platform/signature_scanner_harness.cpp` |
| `src/hooks/signatures/signature_definitions.hpp` | 64 | Reachable include/resource | None within src | `tests/gameplay/gameplay_camera_resolver_harness.cpp`<br>`tests/platform/signature_scanner_harness.cpp` |
| `src/localization/locale_registry.hpp` | 75 | Reachable include/resource | `src/overlay/localization/localization_resource_ids.h` | `tests/runner/localization_catalog_audit.ps1`<br>`tools/fonts/rebuild_catalog_subsets.py` |
| `src/overlay/composition/composition_runtime.cpp` | 944 | Direct build input | `src/overlay/composition/composition_runtime.hpp`<br>`src/overlay/localization/game_language_reader.hpp`<br>`src/overlay/input/input_state.hpp`<br>`src/overlay/optional_overlay_boundary.hpp`<br>`src/overlay/presenter/presenter_runtime_policy.hpp`<br>`src/overlay/presentation/presentation_contracts.hpp`<br>`src/overlay/presenter/renderer_runtime.hpp`<br>`src/diagnostics/diagnostic_runtime.hpp`<br>`src/diagnostics/overlay_activation_trace.hpp`<br>`src/diagnostics/performance_telemetry.hpp`<br>`src/diagnostics/startup_journal.hpp`<br>`src/plugin/runtime.hpp`<br>`src/config/feature_config.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/composition/composition_runtime.hpp` | 8 | Reachable include/resource | None within src | No direct basename reference in tests/tools |
| `src/overlay/input/input_state.cpp` | 339 | Direct build input | `src/overlay/input/input_state.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/input/input_state.hpp` | 176 | Reachable include/resource | `src/config/feature_config.hpp` | `tests/overlay/composition_presenter_state_harness.cpp`<br>`tests/overlay/input_state_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/localization/game_language_reader.cpp` | 184 | Direct build input | `src/overlay/localization/game_language_reader.hpp`<br>`src/localization/locale_registry.hpp`<br>`src/plugin/runtime.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/localization/game_language_reader.hpp` | 18 | Reachable include/resource | None within src | `tests/overlay/localization_harness.cpp` |
| `src/overlay/localization/localization_catalog.cpp` | 223 | Direct build input | `src/overlay/localization/localization_catalog.hpp` | No direct basename reference in tests/tools |
| `src/overlay/localization/localization_catalog.hpp` | 34 | Reachable include/resource | `src/overlay/localization/localization_keys.hpp` | `tests/overlay/localization_harness.cpp` |
| `src/overlay/localization/localization_font.cpp` | 254 | Direct build input | `src/overlay/localization/localization_font.hpp`<br>`src/overlay/localization/localization_resource_ids.h`<br>`src/localization/locale_registry.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/localization/localization_font.hpp` | 16 | Reachable include/resource | None within src | `tests/overlay/localization_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/localization/localization_formatter.cpp` | 53 | Direct build input | `src/overlay/localization/localization_formatter.hpp` | No direct basename reference in tests/tools |
| `src/overlay/localization/localization_formatter.hpp` | 20 | Reachable include/resource | None within src | `tests/overlay/localization_harness.cpp` |
| `src/overlay/localization/localization_keys.cpp` | 133 | Direct build input | `src/overlay/localization/localization_keys.hpp` | No direct basename reference in tests/tools |
| `src/overlay/localization/localization_keys.hpp` | 254 | Reachable include/resource | None within src | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/localization/localization_manager.cpp` | 117 | Direct build input | `src/overlay/localization/localization_manager.hpp`<br>`src/overlay/localization/localization_validator.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/localization/localization_manager.hpp` | 42 | Reachable include/resource | `src/overlay/localization/localization_catalog.hpp`<br>`src/overlay/localization/localization_formatter.hpp`<br>`src/localization/locale_registry.hpp` | `tests/overlay/localization_harness.cpp` |
| `src/overlay/localization/localization_resource_ids.h` | 31 | Reachable include/resource | None within src | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/localization/localization_resources.rc` | 31 | Direct build input | `src/overlay/localization/localization_resource_ids.h` | `tests/runner/localization_catalog_audit.ps1`<br>`tests/runner/native_runtime_evidence.ps1` |
| `src/overlay/localization/localization_validator.cpp` | 59 | Direct build input | `src/overlay/localization/localization_validator.hpp`<br>`src/overlay/localization/localization_formatter.hpp` | No direct basename reference in tests/tools |
| `src/overlay/localization/localization_validator.hpp` | 19 | Reachable include/resource | `src/overlay/localization/localization_catalog.hpp` | `tests/overlay/localization_harness.cpp` |
| `src/overlay/optional_overlay_boundary.hpp` | 33 | Reachable include/resource | None within src | No direct basename reference in tests/tools |
| `src/overlay/presentation/aspect_ratio_format.hpp` | 23 | Reachable include/resource | None within src | `tests/overlay/camera_integration_harness.cpp` |
| `src/overlay/presentation/camera_integration.cpp` | 383 | Direct build input | `src/overlay/presentation/camera_integration.hpp`<br>`src/camera/fov_semantics.hpp`<br>`src/camera/horplus.hpp`<br>`src/cinematics/cinematic_aspect.hpp` | No direct basename reference in tests/tools |
| `src/overlay/presentation/camera_integration.hpp` | 127 | Reachable include/resource | `src/plugin/runtime_settings.hpp` | `tests/overlay/camera_integration_harness.cpp`<br>`tests/plugin/camera_writer_transaction_harness.cpp` |
| `src/overlay/presentation/camera_state_view.cpp` | 311 | Direct build input | `src/overlay/presentation/camera_state_view.hpp`<br>`src/overlay/presentation/aspect_ratio_format.hpp`<br>`src/overlay/presentation/camera_integration.hpp`<br>`src/camera/camera_state_snapshot.hpp`<br>`src/config/feature_config.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/camera_state_view.hpp` | 10 | Reachable include/resource | `src/overlay/presentation/camera_integration.hpp`<br>`src/plugin/runtime_settings.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/feature_presentation.cpp` | 194 | Direct build input | `src/overlay/presentation/feature_presentation.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/feature_presentation.hpp` | 44 | Reachable include/resource | `src/plugin/runtime_settings.hpp`<br>`src/overlay/localization/localization_keys.hpp` | `tests/overlay/feature_presentation_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/hotkey_binding_presentation.hpp` | 47 | Reachable include/resource | `src/config/feature_config.hpp`<br>`src/plugin/runtime_settings.hpp`<br>`src/overlay/localization/localization_keys.hpp` | `tests/overlay/input_state_harness.cpp` |
| `src/overlay/presentation/inline_hotkey_layout.hpp` | 29 | Reachable include/resource | None within src | `tests/overlay/localization_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/overlay_layout_metrics.cpp` | 47 | Direct build input | `src/overlay/presentation/overlay_layout_metrics.hpp` | No direct basename reference in tests/tools |
| `src/overlay/presentation/overlay_layout_metrics.hpp` | 16 | Reachable include/resource | None within src | `tests/overlay/overlay_layout_metrics_harness.cpp` |
| `src/overlay/presentation/placement_config.cpp` | 21 | Direct build input | `src/overlay/presentation/placement_config.hpp`<br>`src/config/config_repository.hpp` | No direct basename reference in tests/tools |
| `src/overlay/presentation/placement_config.hpp` | 10 | Reachable include/resource | None within src | `tests/overlay/placement_config_harness.cpp` |
| `src/overlay/presentation/presentation_contracts.hpp` | 113 | Reachable include/resource | None within src | `tests/overlay/composition_presenter_state_harness.cpp` |
| `src/overlay/presentation/selector_documentation_view.cpp` | 138 | Direct build input | `src/overlay/presentation/selector_documentation_view.hpp`<br>`src/overlay/localization/localization_keys.hpp`<br>`src/overlay/presentation/selector_examples_geometry.hpp`<br>`src/overlay/presentation/tooltip_row_layout.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/selector_documentation_view.hpp` | 14 | Reachable include/resource | `src/overlay/localization/localization_manager.hpp`<br>`src/overlay/presentation/selector_tooltip_state.hpp`<br>`src/overlay/presentation/setting_tooltip_content.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/selector_examples_geometry.hpp` | 39 | Reachable include/resource | None within src | `tests/overlay/localization_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/selector_tooltip_state.hpp` | 17 | Reachable include/resource | None within src | `tests/overlay/selector_tooltip_state_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/setting_tooltip_content.hpp` | 161 | Reachable include/resource | `src/config/feature_config.hpp`<br>`src/overlay/localization/localization_keys.hpp` | `tests/overlay/localization_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presentation/tooltip_row_layout.hpp` | 43 | Reachable include/resource | None within src | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presenter/composition_presenter_state.cpp` | 118 | Direct build input | `src/overlay/presenter/composition_presenter_state.hpp` | No direct basename reference in tests/tools |
| `src/overlay/presenter/composition_presenter_state.hpp` | 85 | Reachable include/resource | None within src | `tests/overlay/composition_presenter_state_harness.cpp` |
| `src/overlay/presenter/imgui_d3d11_renderer.cpp` | 535 | Direct build input | `src/overlay/presenter/imgui_d3d11_renderer.hpp` | No direct basename reference in tests/tools |
| `src/overlay/presenter/imgui_d3d11_renderer.hpp` | 56 | Reachable include/resource | `src/overlay/presentation/presentation_contracts.hpp` | No direct basename reference in tests/tools |
| `src/overlay/presenter/placement_save_state.hpp` | 41 | Reachable include/resource | None within src | `tests/overlay/placement_config_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presenter/presenter_runtime_policy.hpp` | 132 | Reachable include/resource | None within src | `tests/overlay/composition_presenter_state_harness.cpp`<br>`tests/plugin/camera_writer_transaction_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presenter/renderer_runtime.cpp` | 2424 | Direct build input | `src/overlay/presenter/renderer_runtime.hpp`<br>`src/overlay/optional_overlay_boundary.hpp`<br>`src/diagnostics/diagnostic_runtime.hpp`<br>`src/diagnostics/overlay_activation_trace.hpp`<br>`src/diagnostics/startup_journal.hpp`<br>`src/overlay/input/input_state.hpp`<br>`src/overlay/localization/game_language_reader.hpp`<br>`src/overlay/localization/localization_font.hpp`<br>`src/overlay/presentation/selector_tooltip_state.hpp`<br>`src/overlay/presentation/setting_tooltip_content.hpp`<br>`src/overlay/presentation/selector_documentation_view.hpp`<br>`src/overlay/presentation/tooltip_row_layout.hpp`<br>`src/overlay/presentation/camera_state_view.hpp`<br>`src/overlay/presentation/aspect_ratio_format.hpp`<br>`src/overlay/presentation/overlay_layout_metrics.hpp`<br>`src/overlay/presentation/placement_config.hpp`<br>`src/overlay/presentation/hotkey_binding_presentation.hpp`<br>`src/plugin/runtime.hpp`<br>`src/config/feature_config.hpp`<br>`src/overlay/presentation/camera_integration.hpp`<br>`src/overlay/presentation/feature_presentation.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presenter/renderer_runtime.hpp` | 183 | Reachable include/resource | `src/overlay/presenter/composition_presenter_state.hpp`<br>`src/overlay/presenter/placement_save_state.hpp`<br>`src/overlay/presenter/renderer_state.hpp`<br>`src/overlay/presentation/presentation_contracts.hpp`<br>`src/overlay/presentation/inline_hotkey_layout.hpp`<br>`src/overlay/input/input_state.hpp`<br>`src/overlay/localization/localization_manager.hpp`<br>`src/overlay/presenter/imgui_d3d11_renderer.hpp`<br>`src/plugin/runtime_settings.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presenter/renderer_state.cpp` | 75 | Direct build input | `src/overlay/presenter/renderer_state.hpp` | `tests/runner/localization_catalog_audit.ps1` |
| `src/overlay/presenter/renderer_state.hpp` | 66 | Reachable include/resource | None within src | `tests/overlay/composition_presenter_state_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/platform/win32/memory.cpp` | 47 | Direct build input | `src/platform/win32/memory.hpp` | No direct basename reference in tests/tools |
| `src/platform/win32/memory.hpp` | 10 | Reachable include/resource | None within src | `tests/platform/memory_harness.cpp`<br>`tests/plugin/runtime_native_adapter_harness.cpp` |
| `src/platform/win32/sha256.cpp` | 58 | Direct build input | `src/platform/win32/sha256.hpp` | No direct basename reference in tests/tools |
| `src/platform/win32/sha256.hpp` | 9 | Reachable include/resource | None within src | No direct basename reference in tests/tools |
| `src/platform/win32/viewport.cpp` | 49 | Direct build input | `src/platform/win32/viewport.hpp`<br>`src/platform/win32/window.hpp` | No direct basename reference in tests/tools |
| `src/platform/win32/viewport.hpp` | 8 | Reachable include/resource | None within src | `tests/platform/viewport_harness.cpp`<br>`tests/plugin/runtime_native_adapter_harness.cpp` |
| `src/platform/win32/window.cpp` | 26 | Direct build input | `src/platform/win32/window.hpp` | No direct basename reference in tests/tools |
| `src/platform/win32/window.hpp` | 8 | Reachable include/resource | None within src | No direct basename reference in tests/tools |
| `src/plugin/camera_correction_gate.hpp` | 72 | Reachable include/resource | None within src | `tests/plugin/camera_correction_gate_harness.cpp` |
| `src/plugin/camera_writer_transaction.hpp` | 1875 | Reachable include/resource | `src/camera/presentation_state.hpp`<br>`src/camera/gameplay_aspect_restoration.hpp`<br>`src/camera/gameplay_baseline.hpp`<br>`src/camera/fov_observation.hpp`<br>`src/camera/fov_semantics.hpp`<br>`src/camera/camera_state_publication.hpp`<br>`src/cinematics/cinematic_fov.hpp`<br>`src/cinematics/cinematic_recovery_retirement.hpp`<br>`src/dialogue/dialogue_state.hpp`<br>`src/plugin/camera_correction_gate.hpp`<br>`src/plugin/runtime_settings.hpp`<br>`src/gameplay/horplus_gameplay.hpp`<br>`src/gameplay/aspect_policy.hpp`<br>`src/gameplay/gameplay_state.hpp`<br>`src/gameplay/gameplay_camera.hpp` | `tests/plugin/camera_setting_transaction_harness.cpp`<br>`tests/plugin/camera_writer_transaction_harness.cpp`<br>`tests/plugin/dialogue_boundary_transaction_harness.cpp`<br>`tests/plugin/runtime_native_adapter_harness.cpp` |
| `src/plugin/cinematic_lifecycle_transaction.hpp` | 484 | Reachable include/resource | `src/plugin/camera_writer_transaction.hpp`<br>`src/gameplay/gameplay_state.hpp`<br>`src/cinematics/cinematic_fov.hpp`<br>`src/cinematics/cinematic_aspect.hpp`<br>`src/cinematics/cinematic_selection.hpp`<br>`src/cinematics/cinematic_exit_authority.hpp` | `tests/plugin/camera_writer_transaction_harness.cpp`<br>`tests/plugin/cinematic_lifecycle_transaction_harness.cpp` |
| `src/plugin/dialogue_boundary_transaction.hpp` | 313 | Reachable include/resource | `src/plugin/camera_correction_gate.hpp`<br>`src/plugin/runtime_settings.hpp`<br>`src/camera/fov_semantics.hpp`<br>`src/camera/presentation_state.hpp`<br>`src/config/feature_config.hpp`<br>`src/dialogue/dialogue_state.hpp`<br>`src/dialogue/dialogue_fov.hpp` | `tests/plugin/cinematic_lifecycle_transaction_harness.cpp`<br>`tests/plugin/dialogue_boundary_transaction_harness.cpp` |
| `src/plugin/dll_entry.cpp` | 33 | Direct build input | `src/plugin/runtime.hpp`<br>`src/diagnostics/startup_journal.hpp`<br>`src/diagnostics/startup_timeline.hpp` | `tests/runner/runtime_native_adapter_inputs.ps1` |
| `src/plugin/feature_status.cpp` | 21 | Direct build input | `src/plugin/feature_status.hpp` | No direct basename reference in tests/tools |
| `src/plugin/feature_status.hpp` | 16 | Reachable include/resource | None within src | `tests/feature_status/feature_status_harness.cpp` |
| `src/plugin/optional_overlay_startup.hpp` | 51 | Reachable include/resource | None within src | `tests/plugin/optional_overlay_startup_harness.cpp` |
| `src/plugin/runtime_internal.hpp` | 25 | Reachable include/resource | `src/plugin/runtime_settings.hpp`<br>`src/config/config_repository.hpp` | `tests/plugin/runtime_owner_binding_probe.cpp` |
| `src/plugin/runtime_settings.cpp` | 120 | Direct build input | `src/plugin/runtime_internal.hpp`<br>`src/config/feature_config.hpp`<br>`src/localization/locale_registry.hpp` | No direct basename reference in tests/tools |
| `src/plugin/runtime_settings.hpp` | 371 | Reachable include/resource | `src/camera/camera_state_snapshot.hpp`<br>`src/camera/fov_observation.hpp`<br>`src/config/feature_config.hpp` | `tests/config/runtime_settings_api_harness.cpp`<br>`tests/overlay/semantic_snapshot_harness.cpp` |
| `src/plugin/runtime.cpp` | 6803 | Direct build input | `src/plugin/runtime.hpp`<br>`src/plugin/runtime_internal.hpp`<br>`src/overlay/composition/composition_runtime.hpp`<br>`src/plugin/optional_overlay_startup.hpp`<br>`src/plugin/runtime_settings.hpp`<br>`src/plugin/camera_correction_gate.hpp`<br>`src/plugin/camera_writer_transaction.hpp`<br>`src/plugin/dialogue_boundary_transaction.hpp`<br>`src/plugin/cinematic_lifecycle_transaction.hpp`<br>`src/config/feature_config.hpp`<br>`src/config/config_repository.hpp`<br>`src/config/config_template.hpp`<br>`src/hooks/signatures/signature_definitions.hpp`<br>`src/hooks/signature_scanner.hpp`<br>`src/hooks/instruction_validator.hpp`<br>`src/hooks/hook_set.hpp`<br>`src/gameplay/gameplay_state.hpp`<br>`src/gameplay/gameplay_camera.hpp`<br>`src/gameplay/horplus_gameplay.hpp`<br>`src/gameplay/aspect_policy.hpp`<br>`src/cinematics/cinematic_initialization.hpp`<br>`src/cinematics/cinematic_fov.hpp`<br>`src/cinematics/cinematic_exit_authority.hpp`<br>`src/cinematics/cinematic_recovery_retirement.hpp`<br>`src/cinematics/cinematic_selection.hpp`<br>`src/diagnostics/matchgameplay_prediction.hpp`<br>`src/cinematics/cinematic_aspect.hpp`<br>`src/dialogue/dialogue_fov.hpp`<br>`src/dialogue/dialogue_state.hpp`<br>`src/diagnostics/diagnostic_runtime.hpp`<br>`src/diagnostics/camera_writer_identity.hpp`<br>`src/diagnostics/camera_log_probe.hpp`<br>`src/diagnostics/cinematic_continuation_trace.hpp`<br>`src/diagnostics/cinematic_baseline_provenance.hpp`<br>`src/diagnostics/dialogue_source_provenance.hpp`<br>`src/diagnostics/cinematic_lifecycle_context.hpp`<br>`src/diagnostics/startup_journal.hpp`<br>`src/diagnostics/startup_timeline.hpp`<br>`src/diagnostics/performance_telemetry.hpp`<br>`src/camera/fov_observation.hpp`<br>`src/camera/gameplay_baseline.hpp`<br>`src/camera/gameplay_aspect_restoration.hpp`<br>`src/camera/horplus.hpp`<br>`src/camera/presentation_state.hpp`<br>`src/camera/camera_state_snapshot.hpp`<br>`src/camera/camera_state_publication.hpp`<br>`src/camera/fov_semantics.hpp`<br>`src/platform/win32/memory.hpp`<br>`src/platform/win32/sha256.hpp`<br>`src/platform/win32/viewport.hpp`<br>`src/platform/win32/window.hpp` | `tests/plugin/runtime_native_adapter_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1`<br>`tests/runner/runtime_native_adapter_inputs.ps1` |
| `src/plugin/runtime.hpp` | 22 | Reachable include/resource | `src/plugin/worker_lifecycle.hpp`<br>`src/plugin/feature_status.hpp`<br>`src/plugin/runtime_settings.hpp` | `tests/diagnostics/diagnostics_gate_harness.cpp`<br>`tests/diagnostics/performance_telemetry_harness.cpp`<br>`tests/overlay/activation_trace_harness.cpp`<br>`tests/plugin/cinematic_lifecycle_transaction_harness.cpp`<br>`tests/runner/localization_catalog_audit.ps1` |
| `src/plugin/worker_lifecycle.cpp` | 101 | Direct build input | `src/plugin/worker_lifecycle.hpp` | No direct basename reference in tests/tools |
| `src/plugin/worker_lifecycle.hpp` | 44 | Reachable include/resource | None within src | `tests/lifecycle/worker_lifecycle_harness.cpp` |

## 9. Risk and validation separation

| Surface | Mechanical relocation risk | Separate existing safety/evidence gate |
| --- | --- | --- |
| ASI startup/shutdown, workers | No planned startup/callback/worker-owner move. Watcher profile paths must not enter production registry | N03/N04 initialization/partial cleanup; N05 quiescence stay open |
| Native hooks / camera correction | No algorithm/ABI/hook-site changes; preserve transaction headers and owners | N01 retention limits and N04–N06 native evidence not closed by path edits |
| Overlay/D3D/DComp/input | Shared resource-ID include changes only; no renderer or lifecycle edits planned | Existing Overlay placement/runner failures need their own diagnosis |
| Localization/resources | Macro integers must remain identical; all consumers/RC/audit paths change together | 18 catalog/resource/glyph audit required after approved change; no catalog/font change |
| Test and diagnostic targets | Watcher cpp paths, harness include and /I path must change consistently; generated adapter registry remains single source | Compile/fixture results must identify current artifact; prior PASS is historical |
| Packaging | Source moves do not change runtime filenames or package payloads | Verify no research ASI/generated outputs enter release archive |

## 10. Evidence consulted and outcome

Current source/build/test inputs; docs/ARCHITECTURE.md and SAFETY_INVARIANTS.md; TESTING_AND_RESEARCH.md; B1–B7 final checkpoint; N01–N03 remediation implementation report; N06 consolidated evidence report; Overlay implementation report dated 2026-10-11. Historical reports supply status/limits, not proof of current source runtime correctness.

Companion documents: [conventions](SRC_STRUCTURE_CONVENTIONS.md), [full move matrix](SRC_STRUCTURE_MOVE_MATRIX.md), [implementation plan and results](SRC_STRUCTURE_REORGANIZATION_PLAN.md), and [implementation report](SRC_STRUCTURE_REORGANIZATION_IMPLEMENTATION_2026-10-11.md).

### Audit preservation check

Before implementation, all 143 source paths and individual SHA-256 hashes matched the captured audit inventory. The four authorized files were subsequently moved without content changes; their hashes match. The remaining 139 paths retain their original source hashes. No Git staging or destructive operation was invoked. Initial and final status snapshots differ in staging flags on some pre-existing paths; exact staged/unstaged status preservation cannot be claimed, and no attempt was made to alter the index. The matrix has exactly 143 file rows: 137 Keep, 4 moved, 1 Review and 1 Legacy candidate. Build/test validation and baseline findings are recorded in the implementation report.

