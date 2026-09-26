# Overlay Gameplay FOV Source Audit

Date: 2026-09-22  
Scope: read-only source/history audit for the three FOV values requested in the overlay.

## Result

The remembered value has a concrete likely source: the gameplay writer source object's `+0x234` field, historically logged as `secondaryFOV` and currently called `cameraFirstPersonFov` by diagnostics. Prior runtime notes report `secondaryFOV=90` when the in-game Gameplay FOV setting is 90, while `primaryFOV` was transient/current camera state.

This is **not currently exposed as a retained global Gameplay-FOV value** in production code or `OverlaySemanticSnapshot`. The source supports treating `source+0x234` as a strong candidate for the selected/target Gameplay FOV, but the available checked-in evidence does not establish that it stays unchanged through ADS/zoom or every camera/lifecycle transition. Do not yet label it unconditionally as `Game FOV`.

## Evidence and ownership

- `research/completed/CINEMATIC_NATIVE_TWO_PASS_ASPECT_TRANSITION_TASK_PLAN.md:15` records the prior runtime interpretation: `secondaryFOV=90` for a game setting of 90; `primaryFOV` is transient/current camera state. At line 87 the report also records a correlated `primaryFOV=90`, `secondaryFOV=90` observation after an aspect transition. This supports the candidate mapping, not ADS invariance.
- `research/probes/gameplay_aspect_fix.cpp:144-153,199` shows the earlier tracer reading `source+0x234` into `secondaryFov` and printing it alongside `primaryFOV`.
- Current `src/plugin/runtime.cpp:1480-1499` has `LogCameraModeChange` read `source+0x234` into `cameraFirstPersonFov`; it is diagnostic observation, not a retained selected-setting owner. A post-EXIT diagnostic reader also samples the same field at lines 906-918.
- The active `ApplyHorPlusGameplay` path begins at `src/plugin/runtime.cpp:3520`. It receives the live writer value in `context.xmm0`, obtains the source object from `context.rsi`, reads aspect/flags, applies the per-sample transform, and publishes the input/result observation. It does not read or retain `source+0x234` as a Gameplay setting.
- `src/camera/fov_observation.hpp:57-66` defines the coherent per-writer record: input FOV, result FOV, aspect, writer source and publication sequence.
- `src/camera/gameplay_baseline.cpp:52-65` retains `nativeFov` from `observation.inputFov` and the transformed result from that same eligible observation. It is a live camera-writer baseline, not the distinct selected-setting field.
- `src/plugin/runtime.cpp:3185` explicitly sets `configuredGameplayFovKnown = false` in diagnostic snapshot composition. `OverlaySemanticSnapshot` in `src/plugin/runtime_settings.hpp:73-95` contains no numeric selected/input/result FOV bundle today.
- `research/reports/V1_CHANGES_FROM_V0_6.md:257` labels a `global_camera_fov_layer` as added, but does not identify a standalone selected-Gameplay-FOV global. The current Global HorPlus architecture audit describes shared transform/observation/baseline ownership; it does not establish such a scalar.

## Three-value mapping

| UI concept | Existing source | Status |
|---|---|---|
| Game FOV setting | Candidate: writer source `+0x234` (`secondaryFOV` / `cameraFirstPersonFov`) | Historically correlated with the setting; not currently retained/exposed, and ADS persistence is not proven by the checked-in evidence. |
| Before HorPlus | `CameraFovObservation.inputFov` (`GameplayBaseline.nativeFov` when baseline projection is valid) | Authoritative writer input, but dynamic camera state rather than the selected setting. |
| After HorPlus | `CameraFovObservation.resultFov` (`GameplayBaseline.horPlusFov` when transformed and valid) | Authoritative transform result paired with the same writer observation. |

## Minimal read-only projection proposal

After confirming the `+0x234` field's selected-setting semantics across ordinary gameplay, a game-settings FOV change, ADS/zoom, and camera-source/lifecycle changes:

1. At the existing validated CameraWriter callback, safely sample `source+0x234`; do not add a hook or write to game state.
2. Store that sample as a separately typed, validity-tagged field on the same `CameraFovObservation`, with the same writer source and publication sequence as `inputFov`/`resultFov`.
3. Project one coherent FOV bundle through the read-only overlay snapshot. The UI must take Before/After from that one observation and render unavailable for invalid/stale samples; never reconstruct the selected value from either writer value.
4. Until the runtime contract is confirmed, either omit the first row or label the candidate explicitly as the observed `secondaryFOV` field rather than promising it is the game setting.

## Limits and next gate

- Source/history review only; no build, game launch, runtime sampling or source edit.
- The checked-in historical evidence confirms correlation at a 90 FOV setting, but does not prove the candidate is a module-owned global or remains invariant under ADS/zoom.
- Next validation, if approved, is a bounded runtime observation of `source+0x234` while changing the game FOV and entering/exiting ADS/zoom, with camera-source identity and writer sequence recorded. No behavior change is needed for that observation.
