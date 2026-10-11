# Project Summary

S.T.A.L.K.E.R. 2 Camera Tweaks is a unified camera/FOV mod for *S.T.A.L.K.E.R. 2: Heart of Chornobyl*. Version **3.0.0 is published**; the scope and version of a future release have not yet been decided.

The distributed production plugin is `STALKER2CameraTweaks.asi`. Do not load it alongside the superseded `STALKER2UltrawideFix.asi` or `STALKER2GameplayAspectFix.asi`.

## Features

Gameplay, Cinematics, Dialogue and Overlay each have independent enable settings. Enabled camera features coordinate camera ownership during transitions; disabling one does not implicitly disable the others. The Overlay can be disabled at startup without disabling camera correction.

### Gameplay

- `RuntimeCombine` is the recommended default. It combines the earlier `AspectRecalculation` and `HorPlus` approaches: synchronizes the Gameplay camera aspect with the current viewport, confirms the read-back, and only then applies Hor+ FOV correction.
- `HorPlus` adapts Gameplay FOV to the aspect observed at the camera writer. `AspectRecalculation` uses the game's native aspect transition while preserving the selected Gameplay FOV.
- Aspect handling responds to runtime resolution changes and supports arbitrary valid ratios.

### Cinematics and Dialogue

- Cinematic framing supports automatic/native selection and forced `16:9`, `21:9` and `32:9` presets, independently of the physical display aspect.
- Cinematic FOV policies are `GameplayHorPlus` and `NativeHorPlus`; lifecycle handling coordinates ENTER, continuation, EXIT and Gameplay recovery.
- Dialogue correction modes are `Adaptive`, `Reduced` and `Disabled`. There is no `Native` Dialogue mode. Disabling Dialogue correction leaves the game's camera behavior in control; its default dialogue zoom targets 70° FOV.

### Overlay and configuration

- The settings Overlay includes localized controls, tooltips, hotkey rebinding, Camera Integration status and an expanded read-only Camera State view.
- The Overlay embeds 18 interface locales and their required font resources.
- `[Overlay] Enabled=false` disables the Overlay subsystem after restart; it does not disable Gameplay, Cinematics or Dialogue correction.
- Configuration is stored in the main INI beside the ASI. Startup-only changes require a game restart; settings documented as live-applied take effect immediately.

## Architecture

- `src/plugin/runtime.cpp` is the single runtime owner/composition root for native callbacks, hooks, workers, lifecycle and cross-domain camera coordination.
- Domain calculations and transaction helpers are separated where they form useful independent responsibilities. Runtime decomposition B1–B7 is complete with documented `Remain` decisions; further file splitting is not a goal by itself.
- Overlay code is grouped by composition, input, localization, presentation and presenter responsibilities. Shared language-state diagnostics and localization resource IDs have their own source locations.
- `build.cmd` is the production build entry point. CMake + Ninja provides the incremental build graph; `test.cmd` is the canonical test entry point and builds the registered harnesses before running them serially.
- Generated build/test outputs are isolated from release contents. Passive runtime diagnostics are included in the ordinary ASI and controlled by `[Diagnostics] Enabled`; they observe runtime state without changing camera correction semantics.

## Known boundaries

- Weapon/viewmodel FOV behavior after some camera transitions is a separate game-side path. Its reference mechanism has been researched, but production repair is deferred because the mod has no validated, patch-resilient UE reflection/invocation bridge.
- Subtitle horizontal centering is a vanilla UI issue; no production subtitle hook or heuristic offset is included.
- Offline glyph/resource checks do not guarantee every runtime glyph, Arabic shaping or right-to-left layout. Graphics-wrapper compatibility and HDR color parity are not blanket guarantees.

For accumulated testing and research conclusions, see [`TESTING_AND_RESEARCH.md`](TESTING_AND_RESEARCH.md). User-facing changes are summarized in [`RELEASE_NOTES_v3.0.0.md`](RELEASE_NOTES_v3.0.0.md); detailed technical evidence belongs in [`research/reports/`](research/reports/) and [`research/evidence/`](research/evidence/).
