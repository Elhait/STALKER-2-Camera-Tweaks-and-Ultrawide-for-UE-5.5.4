# Security audit — per-file coverage, 2026-10-17

Companion to [the master report](COMPREHENSIVE_ASI_SECURITY_HARDENING_2026-10-17.md).
Inventory was obtained from the explicit production TU registry, src headers/resource declarations and the actual isolated Release Ninja dependencies. This is an inventory and **review-depth ledger**, not an assertion that scanning a file proves its correctness.

## Review-depth legend

- **BOUNDARY:** inspected concrete security-sensitive execution/failure paths and callers. Still not proof of every branch.
- **SURFACE:** structural source/security-operation scan and module contract/test cross-check; **not a completed independent implementation review**. This distinction intentionally preserves the remaining full-audit coverage gap.
- **VENDOR-USE:** reviewed reachable integration, capability boundaries, version/provenance and selected sensitive internals; **not a fresh exhaustive audit of that vendor file**. Includes decoder tables, formatting/templates, fonts and ImGui algorithms whose entire implementation was not independently re-proven.
- **COMPILED-OUT:** header dependency is present, but forensic body is disabled in ordinary Release. No claim that dormant research code was audited for deployment.
- **GENERATED:** build-generated canonical INI documentation; data generation/input checked, not a new executable implementation.
- **NOT-PRODUCTION:** extra research header found by the src-tree inventory, absent from the ordinary Release dependency set.

Counts: GENERATED=1, VENDOR-USE=51, SURFACE=108, BOUNDARY=41, COMPILED-OUT=1, NOT-PRODUCTION=1. Total 203 records; 61 registered TUs. Full hashes are in [source-inventory.json](../evidence/security-hardening-2026-10-17/source-inventory.json).

Do not rename SURFACE or VENDOR-USE to a full independent source-review PASS merely because the tests build or pass. A stronger all-file implementation-audit closure remains unestablished; the validated hardening batch can be reviewed separately.

## Source / compiler dependency ledger

| File | Role | Review depth |
| --- | --- | --- |
| `build-artifacts/security-hardening/release/production-tOFF-dOFF/generated/canonical_ini_documentation.hpp` | header/dependency | GENERATED |
| `external/imgui/backends/imgui_impl_win32.cpp` | vendor | VENDOR-USE |
| `external/imgui/backends/imgui_impl_win32.h` | header/dependency | VENDOR-USE |
| `external/imgui/imconfig.h` | header/dependency | VENDOR-USE |
| `external/imgui/imgui_draw.cpp` | vendor | VENDOR-USE |
| `external/imgui/imgui_internal.h` | header/dependency | VENDOR-USE |
| `external/imgui/imgui_tables.cpp` | vendor | VENDOR-USE |
| `external/imgui/imgui_widgets.cpp` | vendor | VENDOR-USE |
| `external/imgui/imgui.cpp` | vendor | VENDOR-USE |
| `external/imgui/imgui.h` | header/dependency | VENDOR-USE |
| `external/imgui/imstb_rectpack.h` | header/dependency | VENDOR-USE |
| `external/imgui/imstb_textedit.h` | header/dependency | VENDOR-USE |
| `external/imgui/imstb_truetype.h` | header/dependency | VENDOR-USE |
| `external/safetyhook/safetyhook.cpp` | vendor | VENDOR-USE |
| `external/safetyhook/safetyhook.hpp` | header/dependency | VENDOR-USE |
| `external/safetyhook/Zydis.c` | vendor | VENDOR-USE |
| `external/safetyhook/Zydis.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/common-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/common.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/backtracer-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/backtracer.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/circular_q.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/fmt_helper.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/log_msg_buffer-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/log_msg_buffer.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/log_msg-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/log_msg.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/null_mutex.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/os-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/os.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/details/windows_include.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/bundled/base.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/bundled/chrono.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/bundled/color.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/bundled/format-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/bundled/format.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/bundled/ostream.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/bundled/ranges.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/bundled/xchar.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/fmt.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/fmt/xchar.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/formatter.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/logger-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/logger.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/mdc.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/pattern_formatter-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/pattern_formatter.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/sinks/base_sink-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/sinks/base_sink.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/sinks/sink-inl.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/sinks/sink.h` | header/dependency | VENDOR-USE |
| `external/spdlog/include/spdlog/tweakme.h` | header/dependency | VENDOR-USE |
| `src/camera/camera_state_publication.hpp` | header/dependency | SURFACE |
| `src/camera/camera_state_snapshot.cpp` | production | SURFACE |
| `src/camera/camera_state_snapshot.hpp` | header/dependency | SURFACE |
| `src/camera/fov_observation.cpp` | production | SURFACE |
| `src/camera/fov_observation.hpp` | header/dependency | SURFACE |
| `src/camera/fov_semantics.hpp` | header/dependency | SURFACE |
| `src/camera/gameplay_aspect_restoration.cpp` | production | SURFACE |
| `src/camera/gameplay_aspect_restoration.hpp` | header/dependency | SURFACE |
| `src/camera/gameplay_baseline.cpp` | production | SURFACE |
| `src/camera/gameplay_baseline.hpp` | header/dependency | SURFACE |
| `src/camera/horplus.cpp` | production | SURFACE |
| `src/camera/horplus.hpp` | header/dependency | SURFACE |
| `src/camera/presentation_state.cpp` | production | SURFACE |
| `src/camera/presentation_state.hpp` | header/dependency | SURFACE |
| `src/cinematics/cinematic_aspect.cpp` | production | BOUNDARY |
| `src/cinematics/cinematic_aspect.hpp` | header/dependency | SURFACE |
| `src/cinematics/cinematic_exit_authority.hpp` | header/dependency | SURFACE |
| `src/cinematics/cinematic_fov.cpp` | production | SURFACE |
| `src/cinematics/cinematic_fov.hpp` | header/dependency | SURFACE |
| `src/cinematics/cinematic_initialization.cpp` | production | SURFACE |
| `src/cinematics/cinematic_initialization.hpp` | header/dependency | SURFACE |
| `src/cinematics/cinematic_recovery_retirement.hpp` | header/dependency | SURFACE |
| `src/cinematics/cinematic_selection.cpp` | production | SURFACE |
| `src/cinematics/cinematic_selection.hpp` | header/dependency | SURFACE |
| `src/config/config_line_reader.hpp` | header/dependency | BOUNDARY |
| `src/config/config_repository.cpp` | production | BOUNDARY |
| `src/config/config_repository.hpp` | header/dependency | SURFACE |
| `src/config/config_template.cpp` | production | BOUNDARY |
| `src/config/config_template.hpp` | header/dependency | SURFACE |
| `src/config/feature_config.cpp` | production | BOUNDARY |
| `src/config/feature_config.hpp` | header/dependency | SURFACE |
| `src/diagnostics/camera_log_probe.hpp` | header/dependency | SURFACE |
| `src/diagnostics/camera_writer_identity.hpp` | header/dependency | SURFACE |
| `src/diagnostics/cinematic_baseline_provenance.hpp` | header/dependency | SURFACE |
| `src/diagnostics/cinematic_continuation_trace.cpp` | production | SURFACE |
| `src/diagnostics/cinematic_continuation_trace.hpp` | header/dependency | SURFACE |
| `src/diagnostics/cinematic_lifecycle_context.hpp` | header/dependency | SURFACE |
| `src/diagnostics/diagnostic_runtime.cpp` | production | SURFACE |
| `src/diagnostics/diagnostic_runtime.hpp` | header/dependency | SURFACE |
| `src/diagnostics/dialogue_source_provenance.cpp` | production | SURFACE |
| `src/diagnostics/dialogue_source_provenance.hpp` | header/dependency | SURFACE |
| `src/diagnostics/language_state/language_state_transition.hpp` | header/dependency | NOT-PRODUCTION |
| `src/diagnostics/matchgameplay_prediction.hpp` | header/dependency | SURFACE |
| `src/diagnostics/performance_telemetry.cpp` | production | SURFACE |
| `src/diagnostics/performance_telemetry.hpp` | header/dependency | SURFACE |
| `src/diagnostics/startup_journal.hpp` | header/dependency | BOUNDARY |
| `src/diagnostics/startup_timeline.hpp` | header/dependency | COMPILED-OUT |
| `src/dialogue/dialogue_fov.cpp` | production | SURFACE |
| `src/dialogue/dialogue_fov.hpp` | header/dependency | SURFACE |
| `src/dialogue/dialogue_state.cpp` | production | SURFACE |
| `src/dialogue/dialogue_state.hpp` | header/dependency | SURFACE |
| `src/gameplay/aspect_policy.cpp` | production | SURFACE |
| `src/gameplay/aspect_policy.hpp` | header/dependency | SURFACE |
| `src/gameplay/gameplay_camera.cpp` | production | BOUNDARY |
| `src/gameplay/gameplay_camera.hpp` | header/dependency | SURFACE |
| `src/gameplay/gameplay_state.cpp` | production | SURFACE |
| `src/gameplay/gameplay_state.hpp` | header/dependency | SURFACE |
| `src/gameplay/horplus_gameplay.cpp` | production | SURFACE |
| `src/gameplay/horplus_gameplay.hpp` | header/dependency | SURFACE |
| `src/hooks/hook_set.cpp` | production | BOUNDARY |
| `src/hooks/hook_set.hpp` | header/dependency | SURFACE |
| `src/hooks/instruction_validator.cpp` | production | BOUNDARY |
| `src/hooks/instruction_validator.hpp` | header/dependency | SURFACE |
| `src/hooks/signature_scanner.cpp` | production | BOUNDARY |
| `src/hooks/signature_scanner.hpp` | header/dependency | SURFACE |
| `src/hooks/signatures/signature_definitions.hpp` | header/dependency | SURFACE |
| `src/localization/locale_registry.hpp` | header/dependency | SURFACE |
| `src/localization/localization_resource_ids.h` | header/dependency | SURFACE |
| `src/overlay/composition/composition_runtime.cpp` | production | BOUNDARY |
| `src/overlay/composition/composition_runtime.hpp` | header/dependency | SURFACE |
| `src/overlay/composition/owner_finalizer.hpp` | header/dependency | BOUNDARY |
| `src/overlay/composition/stable_record_pool.hpp` | header/dependency | BOUNDARY |
| `src/overlay/input/game_thread_subclass.hpp` | header/dependency | BOUNDARY |
| `src/overlay/input/input_state.cpp` | production | BOUNDARY |
| `src/overlay/input/input_state.hpp` | header/dependency | SURFACE |
| `src/overlay/localization/game_language_reader_publication.hpp` | header/dependency | BOUNDARY |
| `src/overlay/localization/game_language_reader.cpp` | production | BOUNDARY |
| `src/overlay/localization/game_language_reader.hpp` | header/dependency | SURFACE |
| `src/overlay/localization/localization_catalog.cpp` | production | BOUNDARY |
| `src/overlay/localization/localization_catalog.hpp` | header/dependency | SURFACE |
| `src/overlay/localization/localization_font.cpp` | production | BOUNDARY |
| `src/overlay/localization/localization_font.hpp` | header/dependency | SURFACE |
| `src/overlay/localization/localization_formatter.cpp` | production | BOUNDARY |
| `src/overlay/localization/localization_formatter.hpp` | header/dependency | SURFACE |
| `src/overlay/localization/localization_keys.cpp` | production | SURFACE |
| `src/overlay/localization/localization_keys.hpp` | header/dependency | SURFACE |
| `src/overlay/localization/localization_manager.cpp` | production | BOUNDARY |
| `src/overlay/localization/localization_manager.hpp` | header/dependency | SURFACE |
| `src/overlay/localization/localization_resources.rc` | resource | SURFACE |
| `src/overlay/localization/localization_validator.cpp` | production | BOUNDARY |
| `src/overlay/localization/localization_validator.hpp` | header/dependency | SURFACE |
| `src/overlay/optional_overlay_boundary.hpp` | header/dependency | BOUNDARY |
| `src/overlay/presentation/aspect_ratio_format.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/camera_integration.cpp` | production | SURFACE |
| `src/overlay/presentation/camera_integration.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/camera_state_view.cpp` | production | SURFACE |
| `src/overlay/presentation/camera_state_view.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/feature_presentation.cpp` | production | SURFACE |
| `src/overlay/presentation/feature_presentation.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/hotkey_binding_presentation.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/inline_hotkey_layout.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/overlay_layout_metrics.cpp` | production | SURFACE |
| `src/overlay/presentation/overlay_layout_metrics.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/placement_config.cpp` | production | SURFACE |
| `src/overlay/presentation/placement_config.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/presentation_contracts.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/selector_documentation_view.cpp` | production | SURFACE |
| `src/overlay/presentation/selector_documentation_view.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/selector_examples_geometry.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/selector_tooltip_state.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/setting_tooltip_content.hpp` | header/dependency | SURFACE |
| `src/overlay/presentation/tooltip_row_layout.hpp` | header/dependency | SURFACE |
| `src/overlay/presenter/composition_presenter_state.cpp` | production | SURFACE |
| `src/overlay/presenter/composition_presenter_state.hpp` | header/dependency | SURFACE |
| `src/overlay/presenter/imgui_d3d11_renderer.cpp` | production | BOUNDARY |
| `src/overlay/presenter/imgui_d3d11_renderer.hpp` | header/dependency | SURFACE |
| `src/overlay/presenter/placement_save_state.hpp` | header/dependency | SURFACE |
| `src/overlay/presenter/presenter_runtime_policy.hpp` | header/dependency | SURFACE |
| `src/overlay/presenter/renderer_runtime.cpp` | production | BOUNDARY |
| `src/overlay/presenter/renderer_runtime.hpp` | header/dependency | SURFACE |
| `src/overlay/presenter/renderer_state.cpp` | production | SURFACE |
| `src/overlay/presenter/renderer_state.hpp` | header/dependency | SURFACE |
| `src/platform/process_lifetime.hpp` | header/dependency | BOUNDARY |
| `src/platform/win32/memory.cpp` | production | BOUNDARY |
| `src/platform/win32/memory.hpp` | header/dependency | SURFACE |
| `src/platform/win32/module_residency.hpp` | header/dependency | BOUNDARY |
| `src/platform/win32/native_file.hpp` | header/dependency | BOUNDARY |
| `src/platform/win32/native_log_file.hpp` | header/dependency | BOUNDARY |
| `src/platform/win32/native_log_sink.hpp` | header/dependency | BOUNDARY |
| `src/platform/win32/sha256.cpp` | production | BOUNDARY |
| `src/platform/win32/sha256.hpp` | header/dependency | SURFACE |
| `src/platform/win32/viewport.cpp` | production | BOUNDARY |
| `src/platform/win32/viewport.hpp` | header/dependency | SURFACE |
| `src/platform/win32/window.cpp` | production | BOUNDARY |
| `src/platform/win32/window.hpp` | header/dependency | SURFACE |
| `src/plugin/camera_correction_gate.hpp` | header/dependency | BOUNDARY |
| `src/plugin/camera_writer_transaction.hpp` | header/dependency | SURFACE |
| `src/plugin/cinematic_lifecycle_transaction.hpp` | header/dependency | SURFACE |
| `src/plugin/dialogue_boundary_transaction.hpp` | header/dependency | SURFACE |
| `src/plugin/dll_entry.cpp` | dll_entry | BOUNDARY |
| `src/plugin/feature_status.cpp` | production | SURFACE |
| `src/plugin/feature_status.hpp` | header/dependency | SURFACE |
| `src/plugin/native_callback_admission.hpp` | header/dependency | BOUNDARY |
| `src/plugin/optional_overlay_startup.hpp` | header/dependency | SURFACE |
| `src/plugin/runtime_internal.hpp` | header/dependency | SURFACE |
| `src/plugin/runtime_settings.cpp` | production | BOUNDARY |
| `src/plugin/runtime_settings.hpp` | header/dependency | BOUNDARY |
| `src/plugin/runtime.cpp` | runtime_owner | BOUNDARY |
| `src/plugin/runtime.hpp` | header/dependency | SURFACE |
| `src/plugin/worker_lifecycle.cpp` | production | BOUNDARY |
| `src/plugin/worker_lifecycle.hpp` | header/dependency | SURFACE |

## Embedded data / resource inputs

All 29 RCDATA payload hashes in the final ASI match the corresponding source inputs. This establishes byte identity, not upstream font authenticity or immunity of a parser to hostile input. No custom external font/catalog path is accepted.

| Source input | Resource ID | Bytes | Match |
| --- | ---: | ---: | --- |
| `locales/en.json` | 101 | 16137 | IDENTICAL |
| `locales/uk.json` | 102 | 24334 | IDENTICAL |
| `assets/fonts/ProggyVector/ProggyVector-Regular.ttf` | 103 | 399528 | IDENTICAL |
| `assets/fonts/ProggyVector/ProggyVector_Licence.txt` | 104 | 5148 | IDENTICAL |
| `locales/ar.json` | 105 | 20495 | IDENTICAL |
| `locales/cs.json` | 106 | 16463 | IDENTICAL |
| `locales/fr.json` | 107 | 17656 | IDENTICAL |
| `locales/de.json` | 108 | 17471 | IDENTICAL |
| `locales/it.json` | 109 | 17326 | IDENTICAL |
| `locales/ja.json` | 110 | 18433 | IDENTICAL |
| `locales/ko.json` | 111 | 17637 | IDENTICAL |
| `locales/pl.json` | 112 | 16630 | IDENTICAL |
| `locales/pt-BR.json` | 113 | 17384 | IDENTICAL |
| `locales/ru.json` | 114 | 24461 | IDENTICAL |
| `locales/sr-Cyrl.json` | 115 | 22744 | IDENTICAL |
| `locales/zh-Hans.json` | 116 | 14763 | IDENTICAL |
| `locales/es-419.json` | 117 | 17587 | IDENTICAL |
| `locales/es-ES.json` | 118 | 17542 | IDENTICAL |
| `locales/zh-Hant.json` | 119 | 14965 | IDENTICAL |
| `locales/tr.json` | 120 | 16930 | IDENTICAL |
| `assets/fonts/CatalogSubsets/NotoSansMono-Catalog.ttf` | 121 | 6272 | IDENTICAL |
| `assets/fonts/CatalogSubsets/NotoSansArabic-Catalog.ttf` | 122 | 5252 | IDENTICAL |
| `assets/fonts/CatalogSubsets/NotoSansJP-Catalog.ttf` | 123 | 74292 | IDENTICAL |
| `assets/fonts/CatalogSubsets/NotoSansKR-Catalog.ttf` | 124 | 38984 | IDENTICAL |
| `assets/fonts/CatalogSubsets/NotoSansSC-Catalog.ttf` | 125 | 82136 | IDENTICAL |
| `assets/fonts/CatalogSubsets/NotoSansTC-Catalog.ttf` | 126 | 97060 | IDENTICAL |
| `assets/fonts/NotoSansMono/OFL.txt` | 127 | 4396 | IDENTICAL |
| `assets/fonts/CatalogSubsets/OFL-Arabic.txt` | 128 | 4382 | IDENTICAL |
| `assets/fonts/CatalogSubsets/CJK-Copyright.txt` | 129 | 503 | IDENTICAL |

## Build and harness infrastructure review

The master report traces CMakeLists.txt, build.cmd, test.cmd, tools/build/production_sources.json, build-cmake-candidate.cmd, publish_asi.ps1 and package_release.ps1; canonical tests use tests/test_harnesses.json, test_cmd_orchestrator.ps1 and process_tree_supervisor.psm1. Compatibility/research wrappers were distinguished from ordinary production build inputs. Their static presence is not evidence that they ran. Added audit tooling is research-only and absent from production sources/resources.
