# STALKER 2 Camera Tweaks v3.2.0

Gameplay, cinematic and dialogue camera/FOV adjustments for *S.T.A.L.K.E.R. 2: Heart of Chornobyl*.

## What's new

### Overlay architecture and interaction

- The Overlay now renders in its own top-level window with a persistent DirectComposition target, separate from the game window. The game window remains responsible for host discovery, game input, language access and geometry.
- The display window stays click-through. Menu input is admitted only when the Overlay is ready, and failure handling returns graphics cleanup to the Overlay owner thread without making the game callback wait for it.
- Host activation and rebinding, minimize/restore, DPI and geometry changes, negative desktop coordinates and z-order reconciliation are handled by the Overlay lifecycle.
- The settings window can expand to the right to show an independently scrollable Camera State panel. The existing columns remain in place, and the panel's localized “Camera Details / Open / Close” control is distinct from ordinary section headers. Closing the panel restores the compact width.
- Camera State is observational and read-only; unavailable values are not inferred from unrelated state.

### Startup notification

The one-time welcome toast waits for runtime initialization, Overlay readiness and an observed game-data signal. Its existing Auto locale behavior and 10-second draw/commit-gated lifetime are retained. A successful internal draw/commit is not, by itself, a guarantee of physical visibility on every system or launch.

### Configuration and installation

- The INI is a user-owned runtime file generated beside the ASI; it is not included in the release archive or deployed by Vortex as a managed file.
- Known older INI layouts are rebuilt into the current documented structure while compatible settings are preserved. Unreadable or unknown input is backed up before recovery where supported. An already-current file is left untouched when no managed update is needed.
- All 18 embedded locale catalogs include the Camera State panel controls and configuration-persistence failure message.
- Supported passive diagnostics remain in the standard ASI and are controlled by `[Diagnostics] Enabled`. No separate diagnostic ASI is required.

### Reliability and tooling

- Game-window callbacks use nonblocking admission and pass commands to the Overlay owner instead of performing persistence or graphics teardown synchronously.
- Owner-thread finalization handles terminal requests independently of ordinary message-queue capacity and reports released, retained or unverified resources distinctly.
- Partial Overlay initialization and optional Overlay failures have explicit recovery paths; camera correction availability remains separate from Overlay availability.
- Configuration and log I/O, native hook lifetime handling, build-output isolation, test-process containment and release-archive validation have been hardened.

## Camera features retained

The established Gameplay `HorPlus`, `AspectRecalculation` and `RuntimeCombine` modes; cinematic aspect/FOV modes; Dialogue zoom modes; independent module switches; and existing camera ownership/recovery policies remain the baseline. No deliberate change to the camera correction formulas or documented mode semantics was identified in the reviewed changes.

See [README.md](README.md) for current configuration and installation details.

## Diagnostics configuration

```ini
[Diagnostics]
Enabled=false
```

Diagnostics are startup-only; restart the game after changing this value. The default remains `false`.

## Installation

1. Close the game.
2. Install [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) as `dsound.dll` in `Stalker2\Binaries\Win64`.
3. Remove older `STALKER2UltrawideFix.asi` and `STALKER2GameplayAspectFix.asi` files and their old INI/log files. Do not load them alongside this mod.
4. Copy `STALKER2CameraTweaks.asi` into `Stalker2\Binaries\Win64`. The mod creates the user-owned INI beside the ASI on first launch; compatible settings from recognized older configurations are retained.
5. Start the game.

Runtime logs are written beside the game executable. The release archive contains the production ASI, README, license and third-party notices; it does not contain or deploy the user-owned INI. Research files, build outputs, logs and historical binaries are excluded.

## Validation and limitations

- The production Release build passed. The safe canonical regression run completed 62 ordered invocations, including 208 adapter executions and four Zoom fixtures. The graphics/WARP fixture was intentionally omitted; this is not a graphics-inclusive full-suite result.
- An ordinary game exit was manually confirmed for the shutdown-hardening candidate with SHA-256 `97696DBA3C40067F4F89D8C8F62D2D93C5AD2A97707860492E723A4E2016F5D4`. This does not prove every crash, process-termination, third-party unload or graphics-driver shutdown path.
- Earlier NVIDIA HUD, G-SYNC and Frame Generation observations apply only to the exact earlier candidates recorded in their evidence. They are not claimed as validation for this release artifact.
- Weapon/viewmodel FOV may remain incorrectly framed after some cinematics, loads or Gameplay camera rebuilds. This separate game-side camera path is not changed by the mod.
- In windowed mode, a custom resolution not represented by a native game aspect mode may be resized when `AspectRecalculation` returns to `Auto`. This does not mean arbitrary-aspect HorPlus or RuntimeCombine is unsupported.
- Future game patches may require updated signatures. Validation failures are handled safely, but signature resolution cannot guarantee compatibility with future patches.
