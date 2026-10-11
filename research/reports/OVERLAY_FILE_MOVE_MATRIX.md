# Overlay File Move Matrix

**Date:** 2026-10-11  
**Status:** Proposed target only. No moves are approved or performed.

Target paths below preserve current filenames and translation-unit boundaries. The matrix is intentionally a directory organization proposal, not a source split.

| Current file | Proposed target | Classification | Why / dependency that must be updated |
|---|---|---|---|
| `src/overlay/aspect_ratio_format.hpp` | `src/overlay/presentation/aspect_ratio_format.hpp` | Move only | Pure display formatter; update presentation consumers and test includes. |
| `src/overlay/camera_integration.cpp` | `src/overlay/presentation/camera_integration.cpp` | Move + mechanical build updates | Semantic projection; update `build.cmd`, `test.cmd` camera harnesses, native response lists if present, and includes. |
| `src/overlay/camera_integration.hpp` | `src/overlay/presentation/camera_integration.hpp` | Move only | Projection contract; update plugin/test include paths. |
| `src/overlay/camera_state_view.cpp` | `src/overlay/presentation/camera_state_view.cpp` | Move + mechanical build updates | Camera State rendering; update production source list and active source-path audit. |
| `src/overlay/camera_state_view.hpp` | `src/overlay/presentation/camera_state_view.hpp` | Move only | View declaration; renderer and audit include paths. |
| `src/overlay/composition_presenter_state.cpp` | `src/overlay/presenter/composition_presenter_state.cpp` | Move + mechanical build updates | Existing lifecycle/generation state machine; update build/test source lists. |
| `src/overlay/composition_presenter_state.hpp` | `src/overlay/presenter/composition_presenter_state.hpp` | Move only | Presenter contract; update renderer, composition, harness and audit includes. |
| `src/overlay/composition_runtime.cpp` | `src/overlay/composition/composition_runtime.cpp` | Move + mechanical build updates; move last | Keep whole composition owner intact; update plugin facade include, build/test/evidence source paths, and all local includes. No callback or worker extraction. |
| `src/overlay/composition_runtime.hpp` | `src/overlay/composition/composition_runtime.hpp` | Move only; move last | Public-in-repo runtime facade; `src/plugin/runtime.cpp` include and all test response lists must follow. |
| `src/overlay/feature_presentation.cpp` | `src/overlay/presentation/feature_presentation.cpp` | Move + mechanical build updates | Feature status projection; update build, feature harness and localization audit. |
| `src/overlay/feature_presentation.hpp` | `src/overlay/presentation/feature_presentation.hpp` | Move only | Projection API; update harness/renderer/audit include paths. |
| `src/overlay/game_language_reader.cpp` | `src/overlay/localization/game_language_reader.cpp` | Move + mechanical build updates | Existing game-language adapter grouped with localization; update build, localization audit and consumers. Do not alter sync timing. |
| `src/overlay/game_language_reader.hpp` | `src/overlay/localization/game_language_reader.hpp` | Move only | Locale synchronization facade; composition/renderer and audit includes. |
| `src/overlay/hotkey_binding_presentation.hpp` | `src/overlay/presentation/hotkey_binding_presentation.hpp` | Move only | UI label helper; update input/localization harness and audit paths. |
| `src/overlay/imgui_d3d11_renderer.cpp` | `src/overlay/presenter/imgui_d3d11_renderer.cpp` | Move + mechanical build updates | Existing low-level backend; update `build.cmd` and backend/header includes. |
| `src/overlay/imgui_d3d11_renderer.hpp` | `src/overlay/presenter/imgui_d3d11_renderer.hpp` | Move only | Backend API; update renderer header and local include paths. |
| `src/overlay/inline_hotkey_layout.hpp` | `src/overlay/presentation/inline_hotkey_layout.hpp` | Move only | UI layout helper; update renderer/localization harness references. |
| `src/overlay/input_state.cpp` | `src/overlay/input/input_state.cpp` | Move + mechanical build updates | Singleton bridge/state implementation remains one translation unit; update build, input/composition harnesses and source audit. |
| `src/overlay/input_state.hpp` | `src/overlay/input/input_state.hpp` | Move only | Input contract; update renderer/composition headers and all harness/audit includes. |
| `src/overlay/localization_catalog.cpp` | `src/overlay/localization/localization_catalog.cpp` | Move + mechanical build updates | Catalog implementation; update build/test and intra-localization includes. |
| `src/overlay/localization_catalog.hpp` | `src/overlay/localization/localization_catalog.hpp` | Move only | Catalog API; update manager/formatter/harness includes. |
| `src/overlay/localization_font.cpp` | `src/overlay/localization/localization_font.cpp` | Move + mechanical build updates | Embedded font loading; update build/test/audit paths. |
| `src/overlay/localization_font.hpp` | `src/overlay/localization/localization_font.hpp` | Move only | Font API; update renderer/harness/audit includes. |
| `src/overlay/localization_formatter.cpp` | `src/overlay/localization/localization_formatter.cpp` | Move + mechanical build updates | Catalog message formatting; update build/test and includes. |
| `src/overlay/localization_formatter.hpp` | `src/overlay/localization/localization_formatter.hpp` | Move only | Formatter API; update users. |
| `src/overlay/localization_keys.cpp` | `src/overlay/localization/localization_keys.cpp` | Move + mechanical build updates | Key metadata; update build and direct audit path. |
| `src/overlay/localization_keys.hpp` | `src/overlay/localization/localization_keys.hpp` | Move only | Canonical key declarations; update broad consumer set and audit. |
| `src/overlay/localization_manager.cpp` | `src/overlay/localization/localization_manager.cpp` | Move + mechanical build updates | Locale manager; update build/test/audit and renderer dependencies. |
| `src/overlay/localization_manager.hpp` | `src/overlay/localization/localization_manager.hpp` | Move only | Manager API; update renderer and harness includes. |
| `src/overlay/localization_resource_ids.h` | `src/overlay/localization/localization_resource_ids.h` | Move + mechanical resource updates | Move with `.rc`; update its include, `src/localization/locale_registry.hpp`, audit, and active resource scripts. |
| `src/overlay/localization_resources.rc` | `src/overlay/localization/localization_resources.rc` | Move + mechanical resource updates | Resource compiler input in build/test/native runners; update all `rc` invocations and ID include. Validate embedded resource payload paths unchanged. |
| `src/overlay/localization_validator.cpp` | `src/overlay/localization/localization_validator.cpp` | Move + mechanical build updates | Catalog validation; update build/test and includes. |
| `src/overlay/localization_validator.hpp` | `src/overlay/localization/localization_validator.hpp` | Move only | Validator API; update harness/manager includes. |
| `src/overlay/optional_overlay_boundary.hpp` | `src/overlay/optional_overlay_boundary.hpp` | Remain | Shared exception boundary used by composition and renderer. Root placement avoids a one-header support directory and new path indirection. |
| `src/overlay/overlay_layout_metrics.cpp` | `src/overlay/presentation/overlay_layout_metrics.cpp` | Move + mechanical build updates | Pure layout calculation; update build/test and consumers. |
| `src/overlay/overlay_layout_metrics.hpp` | `src/overlay/presentation/overlay_layout_metrics.hpp` | Move only | Layout API; update renderer and harness includes. |
| `src/overlay/placement_config.cpp` | `src/overlay/presentation/placement_config.cpp` | Move + mechanical build updates | Config persistence adapter; update build/placement harness and includes. |
| `src/overlay/placement_config.hpp` | `src/overlay/presentation/placement_config.hpp` | Move only | Placement API; update renderer/harness. |
| `src/overlay/placement_save_state.hpp` | `src/overlay/presenter/placement_save_state.hpp` | Move only | Per-renderer save scheduling state; renderer-owned member and audit path. |
| `src/overlay/presentation_contracts.hpp` | `src/overlay/presentation/presentation_contracts.hpp` | Move only | Shared geometry/damage contract; update renderer/backend/composition and harness includes. |
| `src/overlay/presenter_runtime_policy.hpp` | `src/overlay/presenter/presenter_runtime_policy.hpp` | Move only | Pure wake/scheduler policy; update composition, runtime harness and audit includes. |
| `src/overlay/renderer_runtime.cpp` | `src/overlay/presenter/renderer_runtime.cpp` | Move + mechanical build updates | Keep `Renderer` as one presenter owner; update build, direct audit path and all local includes. Splitting its UI/resource lifecycle needs separate design review. |
| `src/overlay/renderer_runtime.hpp` | `src/overlay/presenter/renderer_runtime.hpp` | Move only | Central presenter API/resource ownership declaration; update composition and audit includes. |
| `src/overlay/renderer_state.cpp` | `src/overlay/presenter/renderer_state.cpp` | Move + mechanical build updates | Pure notification state helpers; update build and composition harness/audit. |
| `src/overlay/renderer_state.hpp` | `src/overlay/presenter/renderer_state.hpp` | Move only | Notification types consumed by Renderer; update headers/harness/audit. |
| `src/overlay/selector_documentation_view.cpp` | `src/overlay/presentation/selector_documentation_view.cpp` | Move + mechanical build updates | Selector documentation view; update build and localization audit. |
| `src/overlay/selector_documentation_view.hpp` | `src/overlay/presentation/selector_documentation_view.hpp` | Move only | View declaration; update renderer/audit include. |
| `src/overlay/selector_examples_geometry.hpp` | `src/overlay/presentation/selector_examples_geometry.hpp` | Move only | Pure selector example geometry; update localization harness and audit. |
| `src/overlay/selector_tooltip_state.hpp` | `src/overlay/presentation/selector_tooltip_state.hpp` | Move only | Tooltip interaction state; update focused harness and audit include. |
| `src/overlay/setting_tooltip_content.hpp` | `src/overlay/presentation/setting_tooltip_content.hpp` | Move only | User-level tooltip content helpers; update renderer/localization harness/audit. |
| `src/overlay/tooltip_row_layout.hpp` | `src/overlay/presentation/tooltip_row_layout.hpp` | Move only | Tooltip geometry helper; update renderer and localization audit. |

## Proposed directory map

```text
src/overlay/
  composition/   composition_runtime.cpp/.hpp
  input/         input_state.cpp/.hpp
  localization/  localization catalog/keys/manager/formatter/validator/font/resources
                 plus game_language_reader.cpp/.hpp
  presentation/  camera/feature projections and UI/layout/tooltip/placement helpers
  presenter/     renderer_runtime, renderer_state, composition_presenter_state,
                 presenter_runtime_policy, ImGui D3D11 backend, placement save state
  optional_overlay_boundary.hpp  (shared root-level boundary)
```

This groups 51 current files as 2 composition, 2 input, 16 localization, 20 presentation, 10 presenter and 1 shared root header. It creates no one-file directory and preserves each translation unit and API.

## Classification legend and non-goals

- **Move only:** header-only or resource declarations with no compiled source-list entry; consumers still need mechanical include/path edits.
- **Move + mechanical build updates:** compiled `.cpp` or `.rc` whose source/resource entry and dependent include/script references must change.
- **Remain:** intentionally stays at the common root.
- **Needs design review:** not used for proposed whole-file moves; required before splitting either runtime owner, changing a callback/thread boundary, or introducing new state access.

No file is classified for semantic redesign. Any extraction from `renderer_runtime.cpp` or `composition_runtime.cpp` is explicitly out of scope and requires a separate reviewed proposal.
