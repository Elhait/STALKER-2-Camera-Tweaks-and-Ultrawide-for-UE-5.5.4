# Project Summary

This document describes the mod's current product behavior, architecture and known boundaries. Testing and research history is summarized separately in [`TESTING_AND_RESEARCH.md`](TESTING_AND_RESEARCH.md).

S.T.A.L.K.E.R. 2 Camera Tweaks is a unified camera/FOV mod for *S.T.A.L.K.E.R. 2: Heart of Chornobyl*.

The production plugin is `STALKER2CameraTweaks.asi`. Do not load it alongside the superseded `STALKER2UltrawideFix.asi` or `STALKER2GameplayAspectFix.asi`.

## Features

Gameplay, Cinematics, Dialogue and Overlay have independent enable settings. Camera features coordinate ownership during transitions; disabling one does not implicitly disable the others. The Overlay can be disabled at startup without disabling camera correction.

### Gameplay

- `RuntimeCombine` is the recommended default. It synchronizes the Gameplay camera aspect with the current viewport, confirms the read-back, then applies Hor+ FOV correction.
- `HorPlus` adapts Gameplay FOV to the aspect observed at the camera writer. `AspectRecalculation` uses the game's native aspect transition while preserving the selected Gameplay FOV.
- Aspect handling responds to runtime resolution changes and supports arbitrary valid ratios.

### Cinematics and Dialogue

- Cinematic framing supports automatic/native selection and forced `16:9`, `21:9` and `32:9` presets, independently of physical display aspect.
- Cinematic FOV policies are `GameplayHorPlus` and `NativeHorPlus`; lifecycle handling coordinates ENTER, continuation, EXIT and Gameplay recovery.
- Dialogue correction modes are `Adaptive`, `Reduced` and `Disabled`. There is no `Native` Dialogue mode. Disabling correction leaves the game's camera behavior in control; the default dialogue zoom targets 70° FOV.

### Overlay and configuration

- The settings Overlay has localized controls, tooltips, hotkey rebinding, Camera Integration status and a read-only Camera State inspector. Camera State can expand into a separately scrollable third column; its toggle is localized across the 18 embedded locales.
- The Overlay uses its own unowned top-level `OverlayHwnd` and persistent DirectComposition target. `GameHwnd` remains the host/input/language/geometry boundary. The display HWND is click-through; the existing owner thread owns ImGui, D3D11, DirectComposition and display-window teardown.
- Startup discovery is event-driven: the Overlay worker creates its message queue before publishing its thread ID, coalesces refresh requests, and can wait for a suitable visible game HWND before initializing the presenter. A live run confirmed that discovery can remain pending for tens of seconds and later initialize successfully without restarting the worker; this does not establish identical timing on every launch.
- The startup welcome toast is gated on runtime/Overlay readiness and observed game data (or the defined safe fallback); its one-shot lifetime remains draw/commit-gated at 10 seconds. User observation confirmed one visible startup run; visibility has not been established as universal across runs.
- Auto interface language resolves its reader with a unique code signature and its matching game-buffer release function with a unique interior anchor mapped through the AMD64 PE exception table. Static checks found one match for each on retained game images 2.0.2–2.0.6, excluding 1.9; the current game executable and runtime behavior on it remain unverified.
- `[Overlay] Enabled=false` disables the Overlay subsystem after restart; it does not disable Gameplay, Cinematics or Dialogue correction. Passive runtime diagnostics are part of the ordinary ASI and controlled by `[Diagnostics] Enabled` in the INI.
- The INI is a user-owned runtime file generated beside the ASI and excluded from the release ZIP. Known older layouts are rebuilt while preserving compatible values; malformed input is backed up before supported recovery. A canonical unchanged file is not rewritten. User-authored comments are not preserved; canonical documentation is regenerated.

## Architecture and workflow

- `src/plugin/runtime.cpp` is the single runtime composition root for native callbacks, hooks, workers, lifecycle and cross-domain camera coordination.
- SafetyHook validates and intercepts authored game-image sites in-process. Its patch transactions track page-protection recovery and trap generations; emitted trampoline/stub code is published RX, and continuations exposed to concurrent execution retain process-lifetime storage when required. These are selected-path safety guarantees, not proof against every OS failure or thread schedule.
- Domain calculations and transaction helpers are separated where they have useful independent responsibilities. Runtime decomposition B1–B7 is complete with documented `Remain` decisions; further file splitting is not a goal by itself.
- Overlay code is divided among composition, input, localization, presentation and presenter responsibilities. Game callbacks use nonblocking admission/command paths; the Overlay owner performs graphics teardown.
- `build.cmd` is the production build entry point. Its default CMake tree is under `build-artifacts/cm/candidate`; CMake keeps the ASI there and the script publishes an identical copy to the repository root unless `CAMERA_TWEAKS_OUTPUT_NAME` selects another path. It shows a final status and waits for a keypress before closing while preserving the build result code. `test.cmd` is the canonical sequential test entry point. Build/test outputs are isolated from release contents.
- The specifically identified SL01/SL02 CRT teardown paths have been hardened, and ordinary game exit was observed. WB01/WB02 retry-generation and continuation-lifetime findings also have targeted offline fixes and regression evidence. This is not a blanket guarantee for crashes, explicit DLL unload, third-party teardown, or graphics-driver completion.

## Known operational boundaries

- The Overlay-owned HWND architecture separates display from the game window and uses a persistent DirectComposition target. Its user-observed compatibility with NVIDIA's on-screen HUD is positive; the G-SYNC Indicator disappearing while the menu is open is accepted behavior and does not establish whether VRR itself changes.
- A startup-area presenting-thread pause has been observed. Available traces show long `UserRequest` waits but do not identify the waited object or cause; it is not established as an Overlay defect.
- Automatic startup-toast visibility has been observed in a live run, but not proven reliable across all launches. Internal draw/commit and TTL evidence does not itself prove physical display presentation.
- The identified shutdown paths involving CRT file streams and selected global-container destructors have been hardened. This is not a blanket guarantee for crashes, explicit DLL unload, third-party teardown or graphics-driver completion.

## Known boundaries

- Weapon/viewmodel FOV behavior after some camera transitions follows a separate game-side path. Its reference mechanism is researched, but production repair is deferred because no validated, patch-resilient UE reflection/invocation bridge exists.
- Subtitle horizontal centering is a vanilla UI issue; no production subtitle hook or heuristic offset is included.
- Offline glyph/resource checks do not guarantee every runtime glyph, Arabic shaping or right-to-left layout. Compatibility with every graphics wrapper, Frame Generation/VRR combination, or HDR/scRGB presentation is not guaranteed.
- Native-hook evidence covers specified executable contracts and tested paths, not every future game patch or teardown interleaving.

User-facing documentation is in [`README.md`](README.md); cumulative testing and research context is in [`TESTING_AND_RESEARCH.md`](TESTING_AND_RESEARCH.md). Detailed technical evidence remains under [`research/reports/`](research/reports/) and [`research/evidence/`](research/evidence/).
