# Overlay Source Structure Audit

**Date:** 2026-10-11  
**Scope:** Read-only inventory and dependency/ownership audit of `src/overlay/`.  
**Decision status:** Findings only; no source moves or implementation approved.

## Executive summary

`src/overlay/` contains 51 files and 7,905 total lines. It is a real subsystem with distinct presentation, localization, input, presenter/rendering, and composition responsibilities, but its root currently flattens those responsibilities into one namespace and directory. The largest concentration is `renderer_runtime.cpp` (2,317 lines, 121,177 bytes), followed by `composition_runtime.cpp` (898 lines, 43,442 bytes). Together they account for 3,215 of 7,905 lines (40.7%); the count is an inventory measure, not evidence that either owner should be split.

The code already has meaningful boundaries: pure presenter state/policy, input bridge/state, localization/catalog/font services, semantic camera/feature projections, and a low-level ImGui D3D11 backend. The most defensible cleanup is directory organization of these existing units, not splitting either runtime owner or introducing new interfaces. `composition_runtime` remains the Overlay composition root; `renderer_runtime::Renderer` remains the owner of the UI context and presentation resources; `RuntimeState` remains owned by `src/plugin/runtime.cpp`.

## Inventory

Paths below are current repository-relative paths. Line and byte counts were read from the current working tree on 2026-10-11; they are not Git-baseline measurements. “Primary dependency” summarizes direct include/use relationships, not a complete transitive graph.

| Current file | Bytes / lines | Responsibility and interface | Primary dependencies / owner | Build and test references |
|---|---:|---|---|---|
| `src/overlay/aspect_ratio_format.hpp` | 632 / 20 | Canonical aspect-ratio display formatting; inline helper. | Presentation-only value formatting. | Camera Integration harness; localization audit references. |
| `src/overlay/camera_integration.cpp` | 18,878 / 359 | Projects semantic camera integration facts and technical values for UI. | `plugin::OverlaySemanticSnapshot`, camera/FOV and cinematic domain helpers; no camera ownership. | `build.cmd`; `test.cmd` camera writer and camera integration harnesses; localization audit. |
| `src/overlay/camera_integration.hpp` | 4,104 / 116 | Public projection/result types and functions for camera integration view. | Plugin runtime settings/snapshot contracts and domain types. | Included by plugin camera writer harness and camera integration harness; localization audit. |
| `src/overlay/camera_state_view.cpp` | 16,149 / 285 | Renders expanded technical Camera State view. | ImGui and camera projection/view contracts. | `build.cmd`; localization audit. |
| `src/overlay/camera_state_view.hpp` | 244 / 8 | Camera State rendering declaration. | ImGui/view interface. | `build.cmd`; localization audit. |
| `src/overlay/composition_presenter_state.cpp` | 4,156 / 107 | Presenter lifecycle/generation state transitions and redraw policy. | Pure state helpers; state itself is a `Renderer` member. | `build.cmd`; `test.cmd` composition presenter harness. |
| `src/overlay/composition_presenter_state.hpp` | 2,575 / 75 | Presenter phases, generation transaction and redraw contracts. | Standard-library-only state types/templates. | `build.cmd`; composition harness; localization audit. |
| `src/overlay/composition_runtime.cpp` | 43,442 / 898 | Composition root: window discovery, worker/message loop, input subclassing, frame-clock worker, lifecycle/logging and wake coordination. | `Renderer`, input bridge, language sync, diagnostics, plugin readiness, feature config; owns file-scope synchronization/worker/window-hook state. | `build.cmd`; actual-adapter source lists in test runners; localization audit; `src/plugin/runtime.cpp` calls its exported facade. |
| `src/overlay/composition_runtime.hpp` | 249 / 6 | C/runtime-facing Overlay start/readiness/publication/redraw facade. | Win32/module handle boundary. | `build.cmd`; included by `src/plugin/runtime.cpp`; runtime test source lists. |
| `src/overlay/feature_presentation.cpp` | 10,638 / 189 | Maps feature/runtime snapshot facts to user-facing status presentation. | Plugin semantic/runtime snapshots and localization keys. | `build.cmd`; `test.cmd` feature presentation harness; localization audit. |
| `src/overlay/feature_presentation.hpp` | 1,099 / 39 | Feature status presentation types/API. | Plugin configuration/runtime types. | Feature presentation harness; localization audit. |
| `src/overlay/game_language_reader.cpp` | 7,846 / 167 | Reads/normalizes game Interface Language and requests Auto locale synchronization on Overlay transitions. | Plugin runtime/config and locale registry; not a general per-frame poller. | `build.cmd`; localization audit; composition and renderer call sites. |
| `src/overlay/game_language_reader.hpp` | 585 / 15 | Language synchronization facade. | Win32 window handle and overlay locale state. | `build.cmd`; composition/renderer includes; localization audit. |
| `src/overlay/hotkey_binding_presentation.hpp` | 1,822 / 41 | Hotkey label/metadata presentation helpers. | Runtime settings/config binding IDs and localization keys. | `build.cmd` indirectly through renderer; input/localization tests and audit path references. |
| `src/overlay/imgui_d3d11_renderer.cpp` | 25,617 / 500 | Low-level ImGui draw-data renderer using D3D11 resources. | ImGui draw data and D3D11; owns backend resource members per renderer instance. | `build.cmd`; localization resources not directly; renderer runtime includes it. |
| `src/overlay/imgui_d3d11_renderer.hpp` | 2,240 / 49 | D3D11 ImGui renderer class/API. | D3D11/WRL and shared presentation geometry. | Included by renderer runtime; `build.cmd`. |
| `src/overlay/inline_hotkey_layout.hpp` | 782 / 26 | Inline layout helper for hotkey labels. | ImGui/localized text presentation. | Renderer; localization harness and localization audit. |
| `src/overlay/input_state.cpp` | 12,143 / 303 | Toggle/focus/rebind behavior and bounded input-event bridge implementation. | Config binding IDs, Win32; function-local singleton owns bridge and input state. | `build.cmd`; `test.cmd` input/composition harnesses; localization audit. |
| `src/overlay/input_state.hpp` | 5,815 / 154 | Input event, bridge, visibility/focus/rebind state APIs. | Config binding IDs, Win32, atomics/mutex. | `build.cmd`; input/composition harnesses; localization audit. |
| `src/overlay/localization_catalog.cpp` | 9,350 / 208 | Catalog loading/validation-facing locale catalog implementation. | Localization keys/resource IDs and registry. | `build.cmd`; localization harness and audit. |
| `src/overlay/localization_catalog.hpp` | 958 / 30 | Catalog interface/types. | Locale registry/key declarations. | Localization harness; `build.cmd`. |
| `src/overlay/localization_font.cpp` | 10,150 / 230 | Font profile/resource loading and locale-aware font setup. | ImGui font atlas, embedded resources and localization catalog. | `build.cmd`; localization harness/audit. |
| `src/overlay/localization_font.hpp` | 441 / 13 | Font loader API. | ImGui/Windows types. | Renderer and localization harness; audit. |
| `src/overlay/localization_formatter.cpp` | 2,182 / 50 | Localized template/message formatting. | Localization catalog/keys. | `build.cmd`; localization harness/audit. |
| `src/overlay/localization_formatter.hpp` | 482 / 17 | Formatting API. | Standard string-view/value types. | Localization harness; `build.cmd`. |
| `src/overlay/localization_keys.cpp` | 7,768 / 132 | Localization key metadata/lookup implementation. | Key declarations and catalog. | `build.cmd`; localization harness/audit. |
| `src/overlay/localization_keys.hpp` | 17,460 / 250 | Canonical localization-key and locale metadata declarations. | Standard types/resource IDs. | `build.cmd`; localization audit reads it directly. |
| `src/overlay/localization_manager.cpp` | 4,596 / 108 | Effective locale selection and catalog/font-facing manager behavior. | Catalog, formatter, registry, resources. | `build.cmd`; localization harness/audit; renderer member. |
| `src/overlay/localization_manager.hpp` | 1,313 / 36 | Localization manager API/state. | Catalog and locale registry. | Localization harness; renderer header; audit. |
| `src/overlay/localization_resource_ids.h` | 865 / 30 | Windows resource identifiers. | Resource compiler. | `.rc`, `src/localization/locale_registry.hpp`, localization audit; several runner scripts indirectly. |
| `src/overlay/localization_resources.rc` | 1,666 / 30 | Embeds locale/font resources into the ASI and test executables. | Resource IDs and resource payload paths. | `build.cmd`, `test.cmd`, native evidence runners and N06 validator scripts. |
| `src/overlay/localization_validator.cpp` | 2,647 / 56 | Catalog/resource validation helpers. | Catalog keys/locale registry. | `build.cmd`; localization harness/audit. |
| `src/overlay/localization_validator.hpp` | 475 / 16 | Validation API. | Catalog/key types. | Localization harness; `build.cmd`. |
| `src/overlay/optional_overlay_boundary.hpp` | 1,031 / 30 | Shared noexcept/exception-containment boundary helpers for optional Overlay calls. | Standard library only; used by composition and renderer runtime. | Included by two runtime units; build lists compile dependents. |
| `src/overlay/overlay_layout_metrics.cpp` | 1,795 / 42 | Pure Overlay geometry/layout metrics. | Shared presentation geometry. | `build.cmd`; `test.cmd` layout harness. |
| `src/overlay/overlay_layout_metrics.hpp` | 451 / 14 | Layout metrics API. | Standard geometry types. | Layout harness; renderer; `build.cmd`. |
| `src/overlay/placement_config.cpp` | 653 / 19 | Reads/writes persisted Overlay placement configuration. | Config repository and placement types. | `build.cmd`; `test.cmd` placement harness. |
| `src/overlay/placement_config.hpp` | 289 / 8 | Placement persistence API. | Config types. | Placement harness; renderer; `build.cmd`. |
| `src/overlay/placement_save_state.hpp` | 1,230 / 35 | Pure placement-save scheduling/state. | Standard types. | Renderer state/header; localization audit. |
| `src/overlay/presentation_contracts.hpp` | 3,361 / 99 | Shared geometry/damage/render presentation contracts. | Standard-library-only types. | Renderer/backend, composition runtime; composition harness and audit. |
| `src/overlay/presenter_runtime_policy.hpp` | 3,914 / 113 | Pure scheduling/wake/timer policy contracts. | Standard types; consumed by composition runtime. | `build.cmd`; camera writer/composition harnesses; localization audit. |
| `src/overlay/renderer_runtime.cpp` | 121,177 / 2,317 | `Renderer` implementation: private D3D11/DComp composition resources, ImGui context/frame/UI, notifications, settings and shutdown. | Most presentation/localization/input/camera-view helpers; owns instance resources and executes on Overlay owner thread. | `build.cmd`; localization audit; path-based research reports/tests. |
| `src/overlay/renderer_runtime.hpp` | 7,046 / 174 | `Renderer` interface and resource/state ownership declaration. | D3D11/DComp/DXGI/ImGui, localization, input, presenter state, runtime settings. | `build.cmd`; composition runtime and localization audit. |
| `src/overlay/renderer_state.cpp` | 2,653 / 66 | Pure startup-notification and notification-lifetime state helpers. | Standard types. | `build.cmd`; composition harness; localization audit. |
| `src/overlay/renderer_state.hpp` | 2,165 / 54 | Notification lifecycle/draw evidence types. | Standard types. | Renderer; composition harness; localization audit. |
| `src/overlay/selector_documentation_view.cpp` | 6,216 / 126 | Selector option documentation layout/render helpers. | ImGui, selector metadata and tooltip state. | `build.cmd`; localization audit. |
| `src/overlay/selector_documentation_view.hpp` | 350 / 11 | Selector documentation view declaration. | Selector option/value types. | `build.cmd`; localization audit. |
| `src/overlay/selector_examples_geometry.hpp` | 1,277 / 34 | Pure example geometry and selector explanatory presentation data. | Standard geometry. | Localization harness/audit; renderer. |
| `src/overlay/selector_tooltip_state.hpp` | 347 / 15 | Selector tooltip activation/state helper. | Standard types/ImGui interaction values. | Focused selector tooltip harness; localization audit. |
| `src/overlay/setting_tooltip_content.hpp` | 7,491 / 146 | User-facing setting tooltip content and composition helpers. | Localization keys/formatter and setting metadata. | Localization harness/audit; renderer. |
| `src/overlay/tooltip_row_layout.hpp` | 1,640 / 39 | Tooltip row geometry/layout helper. | Standard geometry and ImGui-facing dimensions. | Renderer; localization audit. |

## Runtime ownership and contracts

- **Runtime integration:** `RuntimeState` remains the sole plugin runtime owner in `src/plugin/runtime.cpp`. Overlay consumes snapshots/readiness and sends narrow notifications through the composition facade; it does not own correction state, `CameraCorrectionGate`, camera publication or lifecycle semantics.
- **Composition/discovery:** `composition_runtime.cpp` owns the Overlay worker/discovery path, game-window selection, window subclass records, out-of-context WinEvent hooks, wake messages, file logging and the optional compositor-clock worker. Its state is file-scope-private. It uses a recursive renderer mutex and separate log/input-binding mutexes; their ordering and ownership are safety-sensitive.
- **Presenter/rendering:** `Renderer` owns its D3D11 device/context-facing backend, DComp desktop device/target/visual/surface, ImGui context lifecycle, localization manager and per-instance UI/notification/placement state. Rendering and COM operations are serialized on the Overlay owner thread. `CompositionPresenterState` owns only the per-renderer lifecycle/generation model, not OS/COM resources.
- **Input:** `InputEventBridge` and `InputState` are function-local singletons in `input_state.cpp`; the bridge is bounded and synchronized, and the game window procedure forwards events rather than applying them to ImGui. The renderer owner thread drains/applies them.
- **Localization:** localization manager/catalog/font code owns UI locale/catalog/font behavior. `game_language_reader` is an adapter to the game’s Interface Language and is invoked at the existing Overlay open/synchronization points; it is not a new polling owner.
- **Hooks and GPU boundaries:** Overlay does not install DXGI factory, game swapchain, Present/Resize hooks and does not consume game GPU buffers. It does install a Win32 game-window procedure subclass and WinEvent notifications, and it creates its private D3D11/DirectComposition resources. Preserve this distinction in any move plan.
- **Optional failure boundary:** `optional_overlay_boundary.hpp` is shared by the composition and renderer runtime code. It should remain a shared root-level helper unless a broader, independently justified common-support structure is approved.

## Build, tests, resources and packaging references

This repository’s supported build is script-driven, not CMake-driven. The authoritative production compile/resource list is `build.cmd`; `test.cmd` compiles the Overlay harnesses and resource file. Additional production-equivalent native-adapter runners compile source response lists and invoke `rc` on the same `.rc` file. The localization catalog audit reads source paths literally, so it must be updated even where compilation succeeds.

Direct path references confirmed in the current source tree include:

- `build.cmd`: `localization_resources.rc` resource compilation (line 58), all Overlay `.cpp` source entries (line 63), and the generated `.res` link input (line 65).
- `test.cmd`: resource compilation (line 32), camera integration source in the camera writer harness (line 53), and composition/layout/input/camera/placement/feature/localization harness source lists (lines 137–155).
- `tests/runner/native_runtime_evidence.ps1`, `research/evidence/native-runtime/N06/tools/n06_validate.ps1`, and `n06_m01_validate.ps1`: explicit resource compiler input. Their response/source lists must be checked when any `.cpp` moves.
- `tests/runner/localization_catalog_audit.ps1`: explicit source paths for keys, resources, font, renderer, camera view, input, policy, selector views/tooltips, placement state, composition, language reader and feature presentation (lines 126, 176–177, 210–211, 243–282).
- Repository-relative includes in `src/plugin/runtime.cpp`, `src/localization/locale_registry.hpp`, Overlay headers/sources and `tests/**` harnesses. Resource compiler include in `localization_resources.rc` is also root-relative.
- Path mentions in research documents and protected-file manifests are historical evidence, not runtime/build dependencies. Do not mass-rewrite historical records; update only active inventories/manifests that claim current paths.

The active packaging references reviewed here produce the ASI through `build.cmd` and link resources into it; no separate source-file packaging list was identified in those references. Before an eventual move, search active scripts/manifests for old paths and distinguish executable dependencies from historical reports.

## Findings and boundaries

1. The best supported organization is a directory-only reorganization around existing responsibilities. It can improve discoverability without changing ABI or state ownership, if all includes and source lists are updated mechanically.
2. `renderer_runtime.cpp` is large, but is the presenter owner and currently centralizes ImGui frame/UI/resource lifecycle. Splitting it is outside this cleanup and would require a separate ownership/dependency design review.
3. `composition_runtime.cpp` is the Overlay integration/composition root. Moving its file as a unit is mechanically possible, but it is a high-coupling step due to plugin callers, window hooks, worker ownership, diagnostics and startup ordering. Keep it intact and move it last.
4. The resource compiler edge is easy to miss: moving localization files requires updating the `.rc` input, resource-ID include and `src/localization/locale_registry.hpp`, plus every runner that invokes `rc`.
5. Existing dirty changes are present in the workspace (including `src/plugin/runtime.cpp`, `build.cmd`, `test.cmd`, SafetyHook and runtime-decomposition evidence). They are unrelated to this audit and must be preserved. This report does not treat the dirty tree as a clean release baseline.

## Audit limits

This is a static source/build-reference audit. No source files were moved or modified; only the three requested audit/planning reports were created. No build, harness, game or ASI was run. The inventory records present direct dependencies, but does not prove binary equivalence after future moves. Those require per-batch compiler, focused harness, full test and production/diagnostic build evidence.
