# Overlay Semantic Snapshot and Camera Integration — Implementation Report

Date: 2026-09-22

## Outcome

Added a read-only semantic snapshot for the overlay, a deterministic
Camera Integration projector, and contextual status text in the Gameplay,
Cinematics, Dialogue, and Camera Integration sections. No camera hook,
transform, transition, or gameplay/cinematic/dialogue decision was added or
changed. The current-FOV panel remains out of scope because the existing camera
observations do not establish a generally fresh active-path FOV value.

## Semantic sources exposed

Each semantic fact carries its own validity, source label, and freshness
classification. The snapshot documentation and API explicitly state that facts
are sampled independently and do not constitute one atomic camera state.

| Overlay field | Production source | Limit / meaning |
| --- | --- | --- |
| Configured Gameplay, Cinematic, and Dialogue settings | Existing runtime setting atomics read by `ReadRuntimeSettings` | Current configured values only; not proof of camera application. |
| Gameplay hook availability | `g_gameplayHookGate` | Hook/gate availability, not proof that a specific frame was transformed. |
| Gameplay pending transitions | Existing enable-apply, disable-restore, and mode-transition pending atomics | Reports queued transition work only. |
| Cinematic aspect component availability | Result of transactional Cinematic component initialization | Whether the aspect component initialized; not current output aspect. |
| Cinematic FOV lifecycle availability | `g_cinematicLifecycleObservationAvailable` | Lifecycle hook capability; active selected policies are separately reported. |
| Active Cinematic selection | `g_cinematicSelectionValid` plus captured active aspect/FOV policy atomics | Lifecycle-scoped selection; no claim that the UI can infer displayed FOV. |
| Dialogue hook/capability | Dialogue boundary installation result and existing non-Native capability atomic | Native game Dialogue remains distinct from modded non-Native capability. |
| Dialogue lifecycle and active policy | `g_dialoguePhase` and `g_activeDialoguePolicy`, read under `g_dialogueMutex` | Lock-protected lifecycle snapshot. |
| Gameplay baseline usability | `GameplayBaselineStore::Read()` and production `IsUsableGameplayBaseline()` predicate | Only a retained-validity boolean is exported; no baseline FOV is displayed. |
| Presentation coordinator | Existing coordinator atomic | Current coordinator value at read time. |
| Runtime viewport aspect | Strict current-process client-rectangle resolver | Invalid/missing client dimensions yield invalid evidence; it does not substitute monitor aspect. |

## Camera Integration rules

- `Auto` resolves to the valid current client viewport aspect.
- Forced policies resolve to their numeric ratios; matching compares numeric
  values with a `0.01` aspect tolerance rather than comparing policy enums.
- `Native` aspect is `Cannot assess`; the configured policy helper's nominal
  native value is not treated as authoritative runtime evidence.
- Missing, invalid, NaN, or infinite viewport evidence fails closed.
- `NativeHorPlus` is always classified as an independent FOV path. Numeric FOV
  equality cannot turn it into a Gameplay-linked path.
- `GameplayHorPlus` is classified as Gameplay-linked only when its lifecycle
  path is available, the cinematic aspect policy enables the production FOV
  transform, and the retained Gameplay baseline passes the production
  usability predicate. With `AspectRatio=Native`, the production cinematic
  transform is disabled and the FOV path is not reported as active. If an
  enabled path has an invalid baseline, the projector reports the
  authored-baseline fallback, not a linked path.
- Overall `Configuration aligned` additionally requires Gameplay configured
  enabled in HorPlus, a matching aspect, and the cinematic aspect component
  capability. The label does not promise a transition-free presentation.

## Overlay presentation

- Gameplay shows configured values, hook availability, pending-transition
  status, and explicitly says current camera effect is not directly observable.
- Cinematics labels settings as configured for the next cinematic and shows
  active captured selection only when valid.
- Dialogue labels the configured next-dialogue policy and shows observed phase
  plus captured active policy when available.
- Camera Integration shows resolved aspect comparison, FOV path, and overall
  status. No current native/effective FOV numbers or diagnostic-only
  `CameraStateSnapshot` are consumed.

## Deterministic coverage and validation

Added `camera_integration_harness` coverage for Auto and arbitrary 3:1 aspects,
forced 32:9 match, forced 21:9 mismatch, Native aspect uncertainty,
NativeHorPlus independence, missing-baseline fallback, invalid NaN/Inf/missing
viewport data, missing component capabilities, and disabled Gameplay. Added
`semantic_snapshot_harness` coverage for the read-only API and independent
validity/provenance of configured, capability, and observed facts.

```yaml
semantic_snapshot_batch_test_cmd_registry: 38 sources / 38 compiled / 38 executed
test_cmd: PASS
production_build: PASS
overlay_settings_build: PASS
git_diff_check: PASS
runtime: NOT_PERFORMED
camera_behavior_changed: NO
```

The build emitted existing dependency warnings from Zydis and Dear ImGui; both
build commands completed successfully. No game launch or runtime test was
performed. One combined runtime session is still needed to validate the new
status presentation and the integrated overlay artifact.

## Files in this batch

- `src/plugin/runtime_settings.hpp`, `src/plugin/runtime.hpp`,
  `src/plugin/runtime.cpp`
- `src/platform/win32/viewport.hpp`, `src/platform/win32/viewport.cpp`
- `src/overlay/camera_integration.hpp`, `src/overlay/camera_integration.cpp`,
  `src/overlay/renderer_runtime.cpp`
- `tests/overlay/camera_integration_harness.cpp`,
  `tests/overlay/semantic_snapshot_harness.cpp`, `test.cmd`,
  `build-overlay-settings.cmd`
- `research/reports/OVERLAY_SEMANTIC_SNAPSHOT.md`

The existing working tree contained other modified and untracked project work
before this batch. Those unrelated paths were preserved. No Git state mutation,
commit, release, or runtime action was performed.

## Compact User-Facing Status Presentation — 2026-09-22

The integrated settings overlay now presents concise colored states instead of
always displaying hook and capability internals. Gameplay HorPlus is shown as
Active only when its hook is available, no relevant transition is pending, a
Gameplay baseline is usable, and the coordinator reports Gameplay. Missing
camera data/context is Waiting or Cannot assess; unavailable hooks are shown as
Unavailable. AspectRecalculation is described as a stateful native cycle and is
not claimed to be visibly active or reversible from Enabled alone.

Cinematics and Dialogue show Ready for the next lifecycle when no active
selection is observed, and Active only when the corresponding active lifecycle
and captured policy facts are valid. Native cinematic framing and Native/Disabled
Dialogue policies receive explicit user-facing descriptions. Capability failures
and lifecycle transitions remain visible as concise warning/error details.

Tooltips now explain the behavior and application timing of each control,
including HorPlus versus AspectRecalculation, cinematic aspect/FOV policies,
and dialogue zoom policies. Camera Integration assessment and its labels remain
unchanged; no Camera Runtime panel was added. No camera, hook, lifecycle,
semantic snapshot, settings mutation, or persistence behavior changed.

```yaml
status_mapping_harness: PASS
test_cmd_registry: 39 sources / 39 compiled / 39 executed
test_cmd: PASS
production_build: PASS
overlay_settings_build: PASS
git_diff_check: PASS
runtime: NOT_PERFORMED
camera_behavior_changed: NO
```

The integrated overlay artifact was rebuilt at the repository root as
`STALKER2CameraTweaksOverlayIntegration.asi`. Runtime visual review remains
separate and was not performed in this batch.
