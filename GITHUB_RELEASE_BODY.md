### STALKER 2 Camera Tweaks and Ultrawide for UE 5.5.4 — v2.0.0

Gameplay, cinematic and dialogue camera/FOV adjustments for S.T.A.L.K.E.R. 2: Heart of Chornobyl. Gameplay HorPlus uses the Gameplay FOV selected in the game's own settings and adapts it to the current runtime aspect ratio. Camera Tweaks are available on standard 16:9 displays, ultrawide screens and custom/arbitrary aspect ratios—not only 16:9, 21:9 or 32:9.

**Highlights**

- Gameplay modes: `HorPlus` and native `AspectRecalculation`. HorPlus follows the game's Gameplay FOV changes, including ADS and binocular transitions.
- Cinematic framing: `Auto`, `Native`, `16:9`, `21:9` or `32:9`, independently of physical display aspect. The presets do not limit gameplay aspect ratios.
- Cinematic FOV: `GameplayHorPlus` uses the game-selected Gameplay FOV as its baseline while preserving authored cinematic FOV changes; `NativeHorPlus` applies Hor+ to authored cinematic FOV independently of Gameplay FOV.
- Dialogue zoom: `Native`, `Adaptive`, `Reduced` or `Disabled`.
- In-game Overlay for settings and runtime status, localized selector documentation/examples, hotkey rebinding and a read-only Camera State view. It includes 18 embedded interface languages; `Auto (Game Language)` follows the game's interface language.
- Overlay settings are saved in `STALKER2CameraTweaks.ini`. The Overlay Toggle is `Delete` by default; `Esc` dismisses the open Overlay. Optional mode-cycling hotkeys are disabled by default.
- Guarded signature resolution and instruction validation fail safely when a match is ambiguous or validation is unsuccessful. Startup logs include SHA-256 identities for the loaded ASI and game executable.

**Installation**

1. Install [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader), using `dsound.dll` in `Stalker2\Binaries\Win64`.
2. Remove older `STALKER2UltrawideFix.asi` and `STALKER2GameplayAspectFix.asi` files and their old INI/log files. Do not load them alongside this release.
3. Copy `STALKER2CameraTweaks.asi` and `STALKER2CameraTweaks.ini` into the same `Win64` folder.
4. Start the game.

**Default configuration**

```ini
[Gameplay]
Enabled=true
Mode=HorPlus

[Cinematics]
AspectRatio=Auto
FovMode=GameplayHorPlus

[Dialogue]
Zoom=Adaptive

[Diagnostics]
Enabled=false

[Overlay]
Language=Auto
ToggleKey=VK_2E

[Hotkeys]
Enabled=false
GameplayCycle=F9
CinematicCycle=F10
CinematicFovCycle=F11
DialogueCycle=F12
```

The in-game Overlay is the primary configuration method. Its settings are saved to `STALKER2CameraTweaks.ini`; the generated INI documents available options and examples. Restart the game after manually editing the INI.

**Tested**

- Runtime-tested on Steam game build `2.0.6` with Unreal Engine `5.5.4`; the tested executable identity is recorded in the startup log.
- Static resolver portability checked against Steam builds `2.0.2`, `2.0.3`, `2.0.4` and `2.0.5`; these builds do not have separate runtime validation for this release.
- Gameplay tested at 16:9, 21:9 and 32:9, including startup, live aspect switching, FOV preservation, ADS/binocular transitions, and death/load camera rebuilds.
- Cinematic `Auto` aspect switching tested without restarting; `Native` and forced `16:9`, `21:9` and `32:9` framing tested, including forced 32:9 letterboxing on a 16:9 display.
- `GameplayHorPlus` and `NativeHorPlus` cinematic FOV modes were tested with live F11 switching. All four Dialogue zoom modes, Dialogue/Cinematics/ADS coexistence, save/load camera recreation and cinematic EXIT/recovery were tested in the recorded runtime scope.

**Known issues and limitations**

- Weapon/viewmodel FOV can remain incorrectly framed after certain cinematics, loads or gameplay-camera rebuilds. This separate game-side path is not modified; [Weapon Viewmodel FOV](https://www.nexusmods.com/stalker2heartofchornobyl/mods/2422) can be used alongside this mod.
- In windowed mode, a custom resolution whose aspect is not represented by a native game aspect mode may be resized when native `AspectRecalculation` returns to `Auto`. This does not mean arbitrary-aspect `HorPlus` is unsupported.
- The game's native post-cinematic FOV recovery remains game-owned and is not rewritten. Manual INI changes require a game restart.
- Future game patches may require updated signatures. Validation failures are handled safely, but signature resolution does not guarantee compatibility with future patches.

Do not install this release alongside old `STALKER2UltrawideFix.asi` or `STALKER2GameplayAspectFix.asi` files. The release archive contains the production ASI, default INI, README, license and third-party notices; diagnostic ASIs, research files, logs and historical binaries are excluded.

For full documentation, see the [repository README](https://github.com/Elhait/STALKER-2-Camera-Tweaks-and-Ultrawide-for-UE-5.5.4/blob/main/README.md).
