# Panini CVar Runtime Feasibility — Task Plan

Status: Batch 1 complete — Panini confirmed active; CVar tuning exhausted;
production suitability blocked by persistent rendering artifact.

## Objective

Determine whether the shipping STALKER 2 / UE 5.5.4 runtime exposes a usable
Panini post-process pass through `r.LensDistortion.Panini.D`, and whether it
visually improves the current 32:9 full-Hor+ cinematic presentation against
the retained 16:9 authored-FOV reference.

This is a feasibility test only. It does not change the ASI or establish a
production feature.

## External technical basis

- Epic documents Panini as an optional post-processing pass performed in the
  upscaling pass when `r.LensDistortion.Panini.D > 0`.
- Epic documents possible center blur as `D` increases, and warns that some
  perspective-projected pixels near the top and bottom may not be shown.
- The UE 5.5 forum guidance identifies `r.LensDistortion.Panini.D` as the
  replacement for `r.Upscale.Panini.D`, with a practical range of `0.0–1.0`.
- These sources establish the engine-side mechanism, not its availability or
  visual quality in STALKER 2's shipping configuration.

## Approved scope

- One fixed 32:9 cinematic scene.
- Keep the current mod build and all mod settings unchanged.
- Compare `r.LensDistortion.Panini.D 0`, `0.25`, `0.5` and `1.0`.
- Keep `r.LensDistortion.Panini.S` and `r.LensDistortion.ScreenFit` unchanged
  during the first pass.
- Compare center composition, central apparent scale, side coverage, edge
  distortion, vertical crop, blur, sharpening and cinematic EXIT behavior.

## Explicit non-goals

- No source, ASI, INI or release-asset changes.
- No renderer hook, CVar automation or new configuration option.
- No claim that Panini removes the camera-FOV excursion.
- No change to the current Full Hor+ production baseline.
- No `S` or `ScreenFit` tuning until the `D` sweep shows a useful effect.

## User-run test

1. Install/use the console-enabler separately from the ASI under test.
2. Use the same 32:9 resolution and the same cinematic/reference scene.
3. Capture the current baseline with `r.LensDistortion.Panini.D 0`.
4. Repeat the scene with `D 0.25`, `D 0.5` and `D 1.0`.
5. For each value record center composition, edges, top/bottom crop, blur,
   whether the command visibly changes the image and:
   `Gameplay also affected? YES / NO`.
6. If a value is promising, check the cinematic-to-gameplay transition; if the
   effect is clearly unavailable or unusable, finish the sweep without
   repeating the full lifecycle check for every value.
7. Restore `r.LensDistortion.Panini.D 0` after the test.

## Acceptance criteria

- `D=0` matches the current baseline.
- At least one positive `D` value visibly changes the cinematic image, or the
  runtime is classified as unavailable/ineffective.
- A useful result must bring the central composition perceptually closer to
  the 16:9 reference while retaining side content and avoiding unacceptable
  center blur, vertical crop, shadow/temporal duplication or EXIT regression.
- Any positive visual result remains a candidate for later combined testing;
  it is not a production recommendation yet.

## Initial user-run evidence

- The CVar is available and visibly changes the cinematic presentation in the
  shipping runtime.
- The first positive-`D` result shows a detached/duplicate shadow tree near
  the scene geometry. This is classified as a severe shadow/projection
  artifact, not as a harmless visual trade-off.
- The exact `D` value, `S`/`ScreenFit` state and whether the artifact persists
  across the sweep remain to be recorded.
- Current result: Panini feasibility `PARTIAL`; production suitability
  `BLOCKED pending bounded artifact characterization`.

Final bounded result: `research/reports/PANINI_CVAR_RUNTIME_FEASIBILITY_RESULT.md`
records that positive `D` values visibly activate Panini but all tested
strengths retain the detached shadow/tree artifact. `Panini.S` from `0` to `1`,
the correctly named `r.LensDistortion.Panini.ScreenFit` from `0` to `10`, and
`r.Shadow.Denoiser 0` do not remove it. The CVar-only branch is exhausted.
The next step, if pursued, requires a separate read-only render-pass isolation
task; no further blind CVar tuning is authorized.

## Risks and safe failure

- The CVar may be renamed, unavailable, overridden by the game or active only
  under a different upscaling path.
- A visible effect may improve edge distortion while worsening composition or
  image quality; classify those separately.
- If the pass affects gameplay or creates a lifecycle seam, restore `D=0` and
  preserve the unchanged production baseline.

## Stop conditions and next gate

- Stop after the bounded `D` sweep.
- Do not start a new static scan from this test alone.
- If no useful effect appears, record the negative result and keep the parent
  projection-remap task blocked.
- If a useful effect appears, prepare a separate combined feasibility plan
  before testing `S`, `ScreenFit`, reduced cinematic FOV or ASI integration.

## Expected final review

- Record exact CVar values, scene/resolution and observed visual/runtime result.
- Keep user-run evidence separate from static engine documentation.
- No commit, tag, push or release change is part of this task.
