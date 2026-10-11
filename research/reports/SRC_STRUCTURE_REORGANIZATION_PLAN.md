# SRC_STRUCTURE_REORGANIZATION_PLAN

Date: 2026-10-11  
Status: approved and implemented  
Scope completed: four files relocated in two batches, with only required path-reference changes.

## Goal and non-goals

Implement the move matrix without changing behavior, owners, ABI, hooks, synchronization, runtime correction, effective settings, INI, localization text, resources, compiler/link settings or release contents. No source split/merge/deletion/rename and no new manager. Closed B1–B7 decisions remain closed. Review/Legacy rows are retained and are not implementation batches.

## 0. Baseline findings

The initial physical inventory contained 143 files. Per-file SHA-256 hashes were captured before the moves. No reset, clean, checkout or staging commands were used. Initial and final status snapshots differ in staging flags on some pre-existing paths; exact staged/unstaged status preservation cannot be claimed, and those paths were left untouched. The four moved file hashes match their baseline exactly.

The placement failure was reproduced when the harness used the sandbox's system TEMP path. Rebuilding and running it with TEMP/TMP set to the workspace's ignored build-artifacts/tests/temp path passed all save, migration and 24/24 concurrent-writer assertions. This identifies the environment-path dependency; it does not show a placement persistence defect.

The full test.cmd remains non-green (exit 2). The runner inventory reports 59 registered harnesses and one unregistered repository harness, n06_m01_zoom_game_derived_harness. Its compiler output also reports C++17-required diagnostics even though the runner intends /std:c++latest. The same toolchain and explicit C++23 flags successfully compile the focused placement and watcher harnesses, but the mechanism producing the full-run dialect mismatch is not established. No runner registration or compiler behavior was changed in this task.

Read-only Git status/revision work; git diff --check still exits 128 with “this operation must be run in a work tree.” No staging/index workaround was attempted. This remains a tooling limitation for rename-aware diff review.

## Batch D — Standalone language-state research profile

Move these unchanged files together to diagnostics/language_state:

- language_state_watcher.cpp
- language_state_transition.cpp
- language_state_transition.hpp

Update watcher builder source lists and /I, test.cmd classifier source, harness include. Keep DLL entry, worker/state lifetime, output name STALKER2LanguageStateWatcherDiagnostic.asi and research-only target. Production build input set must remain exactly unchanged.

Gate: old active paths absent; moved contents identical except necessary include-depth edits (local pair includes need none); transition harness compile/run; standalone watcher compile/link without loading it; registered test source audit. No watcher/game launch.

## Batch L — Shared localization resource identity

Move overlay/localization/localization_resource_ids.h to localization/localization_resource_ids.h.

Update shared registry include, font-loader include, resource-script include and localization audit path. No .rc payload move, resource-number edit, catalog/font edit or localization API transfer.

Gate: macro header byte-identical; all IDs equal baseline; RC compile from repository root; full 18-catalog/resource/glyph audit; localization harness; config/default/template and settings harnesses because config imports registry. Production and diagnostic compile/link with unchanged source/define/link-library sets.

## Automatic progression and stop conditions

The user approved D → L without intermediate gates for mechanical path updates. Both batches completed. No changes required a behavioral edit, new API/owner, ABI/mutex/lifecycle change, resource-ID change or destructive operation.

The approved order was D then L. Both batch checks passed.

## Final proposed target tree

Review/Legacy files remain. All 143 existing files are present; only four locations differ.

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
    language_state/
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
    localization_resource_ids.h
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

## Final validation

- Inventory equality: current minus four sources plus four targets; 143 files, no duplicates; every matrix row accounted for.
- Hash/body comparison: moved resource header and watcher/classifier bodies unchanged; any reference-only include edits explicitly enumerated. Pre-existing files outside the approved scope were not intentionally changed; final staging flags differed from the initial status snapshot, and no attempt was made to alter the index.
- Scan old active source paths in src/build/test/tools/evidence runners for both slash styles. Distinguish historical reports from operative instructions.
- Header graph: no new include SCC, no duplicate owner/gate/static/TLS storage, no runtime_internal.hpp promotion.
- Full test.cmd: FAIL, exit 2, for the runner inventory and C++ standard diagnostics described above. Do not treat registered source counts as successful executions.
- Focused language-state classifier harness and separate watcher build: PASS, exit 0. Watcher output was built only; it was not run or installed.
- Placement harness rebuilt from moved/current sources: PASS, exit 0 with workspace TEMP/TMP; 24/24 concurrent updates retained.
- Localization/resource audit: PASS, 18 locales and 212 keys. Production and diagnostic builds: PASS, exit 0; both resource compilation steps passed.
- Four moved source hashes equal their pre-move values; src still contains 143 files. Old active paths are absent from src, tools, tests, build/test entry points and the active N06 resource runners.
- git diff --check / rename-aware diff remain unavailable: Git returns “this operation must be run in a work tree.” No staging was used to force rename detection.
- Development artifacts were written only under ignored build-artifacts/source-structure and build-artifacts/language-state-watcher. No release archive, installed ASI or game process was touched.
- Full results and artifact hashes are in [the implementation report](SRC_STRUCTURE_REORGANIZATION_IMPLEMENTATION_2026-10-11.md).

## Risk register

| Area | Proposed risk / control |
| --- | --- |
| Startup/install/reset, camera correction | No native source move. Preserve owner/gate, transaction headers and worker control; stop on any change beyond paths |
| Research watcher | Wrong source registry could accidentally include second DLL entry; require unchanged 54-file production C++ set |
| Locale registry/RC | Wrong include depth/root can fail RC or drag inactive copies into build; move single header, preserve IDs, compile RC and all consumers |
| Overlay composition/input/presenter | No planned lifecycle move; exact existing source preserved except resource include |
| Tests/tools | Source registry extraction and source-inspecting audits must keep working; separate full runner failure from successful focused checks |
| Dirty tree/Git | Preserve user's staged and unstaged work; no destructive commands or staging assumptions |
| Production/diagnostic packaging | Separate profiles/output filenames; no release-assets edits or installed ASI |
| Native safety evidence | N03 Partially mitigated; N04/N05 Open; N06 Partially verified / Open; retained exact-final-ASl gate Pending. Path validation cannot close these |

## Deferred decisions

Renderer experiment gates and include-only hook_set.cpp remain outside batches. Existing missing retired-script documentation and historical path drift may be handled by a separately approved documentation correction, not source reorganization. Runtime/ODR/header self-sufficiency improvements require their own justification and validation.

Result: the approved structural work is complete. The 143-file source tree now has the four approved paths in place. The remaining runner failure and Git diff limitation are recorded for their own follow-up; they did not require changing code behavior to complete these path moves.

