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
- Gameplay HorPlus can resume on the same camera after the game's FOV changes between dialogues. Independently confirmed Gameplay can clear a pending Dialogue classification and admit a fresh FOV reference.
- On the first eligible writer call after a camera receiver change, `RuntimeCombine` can synchronize aspect and apply Hor+ once read-back confirms the new aspect. The previous receiver's baseline is invalidated for the new source.

### Cinematics and Dialogue

- Cinematic framing supports automatic/native selection and forced `16:9`, `21:9` and `32:9` presets, independently of physical display aspect.
- `GameplayHorPlus` uses an eligible Gameplay reference at cinematic ENTER and preserves authored FOV variation. If that reference is unavailable, correction uses the authored ENTER FOV, as in `NativeHorPlus`.
- A verified dialogue provider can take over after cinematic EXIT while Gameplay recovery still protects the baseline. A response choice inside an active cinematic retains cinematic ownership. A following cinematic can start its blend from the last confirmed corrected Dialogue FOV.
- Dialogue recovery can end at a verified new native FOV target. Cinematic re-entry can carry the previous correction through activations without a Writer when the blend evidence still matches. Continuation allows up to 0.6° of progress from the last admitted Writer toward the same target and rechecks transient projection flags at the next eligible Writer.
- The selectable Dialogue modes are `Adaptive`, `Reduced` and `Disabled`. With Dialogue correction disabled, the game controls the camera and normally targets 70° FOV.

### Overlay and configuration

- The settings Overlay has localized controls, tooltips, hotkey rebinding, Camera Integration status and a read-only Camera State inspector. Camera State can expand into a separately scrollable third column; its toggle is localized across the 18 embedded locales.
- Camera State distinguishes the admitted Gameplay reference, original native FOV, writer input, planned output and transaction-correlated read-back. Each value carries its own availability and freshness. Neutral zoom observation is tracked separately.
- With hotkeys enabled, `CinematicFovCycle` (F11 by default) cycles the FOV policy for the next cinematic. Its binding is configured in the INI. The Overlay selector also sets the policy directly.
- The Overlay uses its own unowned top-level `OverlayHwnd` and persistent DirectComposition target. `GameHwnd` remains the host/input/language/geometry boundary. The display HWND is click-through; the existing owner thread owns ImGui, D3D11, DirectComposition and display-window teardown.
- Startup discovery is event-driven: the Overlay worker creates its message queue before publishing its thread ID, coalesces refresh requests, and can wait for a suitable visible game HWND before initializing the presenter. A live run confirmed that discovery can remain pending for tens of seconds and later initialize successfully without restarting the worker; this does not establish identical timing on every launch.
- The startup welcome toast is gated on runtime/Overlay readiness and observed game data (or the defined safe fallback); its one-shot lifetime remains draw/commit-gated at 10 seconds. User observation confirmed one visible startup run; visibility has not been established as universal across runs.
- Auto interface language resolves its reader with a unique code signature and its matching game-buffer release function with a unique interior anchor mapped through the AMD64 PE exception table. Static checks found one match for each on retained game images 2.0.2–2.0.6 and the installed image reported as 2.1; function-entry mapping and runtime language behavior on the latter remain unverified.
- The v4 required camera/provider resolver set statically matched on retained game images 2.0.2–2.0.6 and the installed image reported as 2.1. This establishes signature coverage. Optional Zoom observations are unavailable on 2.0.2–2.0.4. [Evidence and limits](docs/SUPPORTED_BUILD_MANIFEST.md#build-support-and-evidence-matrix).
- `[Overlay] Enabled=false` disables the Overlay subsystem after restart; it does not disable Gameplay, Cinematics or Dialogue correction. Passive runtime diagnostics are part of the ordinary ASI and controlled by `[Diagnostics] Enabled` in the INI.
- The INI is a user-owned runtime file generated beside the ASI and excluded from the release ZIP. Known older layouts are rebuilt while preserving compatible values; malformed input is backed up before supported recovery. A canonical unchanged file is not rewritten. User-authored comments are not preserved; canonical documentation is regenerated.

## Architecture and workflow

- `src/plugin/runtime.cpp` wires native callbacks, hooks and workers. The production Lifecycle Authority owns camera transitions, provider handoff, recovery, baseline eligibility and camera settings changes. It commits decisions under `CameraCorrectionGate`. Overlay, zoom and telemetry consume read-only projections.
- Writer preparation, native execution and read-back are separate stages. Diagnostics can observe the result, and correction remains available if observation storage is full.
- Runtime, startup and Overlay logs include the process ID. They use a reusable `.PID.log` fallback on a sharing conflict, and logging failure is handled independently of camera hook initialization. An open fallback log stays usable if its diagnostic message fails. Cinematic diagnostics trace Writer evidence across EXIT, activation and the first eligible Writer.
- Camera hook setup checks actual patch spans before enabling hooks, rejects guarded executable pages and checks that the optional Dialogue return hook is outside the primary patch.
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

- In-game checks confirmed smooth Cinematic, Dialogue and Gameplay transitions and recovery after changing FOV between dialogues on the same receiver. Receiver replacement, the exact earlier Cinematic FOV drop and native target changes during active Dialogue recovery have offline coverage. [Testing and evidence](TESTING_AND_RESEARCH.md#v401-camera-lifecycle-regression-checks)
- Weapon/viewmodel FOV behavior after some camera transitions follows a separate game-side path. Its reference mechanism is researched, but production repair is deferred because no validated, patch-resilient UE reflection/invocation bridge exists.
- Subtitle horizontal centering is a vanilla UI issue; no production subtitle hook or heuristic offset is included.
- Offline glyph/resource checks do not guarantee every runtime glyph, Arabic shaping or right-to-left layout. Compatibility with every graphics wrapper, Frame Generation/VRR combination, or HDR/scRGB presentation is not guaranteed.
- Native-hook evidence covers specified executable contracts and tested paths, not every future game patch or teardown interleaving.

User-facing documentation is in [`README.md`](README.md); cumulative testing and research context is in [`TESTING_AND_RESEARCH.md`](TESTING_AND_RESEARCH.md). Detailed technical evidence remains under [`research/reports/`](research/reports/) and [`research/evidence/`](research/evidence/).
