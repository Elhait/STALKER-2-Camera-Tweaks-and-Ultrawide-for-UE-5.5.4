# STALKER 2 Camera Tweaks v3.2.2

Gameplay, cinematic and dialogue camera/FOV adjustments for *S.T.A.L.K.E.R. 2: Heart of Chornobyl*.

## What's new

### Automatic interface-language detection

- Updated `Auto (Game Language)` to locate the game's interface-language reader through validated code signatures instead of a fixed executable-version address.
- Resolve the matching game-buffer release routine from a validated code anchor and the PE exception-function table. Ambiguous or invalid matches fail safely.
- The user confirmed automatic language detection works with the latest game patch. Static signature checks also matched the retained game images 2.0.2–2.0.6; image 1.9 was excluded from that check. These results do not guarantee compatibility with future patches.

### Reliability and safety

- Retains the v3.2.1 native-hook hardening: executable trampoline and stub code is built as read/write and then published read/execute, without an RWX fallback.
- Retains narrowed access-violation handling, page-protection recovery, delayed execute-fault tracking, and executable-region ownership safeguards.
- Retains Overlay startup discovery and refresh delivery improvements, including bounded diagnostics for game-window selection and refresh dispatch.
- Retains safer configuration and log-file handling, including bounded configuration reads and safer replacement behavior when updating the user-owned INI.
- Windows file-version metadata identifies the product and version as 3.2.2.

### Overlay and startup notification

The Overlay architecture and interaction introduced in v3.2.0 are retained. The Overlay renders in its own top-level window with a persistent DirectComposition target, separate from the game window. Its display remains click-through; menu input is admitted only when the Overlay is ready.

The settings window can expand to the right to show an independently scrollable Camera State panel. The existing columns remain in place, and the localized “Camera Details / Open / Close” control is distinct from ordinary section headers. Camera State remains observational and read-only; unavailable values are not inferred from unrelated state.

The one-time welcome toast retains its Auto locale behavior and 10-second draw/commit-gated lifetime. A successful internal draw/commit is not, by itself, a guarantee of physical visibility on every system or launch.

### Configuration and installation

- The INI is a user-owned runtime file generated beside the ASI; it is not included in the release archive or deployed by Vortex as a managed file.
- Recognized older INI layouts are migrated while compatible settings are preserved. An already-current file is left untouched when no managed update is needed.
- All 18 embedded locale catalogs include the Camera State panel controls and configuration-persistence failure message.
- Optional diagnostics remain in the standard ASI and are controlled by `[Diagnostics] Enabled`.

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

- The v3.2.2 production Release ASI was built successfully. Its embedded file version is `3.2.2.0` and product version is `3.2.2`; the packaged ASI matches the build output by SHA-256.
- Automatic interface-language detection was confirmed by the user in the latest game patch. The current v3.2.2 rebuild was not separately launched in-game by the build process.
- The latest recorded canonical non-graphics regression run for the preceding hardened baseline passed 62/62. That suite was not rerun specifically for the language-resolver update. Graphics/WARP fixtures were not run for this release build.
- Offline signature checks on retained game images 2.0.2–2.0.6 found a unique reader and release-anchor match; image 1.9 and the currently installed game executable were not part of that static check. A future game patch may require updated signatures; resolution fails safely when validation is ambiguous or unsuccessful.
- No claim is made here that antivirus classifications have been resolved.
- Weapon/viewmodel FOV may remain incorrectly framed after some cinematics, loads or Gameplay camera rebuilds. This separate game-side camera path is not changed by the mod.
- In windowed mode, a custom resolution not represented by a native game aspect mode may be resized when `AspectRecalculation` returns to `Auto`. This does not mean arbitrary-aspect HorPlus or RuntimeCombine is unsupported.
