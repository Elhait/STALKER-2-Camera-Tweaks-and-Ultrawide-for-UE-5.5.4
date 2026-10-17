# v2.1 → potential v3.0: change inventory

**Prepared:** 2026-10-09 (updated after R01/R02 validation)  
**Purpose:** inventory the present unreleased source-tree delta relative to the v2.1 production package, and assess what a hypothetical 3.0 label would represent. This is not a release approval, final changelog, or claim that v3.0 exists.

## Executive summary

The current candidate is a substantial post-v2.1 evolution across configuration, camera-state authority/projection, Gameplay/Cinematics/Dialogue lifecycle handling, Overlay configuration and presentation, localization/font resources, and regression infrastructure. It is not merely the `runtime.cpp` decomposition: that work is only at B1, whose change is a private accessor to the existing resident runtime owner; no callbacks or lifecycle orchestration were moved.

The changes appear to be predominantly correctness hardening, observability, configuration/UI improvements, and internal validation—not a wholesale replacement of the mod's purpose or a new incompatible user-facing product contract. On the evidence available here, “possible 3.0” is a versioning proposal, not an inherent technical requirement. A v2.2 release could also be a reasonable label if compatibility and release policy permit. The version decision remains with the project owner after runtime acceptance.

## Comparison basis and confidence

| Item | Evidence used | Interpretation / limitation |
|---|---|---|
| Public v2.1 artifact | `release-assets/STALKER2CameraTweaks-UE5.5.4-v2.1.zip`; SHA-256 `32627CADAD9BB309FCF34000C5DBBA7BCA5D0A38BAD6AA5EBB971E1FBE15C1C1` | Strongest available release baseline. Embedded ASI: 2,739,200 bytes, SHA-256 `DF02F40BDEDA42666D2395F9558FA53F2996AFBAB8FBA54D1BE32995068E57A1`; INI: 5,524 bytes, SHA-256 `79596E9D13693772BE7F0F029B374B62DAA639664B8D8667D4547AE9C1B6291F`. |
| Source anchor | `c844d2e379f922fe72f46e98c3e9ff61dd8eaeb0` (2026-09-29, `feat: migrate overlay to DComp and smooth post-cinematic HorPlus recovery`) | This is the source-tree anchor associated with the v2.1 release-era state and the local `GITHUB_RELEASE_BODY.md`. The exact byte-for-byte source-to-ASI reproducibility of the archived artifact was not established in this audit. |
| Misleading tag | Local `v2.1.0` resolves to `56dbeb9788154da84f47fed8138945afc401e823` (2026-09-27, `release: prepare v2.0.0`) | Not used as the v2.1 baseline: its commit metadata identifies v2.0.0. This discrepancy should be corrected or documented in release metadata separately. |
| Candidate state | Current index/worktree status, current production source, project contracts, and dated B0/V0/B1 and camera-state reports | The current candidate includes staged modifications and added files. Research plans, reports, probes, and harnesses are distinguished below from product changes. |
| Diff completeness | `git status --short --untracked-files=all` enumerated the full changed-path set. A conventional `git diff` invocation in this checkout returned `fatal: this operation must be run in a work tree`. | The report inventories all paths exposed by status and categorizes the source changes; it does not claim a line-by-line unified patch review. The repository limitation and missing exact patch view reduce confidence in detecting incidental line-level changes. |
| Runtime | The project owner confirms the final runtime regression matrix completed successfully. The 2026-10-09 game log independently records production ASI hash `53763B95362F6536FCB40A9AED30B88C817855E8CF6F3C988E96A6B24FC62465` and the first-generation forced-16:9 Cinematic ENTER; see the release-candidate update below. | Runtime regression is reported PASS; the attached log provides direct evidence for the R01 policy-change path. |

The historical [`2.0.1 → 2.1 release delta`](V2_0_1_TO_V2_1_PRODUCTION_RELEASE_DELTA_2026-09-29.md) describes the earlier transition to DirectComposition and post-cinematic interpolation. Those are **already part of the v2.1 baseline**, not new changes in this report.

## Product and behavior changes since the v2.1 baseline

| Area | Current candidate change | User impact / status |
|---|---|---|
| Gameplay correction default | `Gameplay.Mode` now defaults to `RuntimeCombine`, described in the generated INI as recommended. Existing `AspectRecalculation` and `HorPlus` remain selectable and retain their own semantics. RuntimeCombine synchronizes the Gameplay writer aspect only when needed, confirms the write/readback, then applies HorPlus; it fails closed when the aspect cannot be confirmed. | New recommended/default behavior for fresh configuration. Existing user values are intended to persist. This is a meaningful default change, not a removal of old modes. |
| Independent module enable/disable settings | The candidate has separate `Enabled` settings for Gameplay, Cinematics, Dialogue, and Overlay; there is no single master switch that disables every module. This is not wholly new versus v2.1: `Gameplay.Enabled` already existed. In the v2.1 INI, Cinematics had no dedicated `Enabled` key and used the legacy `Native` aspect/mode contract to leave native framing; Dialogue had no dedicated `Enabled` key, but `Dialogue.Zoom=Disabled` already disabled Dialogue zoom correction. The candidate makes Cinematics and Dialogue enablement explicit, while `Overlay.Enabled=false` newly disables the entire Overlay subsystem at startup. `Hotkeys.Enabled` is separate and controls camera hotkey actions, not module correction. | Clarifies which controls are new versus inherited. Per-module controls are independent; disabling Overlay does not disable camera corrections. |
| FOV meaning and tolerance | Canonical semantic FOV equivalence is centralized at ±1.0°; the tolerance is explicitly not reused for interpolation, endpoint, source identity, or numeric formatting decisions. | Correctness contract and guard against treating nearby FOV values as proof of common source/ownership. |
| Gameplay source/baseline | Gameplay FOV observations, production baseline, shared/presentation facts, writer identity, provenance, and freshness are more explicitly separated. Baseline qualification remains distinct from shared presentation qualification. | More conservative attribution and display; not a “one float for all” consolidation. The production baseline may remain separately owned. |
| Cinematic lifecycle and recovery | Lifecycle authority, ENTER/EXIT transaction, source association, continuation, and recovery retirement have explicit helpers/contracts. Validated RuntimeCombine EXIT recovery retains transformed output across the confirmed native interpolation segment and then resumes normal RuntimeCombine. | Builds on v2.1’s validated post-cinematic HorPlus interpolation. The added ownership/evidence gates constrain when transformation occurs; ambiguous samples remain pass-through. Offline coverage exists; current report does not claim new runtime acceptance for every branch. |
| Cinematic ENTER aspect-policy selection (R01) | `TraceCinematicEnter` resolves the effective aspect from the accepted ENTER selection's `aspectPolicy`, rather than the previously active policy. This fixes a configuration change such as `Auto → 16:9` being evaluated with the old wide aspect on the next ENTER. | Targeted actual-adapter regression checks the selected policy, production writer output, and published output. Runtime evidence for the first generation is recorded below; no camera math or lifecycle contract was changed. |
| Transition Evidence | Evidence is projected separately from live FOV. Accepted cinematic ENTER evidence can remain available through rapid cinematic generations/continuation; after lifecycle end it is presented as historical/last-validated evidence, not active camera state. | Addresses the observed rapid-generation evidence-loss case without converting evidence into live FOV. A user-provided runtime sequence showed generations 1, 2 and 4 with evidence retained and later shown as last validated; that is scenario-specific evidence, not universal coverage. |
| Dialogue lifecycle | Dialogue candidate/confirmation, source provenance, recovery exclusion, rearm, and its boundary from Gameplay recovery received additional hardening. The visible mode cycle is Adaptive → Reduced → Disabled → Adaptive; the stale `Native` entry was removed from the cycle description/behavior where applicable. | Dialogue’s existing modes and independent recovery contract remain; no claim that Gameplay interpolation rules are applied to Dialogue. Disabled dialogue correction follows the native game zoom (documented as defaulting to 70° FOV). |
| Shared Camera State | Publication and projection now make per-fact validity, source/provenance, freshness, and lifecycle treatment more explicit. Current, Retained, Historical, Unavailable, Unknown, and last-validated states are kept semantically distinct. | Diagnostics/Overlay can communicate what is current versus retained or historical without asserting a globally atomic snapshot. Camera State remains observational, not a correction authority. |
| Overlay optionality | New startup-only `[Overlay] Enabled=true|false`, defaulting true for compatibility. False skips the Overlay startup path and is intended to leave camera correction and ordinary camera hotkeys independent. | Allows an A/B test of Overlay involvement in graphics issues. The code has a disabled-by-configuration log marker. Offline coverage exists; no new in-game OFF/ON A/B is established by this report. |
| Overlay technical panel | The right panel uses canonical English technical labels/values, compact FOV / Camera Transition / Cinematics subheadings, a shared aspect-ratio formatter, and full-row tooltip hit targets in Current State/Camera Integration. Expanded Camera State is technical and tooltip-free. | User-visible presentation changes. Aspect display is `round(aspect × 9):9 (X.XXXX)` for valid values, including unusual ratios; underlying values/comparisons are unchanged. Tooltip coverage was expanded across all 18 catalogs. |
| Tooltip and status text | Dialogue enable/disable tooltip explains native zoom behavior. Overlay status wording and tooltip behavior were corrected while retaining the accepted all-statuses tooltip model; Waiting is shown as the canonical English technical value on the right. | UI clarity/localization only; does not change evidence or status contracts. |
| INI presentation and defaults | Generated INI gains clearer spacing/grouping, category separation, and plain `Variable = value` formatting rather than padded key alignment. RuntimeCombine is the Gameplay default. | Readability and defaults. Existing persistence/migration contracts are intended to remain, including non-destructive INI persistence. |
| Localization and font coverage | The 18 locale catalogs were updated for the right panel/tooltips and related strings. Font subsets and glyph contract were regenerated from the current catalogs; the previously missing catalog glyphs (including the `runtime_combine` tooltip cases) are now covered. | Full catalog/resource audit passes for all 18 locales: 1,146 unique required codepoints, zero missing. This establishes embedded glyph coverage, not visual proof of shaping, RTL, or every locale at runtime. |

## Runtime and architecture changes

### Camera transaction/state helpers

New or expanded helpers make existing transactional boundaries explicit: `camera_correction_gate.hpp`, `camera_writer_transaction.hpp`, `cinematic_lifecycle_transaction.hpp`, `dialogue_boundary_transaction.hpp`, `cinematic_exit_authority.hpp`, `cinematic_recovery_retirement.hpp`, and `camera_state_publication.hpp`. Their intent is to centralize decisions over the existing runtime state and preserve the ordering of writes, confirmation, publication, wake, and recovery. They do not constitute a new user-facing camera mode by themselves.

Additional diagnostic identity/provenance/context/trace helpers record the source and lifecycle evidence used by those decisions. Instrumentation is observational; the architecture contract says Diagnostic Mode must not change camera output, ownership, lifecycle, or state-machine timing.

### `runtime.cpp` decomposition status

Only B1 is complete. `src/plugin/runtime_internal.hpp` exposes an opaque private `plugin::detail::RuntimeState` binding via `GetRuntimeState()`. The resident allocation, concrete state definition, callbacks, `Initialize`, `ResetAllRuntimeResources`, and hook installation remain in `runtime.cpp`. B1 did not move camera callbacks or lifecycle orchestration, and did not introduce another resident owner, correction gate, or hook set. B2–B7 are not part of the implemented candidate and were not authorized/started in the recorded B1 report.

## Validation and release evidence

The B1 results report records 168 actual-adapter fixture-process runs. After the R01/R02 fixes, a fresh `test.cmd` run exited 0, including the 18-locale resource audit and 128 actual-adapter fixture processes (64 scenarios with Diagnostic Mode OFF and ON). Production `build.cmd` and the diagnostic build both exited 0. The fixtures exercise production callback adapters with controlled dependencies; they do not prove full game `Initialize`/hook-install/reset orchestration or every native runtime sequence.

### Release-candidate update — R01/R02 (2026-10-09)

| Finding | Change and evidence | Status |
|---|---|---|
| R01 — first/new Cinematic ENTER used stale aspect policy | Production callback now resolves aspect from `selection.aspectPolicy`. Regression passes with an old active `Auto` policy and a newly selected `Forced16x9`; it checks the ENTER target, actual writer register output, and published output. In the game log, generation 1 reports `aspectPolicy=16:9`, effective aspect `1.77778`, and target `95.1311`; the first post-ENTER writer sample retains `95.1311` at aspect `1.77778`. | Fixed; offline and this targeted runtime scenario PASS. |
| R02 — missing embedded catalog glyphs | Rebuilt all six checked-in catalog font subsets from the pinned source fonts and regenerated the derived glyph contract from current catalogs/locale metadata. No translation text or font-loading architecture was changed. | Fixed; full glyph/resource audit and `test.cmd` PASS, 18/18 locales, 1,146 unique codepoints, missing=0. |

The runtime log identifies the loaded module with SHA-256 `53763B95362F6536FCB40A9AED30B88C817855E8CF6F3C988E96A6B24FC62465`, matching the production ASI built after these fixes. It also records later `Auto` ENTERs at aspect `3` with target `135.398`, plus successful `CINEMATIC_RECOVERY_COMPLETED` events. The log directly confirms R01 for this run. The project owner separately confirms the overall runtime regression matrix passed without issues.

### Glyph-test status and remaining release gates

The earlier U+0148 discrepancy is resolved in the current generated contract: U+0148 remains explicitly pinned in the common subset, and the complete current font/resource audit passes. The separately identified catalog glyph gaps are also now covered. These are offline embedded-font coverage results, not runtime visual proof of shaping or RTL rendering.

The R01/R02 candidate was tested and built, the log confirms the exact production ASI hash and targeted R01 behavior, and the project owner reports the final runtime regression matrix passed. This report still does **not** establish:

- a final release package assembled and independently inspected at the exact candidate revision;
- successful runtime presentation for every locale/font profile;
- closure of startup/install/reset orchestration evidence gaps;
- release package assembly/independent inspection or publication approval.

## Product changes versus repository-only work

| Present in a production package if built from this candidate | Development/research-only; not an end-user feature |
|---|---|
| Camera/configuration/Overlay source changes; embedded locale catalogs and font resources; updated INI template; production diagnostic marker/code; build-selected product resources. | B0/V0/B1 plans and reports; F1–F10 research plans/audits; runtime decomposition baseline/move ledger and capture scripts; Ghidra research script; offline harnesses/fixture inputs and localization audit runner; this report. |

The repository has not been packaged as a v3.0 release in the scope of this task. A release package must be inspected independently; a source-tree file or successful build alone does not establish package contents.

## Complete changed-path inventory observed on 2026-10-08

The paths below are the full set reported by `git status --short --untracked-files=all` for the 2026-10-08 inventory snapshot. The R01/R02 changes made on 2026-10-09 are listed separately below. `M` means modified and `A` means newly added relative to the index/HEAD status view; they are not all product-shipped files.

### Product source, configuration, resources, and user-facing docs

```text
README.md
build.cmd
docs/ARCHITECTURE.md
docs/RELEASE_ARCHIVE_README.txt
docs/SAFETY_INVARIANTS.md
locales/README.md
locales/ar.json
locales/cs.json
locales/de.json
locales/en.json
locales/es-419.json
locales/es-ES.json
locales/fr.json
locales/it.json
locales/ja.json
locales/ko.json
locales/pl.json
locales/pt-BR.json
locales/ru.json
locales/sr-Cyrl.json
locales/tr.json
locales/uk.json
locales/zh-Hans.json
locales/zh-Hant.json
assets/fonts/CatalogSubsets/NotoSansJP-Catalog.ttf
assets/fonts/CatalogSubsets/NotoSansKR-Catalog.ttf
assets/fonts/CatalogSubsets/NotoSansMono-Catalog.ttf
assets/fonts/CatalogSubsets/NotoSansSC-Catalog.ttf
assets/fonts/CatalogSubsets/NotoSansTC-Catalog.ttf
assets/fonts/CatalogSubsets/README.md
assets/fonts/CatalogSubsets/glyph-contract.json
src/camera/camera_state_publication.hpp
src/camera/camera_state_snapshot.cpp
src/camera/camera_state_snapshot.hpp
src/camera/fov_observation.cpp
src/camera/fov_observation.hpp
src/camera/fov_semantics.hpp
src/cinematics/cinematic_exit_authority.hpp
src/cinematics/cinematic_fov.cpp
src/cinematics/cinematic_fov.hpp
src/cinematics/cinematic_recovery_retirement.hpp
src/cinematics/cinematic_selection.hpp
src/config/config_repository.cpp
src/config/config_template.cpp
src/config/feature_config.cpp
src/config/feature_config.hpp
src/diagnostics/camera_log_probe.hpp
src/diagnostics/camera_writer_identity.hpp
src/diagnostics/cinematic_baseline_provenance.hpp
src/diagnostics/cinematic_continuation_trace.cpp
src/diagnostics/cinematic_continuation_trace.hpp
src/diagnostics/cinematic_lifecycle_context.hpp
src/diagnostics/diagnostic_runtime.hpp
src/diagnostics/dialogue_source_provenance.cpp
src/diagnostics/dialogue_source_provenance.hpp
src/diagnostics/overlay_activation_trace.hpp
src/dialogue/dialogue_state.cpp
src/dialogue/dialogue_state.hpp
src/gameplay/gameplay_state.cpp
src/gameplay/gameplay_state.hpp
src/gameplay/horplus_gameplay.cpp
src/overlay/aspect_ratio_format.hpp
src/overlay/camera_integration.cpp
src/overlay/camera_integration.hpp
src/overlay/camera_state_view.cpp
src/overlay/camera_state_view.hpp
src/overlay/composition_runtime.cpp
src/overlay/composition_runtime.hpp
src/overlay/feature_presentation.cpp
src/overlay/localization_keys.cpp
src/overlay/localization_keys.hpp
src/overlay/presenter_runtime_policy.hpp
src/overlay/renderer_runtime.cpp
src/overlay/setting_tooltip_content.hpp
src/plugin/camera_correction_gate.hpp
src/plugin/camera_writer_transaction.hpp
src/plugin/cinematic_lifecycle_transaction.hpp
src/plugin/dialogue_boundary_transaction.hpp
src/plugin/optional_overlay_startup.hpp
src/plugin/runtime.cpp
src/plugin/runtime_internal.hpp
src/plugin/runtime_settings.hpp
tools/fonts/rebuild_catalog_subsets.py
tools/generate_ini_documentation.ps1
```

### Tests, test runners, and validation inputs

```text
test.cmd
tests/camera/camera_state_snapshot_harness.cpp
tests/camera/fov_observation_harness.cpp
tests/cinematics/cinematic_exit_authority_harness.cpp
tests/cinematics/cinematic_fov_harness.cpp
tests/cinematics/coordinator_recovery_harness.cpp
tests/cinematics/horplus_post_exit_recovery_harness.cpp
tests/config/config_persistence_harness.cpp
tests/config/gameplay_mode_harness.cpp
tests/config/runtime_settings_api_harness.cpp
tests/diagnostics/camera_writer_identity_harness.cpp
tests/diagnostics/cinematic_lifecycle_context_harness.cpp
tests/diagnostics/diagnostics_gate_harness.cpp
tests/diagnostics/dialogue_source_provenance_harness.cpp
tests/dialogue/candidate_hardening_harness.cpp
tests/dialogue/post_cinematic_exclusion_harness.cpp
tests/dialogue/recovery_rearm_harness.cpp
tests/gameplay/gameplay_camera_safety_harness.cpp
tests/gameplay/gameplay_mode_transition_harness.cpp
tests/overlay/activation_trace_harness.cpp
tests/overlay/camera_integration_harness.cpp
tests/overlay/composition_presenter_state_harness.cpp
tests/overlay/feature_presentation_harness.cpp
tests/overlay/localization_harness.cpp
tests/overlay/placement_config_harness.cpp
tests/overlay/semantic_snapshot_harness.cpp
tests/plugin/camera_correction_gate_harness.cpp
tests/plugin/camera_setting_transaction_harness.cpp
tests/plugin/camera_writer_transaction_harness.cpp
tests/plugin/cinematic_lifecycle_transaction_harness.cpp
tests/plugin/dialogue_boundary_transaction_harness.cpp
tests/plugin/optional_overlay_startup_harness.cpp
tests/plugin/runtime_native_adapter_harness.cpp
tests/plugin/runtime_owner_binding_probe.cpp
tests/regression/module_ownership_harness.cpp
tests/regression/runtime_evidence_replay_harness.cpp
tests/runner/localization_catalog_audit.ps1
tests/runner/runtime_native_adapter_inputs.ps1
```

### Plans, reports, evidence, and research tools

```text
TESTING_AND_RESEARCH.md
research/completed/CAMERA_MODULE_REMEDIATION_F1_F8_TASK_PLAN.md
research/completed/MODULAR_GAMEPLAY_CINEMATICS_DIALOGUE_REFACTOR_TASK_PLAN.md
research/completed/RUNTIME_CPP_DECOMPOSITION_IMPLEMENTATION_PLAN_2026-10-08.md
research/active/CAMERA_STATE_SEMANTIC_CONTRACT_AUDIT_2026-10-05.md
research/active/F9_GAMEPLAY_ASPECT_AUTHORITY_PLAN_2026-10-06.md
research/active/F9_GAMEPLAY_WRITER_ASPECT_VIEWPORT_DIVERGENCE_2026-10-05.md
research/active/HORPLUS_GAMEPLAY_FOV_STATE_DESIGN_TASK_PLAN.md
research/active/OVERLAY_DEBUGGING_LEDGER_2026-09-29.md
research/active/OVERLAY_SHARED_CAMERA_STATE_F10_LIFECYCLE_PLAN.md
research/active/OVERLAY_SHARED_CAMERA_STATE_F6_F8_REMEDIATION_PLAN.md
research/active/OVERLAY_SHARED_CAMERA_STATE_F9_CONSOLIDATION_PLAN.md
research/active/OVERLAY_STARTUP_BOUNDARY_AUDIT_2026-09-29.md
research/evidence/runtime-decomposition/BASELINE_2026-10-08.md
research/evidence/runtime-decomposition/MOVE_LEDGER_2026-10-08.md
research/reports/F2_CONTINUATION_SEMANTIC_REVIEW_2026-10-02.md
research/reports/OVERLAY_RIGHT_PANEL_TEXT_INVENTORY_2026-10-08.md
research/reports/OVERLAY_SHARED_CAMERA_STATE_ARCHITECTURE_AUDIT_2026-10-07.md
research/reports/RUNTIME_CPP_DECOMPOSITION_AUDIT_2026-10-08.md
research/reports/RUNTIME_CPP_DECOMPOSITION_B0_V0_RESULTS_2026-10-08.md
research/reports/RUNTIME_CPP_DECOMPOSITION_B1_RESULTS_2026-10-08.md
research/tools/Ghidra206WriterContext.java
research/tools/capture_runtime_decomposition_baseline.ps1
research/tools/runtime_decomposition_move_ledger.ps1
```

### R01/R02 paths changed after the 2026-10-08 inventory snapshot

```text
src/plugin/runtime.cpp
tests/plugin/runtime_native_adapter_harness.cpp
assets/fonts/CatalogSubsets/NotoSansArabic-Catalog.ttf
assets/fonts/CatalogSubsets/NotoSansJP-Catalog.ttf
assets/fonts/CatalogSubsets/NotoSansKR-Catalog.ttf
assets/fonts/CatalogSubsets/NotoSansMono-Catalog.ttf
assets/fonts/CatalogSubsets/NotoSansSC-Catalog.ttf
assets/fonts/CatalogSubsets/NotoSansTC-Catalog.ttf
assets/fonts/CatalogSubsets/glyph-contract.json
```

## Bottom line

Relative to the actual v2.1 package, the candidate changes several user-visible defaults and Overlay/INI behaviors and substantially tightens camera-state qualification and evidence handling. R01 is fixed and confirmed in the game on the exact built production ASI; R02 glyph coverage, the complete offline suite, production/diagnostic builds, and the owner-reported runtime regression matrix all pass. B2–B7 remain unimplemented. Remaining work is final release-package assembly and independent inspection. Calling the release 3.0 is a product/versioning choice; the available technical evidence alone does not require a major-version bump.
