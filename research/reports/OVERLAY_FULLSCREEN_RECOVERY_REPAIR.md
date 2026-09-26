# Overlay Fullscreen/Recovery Repair

## Scope

This batch addressed the two defects observed during the combined overlay
settings test. Overlay UI, Insert behavior, F9-F12, camera math and Dialogue
semantics were not changed.

## Resize path audit

The current path is:

```text
ResizeBuffers hook
  -> Renderer::BeforeResize
     -> WaitForGpu (queue fence)
     -> ImGui device-object invalidation
     -> overlay backbuffer release
  -> original ResizeBuffers
  -> association/lifecycle update
  -> next supported Present
     -> backbuffer/resource rebuild
     -> ImGui device-object rebuild
```

`WaitForGpu()` signals an overlay-owned fence on the presentation queue and
waits for that fence before releasing overlay resources. Static review did not
establish that this synchronization is redundant or safe to remove. Therefore
no speculative synchronization optimization was made.

The resize path now records lifecycle-only timing fields:

- `preResizeWaitMs`;
- `overlayReleaseMs`;
- `originalResizeBuffersMs`;
- `overlayRebuildMs`;
- `totalResizeHookMs`.

The telemetry is emitted only after a resize rebuild, not per frame. It will
allow the next runtime session to distinguish overlay synchronization,
original game resize, and resource rebuild cost.

## Gameplay recovery repair

The existing production flow already retained the pending
`AspectRecalculation -> HorPlus` intent. The observed gap was ordering: the
first writer callback could defer before the same callback published its newly
valid restoration aspect.

The repair retries at the existing CameraWriter boundary immediately after a
valid Gameplay observation updates `GameplayAspectRestorationStore`. It then
rereads the camera aspect/flags and uses that newly established value for the
remaining callback work.

Properties:

- no timer or polling worker;
- no stale-aspect or monitor/16:9 fallback;
- invalid aspect cannot trigger retry;
- pending intent remains until successful application;
- reverse/latest mode selection clears or replaces obsolete pending intent via
  the existing `SelectGameplayMode` path;
- Dialogue and camera mathematics are unchanged.

## Validation

```yaml
gameplay_recovery_repair: IMPLEMENTED
resize_timing: IMPLEMENTED
resize_synchronization_optimization: NOT_PERFORMED_CAUSE_UNPROVEN
deterministic_tests: PASS (36/36)
production_build: PASS
diagnostic_build: PASS
combined_overlay_settings_build: PASS
git_diff_check: PASS (line-ending warnings only)
runtime: NOT_PERFORMED
combined_runtime_ready: YES
```

The next runtime must collect the new resize timing entries and verify
fullscreen/windowed transitions, repeated resolution changes, Gameplay mode
recovery, one cinematic lifecycle, one Dialogue lifecycle and normal overlay
open/close behavior in a single combined session.
