# Overlay Directory Reorganization Plan

**Date:** 2026-10-11  
**Status:** Proposed only; requires separate approval before any move.  
**Scope:** Organize existing `src/overlay/` files into responsibility-based directories without changing runtime behavior or ownership.

## Goal and invariants

The goal is to make the current subsystem easier to navigate, not to redesign it. Preserve all filenames, translation-unit boundaries, public declarations, data, control flow, exception boundaries, thread ownership, lock ordering, resource lifetime, startup/readiness ordering, and Overlay/camera contracts. The shared plugin `RuntimeState` and correction gate remain untouched. Keep `Renderer` and `composition_runtime` intact as whole owners.

Do not introduce new abstractions, interfaces, single-file directories, forwarding headers, compatibility aliases, or broad namespace changes. Use ordinary include-path edits and build/test reference updates only. Do not mass-rewrite historical research documents or immutable evidence manifests; active scripts and current-path inventories must be updated.

## Proposed destination layout

The complete file-by-file map is in `OVERLAY_FILE_MOVE_MATRIX.md`.

```text
src/overlay/
  composition/    Composition/discovery root and its in-repo facade
  input/          Bounded event bridge and Overlay input state
  localization/   Catalogs, keys, formatting, validation, fonts, resources,
                  and game-language synchronization adapter
  presentation/   Camera/feature projections and UI/layout/tooltip/placement helpers
  presenter/      Renderer owner, D3D11 ImGui backend, presenter lifecycle/policies
  optional_overlay_boundary.hpp
```

Expected dependency direction is approximately:

```text
plugin runtime -> composition -> presenter -> presentation
                         |           |             |
                         +-> input <-+             +-> localization
                                     +-------------> localization
```

This is a navigation guide, not a new enforced layering architecture. Existing dependencies should stay as they are unless a path needs updating. In particular, `composition_runtime` consumes presenter, input and localization; the presenter consumes presentation, input and localization. The proposed grouping must not introduce an include cycle or move code to manufacture a cleaner-looking graph.

## Small-batch sequence

Each batch is an independent, path-only change. Complete and verify one batch before starting the next; stop if any needed edit becomes behavioral or ownership-related.

### Batch 1 — semantic presentation projections

Move to `presentation/`: `aspect_ratio_format.hpp`, `camera_integration.cpp/.hpp`, `camera_state_view.cpp/.hpp`, `feature_presentation.cpp/.hpp`.

Mechanical references: `build.cmd`; camera writer and Camera Integration/feature harness inputs in `test.cmd`; `tests/plugin/camera_writer_transaction_harness.cpp`; `tests/overlay/camera_integration_harness.cpp`, `feature_presentation_harness.cpp`; renderer includes; active localization audit reads for camera state and feature presentation. Search actual-adapter response files for `camera_integration.cpp` before concluding there are no more source-list references.

Checks: compile/run the camera integration, camera writer transaction and feature presentation harnesses; run the localization resource/source audit; run `test.cmd` and `build.cmd`. Compare the moved files’ content hashes to the pre-move snapshot after normalizing only include path changes.

### Batch 2 — presentation layout, selector and tooltip support

Move to `presentation/`: `hotkey_binding_presentation.hpp`, `inline_hotkey_layout.hpp`, `overlay_layout_metrics.cpp/.hpp`, `placement_config.cpp/.hpp`, `presentation_contracts.hpp`, `selector_documentation_view.cpp/.hpp`, `selector_examples_geometry.hpp`, `selector_tooltip_state.hpp`, `setting_tooltip_content.hpp`, `tooltip_row_layout.hpp`.

Mechanical references: `build.cmd`; `test.cmd` layout/placement inputs; plugin and overlay harness includes; `renderer_runtime.hpp/.cpp` includes; `tests/overlay/localization_harness.cpp`; selector, tooltip, geometry, placement and presentation-contract reads in `tests/runner/localization_catalog_audit.ps1`.

Checks: focused layout, placement, selector tooltip, feature/camera presentation and composition-state harnesses as applicable; localization audit; `test.cmd`; `build.cmd`. Confirm UI contracts and data remain textually identical except paths.

### Batch 3 — localization and game-language synchronization

Move all 16 localization files listed in the move matrix into `localization/`, including `localization_resources.rc`, resource IDs, and `game_language_reader.cpp/.hpp` as one coherent resource/API group.

Mechanical references: `build.cmd` and `test.cmd` resource compiler inputs and compiled locale source list; `tests/runner/native_runtime_evidence.ps1`; `research/evidence/native-runtime/N06/tools/n06_validate.ps1` and `n06_m01_validate.ps1`; `src/localization/locale_registry.hpp`; renderer/composition includes; every literal localization path in `tests/runner/localization_catalog_audit.ps1`. Keep the `.rc` payload resource paths stable and update its ID include relative to repository root.

Checks: run the resource compiler from both supported script paths; localization harness and full locale/resource audit; game-language reader source-level contract audit; `test.cmd`; production `build.cmd` and diagnostic build if the active build entry point consumes the same resources. Verify embedded resource IDs/counts and catalog/font audit output are unchanged.

### Batch 4 — input subsystem

Move `input_state.cpp/.hpp` to `input/`.

Mechanical references: `build.cmd`; `test.cmd` composition and input harness source lists; all Overlay/plugin harness includes; renderer/composition includes; input source reads in `tests/runner/localization_catalog_audit.ps1`. `hotkey_binding_presentation.hpp` has already moved in Batch 2 and remains presentation-only.

Checks: input and composition presenter-state harnesses, localization/source audit, `test.cmd`, `build.cmd`. Confirm singleton declarations/definitions remain unique and the bounded queue, mutex, atomics and message forwarding are unchanged.

### Batch 5 — presenter and rendering owner

Move to `presenter/`: `composition_presenter_state.cpp/.hpp`, `imgui_d3d11_renderer.cpp/.hpp`, `placement_save_state.hpp`, `presenter_runtime_policy.hpp`, `renderer_runtime.cpp/.hpp`, `renderer_state.cpp/.hpp`.

Mechanical references: `build.cmd`; `test.cmd` composition and camera writer policy harness source lists; all renderer/composition/plugin/test includes; every explicit presenter/renderer/policy/placement-state path in `tests/runner/localization_catalog_audit.ps1`; any active source response lists. Do not split `renderer_runtime.cpp` or move ImGui/D3D/DComp members to another owner.

Checks: composition state and camera writer transaction harnesses, localization/source audit, `test.cmd`, production and diagnostic builds. Review compile dependency output for no duplicated definitions and inspect the diff to ensure the Renderer class layout, ImGui calls, COM ownership, and frame ordering are unchanged.

### Batch 6 — composition root, last

Move `composition_runtime.cpp/.hpp` to `composition/` only after all its dependencies have their final paths.

Mechanical references: `build.cmd`; runtime/actual-adapter source response lists in `test.cmd` and `tests/runner/native_runtime_evidence.ps1`; N06 source-generation/validation scripts if they enumerate this file; `src/plugin/runtime.cpp` include; localization audit’s composition source reads; intra-folder includes. Re-scan active scripts globally for the old paths.

Checks: Overlay startup/composition and actual-adapter harnesses, localization/source audit, `test.cmd`, production and diagnostic builds. Review the final path diff for unchanged exports, worker startup/stop/join order, readiness flags, HWND subclass/WinEvent registration and teardown, renderer mutex, input bridge activation, and Overlay wake/publication order. Do not launch the game as part of this source-organization task unless separately authorized.

## Cross-batch verification gates

For every batch:

1. Record the exact moved-file set and compare file hashes before/after; allow only expected include/path-line edits.
2. Search active code, build files, test runners, resource scripts, and validation scripts for stale old paths. Exclude historical reports/manifests from automated rewrite; review them separately and leave immutable historical evidence intact.
3. Run the batch’s focused harnesses and the locale/resource audit where applicable, followed by `test.cmd` and production build. Diagnostic build is required at least after Batches 3, 5 and 6, and again on the final tree.
4. Record exit codes and artifact SHA-256s. Treat any test/build difference as a stop-and-investigate condition, not as permission to expand scope.
5. Review `git diff --find-renames` and the complete working-tree status so pre-existing user changes remain intact and no accidental file content rewrite is hidden by the moves.

No validation was run for this plan. The workspace currently has unrelated dirty changes, including runtime, build/test, vendor and evidence files; before implementation, identify and preserve those exact edits. Do not stage, revert, or overwrite them as part of the directory reorganization.

## Risk classification

| Risk | Level | Reason | Guardrail |
|---|---|---|---|
| Stale include/build/test path | Medium | Many explicit references; resource compiler paths are separate from C++ source lists. | Batch-by-batch search plus focused and full compilation/tests. |
| Runtime ownership/lifecycle drift | High if code is split; low for a faithful move | Composition and renderer files own threads, hooks, locks and presentation lifetime. | Move each runtime owner intact; no extraction or body edits. Composition last. |
| Resource embedding change | Medium | `.rc`, resource IDs and locale registry cross directory boundaries. | Move resource files together; validate `rc`, catalogs, IDs and final linked artifact. |
| Historical evidence corruption | Medium | Research reports/manifests contain old source paths by design. | Update active tools/current inventories only; preserve historical records. |
| Existing uncommitted work | High operational risk | Working tree is already dirty in unrelated areas. | Preserve, avoid broad rewrites, inspect move-aware diff and status. |

## Approval boundary

This plan is a proposal only. Before the first move, approve the target directory map and batch sequence. Any request to split either runtime owner, change include/API boundaries beyond mechanical paths, alter hooks/workers/resources/lifecycle, or modify camera/shared-state behavior requires a separate design review and approval.
