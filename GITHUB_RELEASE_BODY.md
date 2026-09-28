### STALKER 2 Camera Tweaks and Ultrawide for UE 5.5.4 — v2.1

Gameplay, cinematic and dialogue camera/FOV adjustments for S.T.A.L.K.E.R. 2: Heart of Chornobyl. Gameplay HorPlus continues to use the Gameplay FOV selected in the game's own settings and adapts it to the current runtime aspect ratio, including ultrawide and custom aspect ratios.

## What's new in 2.1

### Independent Overlay presentation

The Overlay has been rebuilt around a private D3D11 renderer and DirectComposition surfaces. It no longer intercepts the game's DXGI factory or swapchain `Present`/`ResizeBuffers` methods, and it does not own or render through game backbuffers, devices or queues. This removes the Camera Tweaks Overlay from the game's mutable presentation-hook chain involved in recursive resize forwarding during tested Frame Generation transitions.

The Overlay discovers the game window independently and manages its own surface generations, redraws and recovery lifecycle. The existing ImGui interface, settings, localization, notifications and camera runtime are retained. The former DXGI/D3D12 Overlay backend is not included as a fallback.

### Overlay lifecycle and interaction

- Startup guidance waits for automatic game-language detection and begins its visible lifetime only after a committed frame containing the notification.
- Mouse, keyboard and focus events use a bounded game-window event bridge and a single ImGui/render owner thread. Delete toggles the Overlay; Esc dismisses it.
- Minimize/restore, resize, surface replacement and presenter recovery are handled by the independent composition lifecycle.
- Idle rendering is demand-driven. Notification animation uses the system compositor clock when available; partial damage updates reduce unnecessary redraw work while retaining full-redraw fallbacks for correctness.
- DPI handling uses physical client pixels for the composition surface and scales font/style metrics from the effective window DPI.

### Post-cinematic Gameplay FOV

Validated native Gameplay FOV interpolation after cinematic exit is mapped continuously into HorPlus output space without publishing transitional values as the native `GameplayBaseline`. Ambiguous or insufficiently validated samples remain untransformed. Existing Gameplay modes and Dialogue recovery semantics are unchanged.

## Existing features

- Gameplay modes: `HorPlus` and native `AspectRecalculation`. HorPlus follows the game's selected Gameplay FOV, including ADS and binocular transitions.
- Cinematic framing: `Auto`, `Native`, `16:9`, `21:9` or `32:9`, independently of physical display aspect. These presets do not limit gameplay aspect ratios.
- Cinematic FOV: `GameplayHorPlus` uses the game's Gameplay FOV as its baseline while preserving authored cinematic FOV changes; `NativeHorPlus` applies HorPlus to authored cinematic FOV independently of Gameplay FOV.
- Dialogue zoom: `Native`, `Adaptive`, `Reduced` or `Disabled`.
- In-game settings Overlay with runtime status, hotkey rebinding, localized selector help and a read-only Camera State view.
- 18 embedded interface languages. `Auto (Game Language)` follows the game's interface language.
- Overlay settings are stored in `STALKER2CameraTweaks.ini`. The Overlay toggle defaults to `Delete`; optional mode-cycling hotkeys are disabled by default.
- Guarded signature resolution and instruction validation fail safely when a match is ambiguous or validation is unsuccessful.

## Installation

1. Install [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader), using `dsound.dll` in `Stalker2\Binaries\Win64`.
2. Remove older `STALKER2UltrawideFix.asi` and `STALKER2GameplayAspectFix.asi` files and their old INI/log files. Do not load them alongside this mod.
3. Copy `STALKER2CameraTweaks.asi` and `STALKER2CameraTweaks.ini` into the same `Win64` folder.
4. Start the game.

The mod creates and documents the INI when it is missing. Restart the game after manually editing it. Runtime logs are written beside the game executable.

## Default configuration

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

## Validation

- The supplied production runtime logs identify mod SHA-256 `DF02F40BDEDA42666D2395F9558FA53F2996AFBAB8BFA54D1BE32995068E57A1` and game executable SHA-256 `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
- In that runtime session, the Overlay log recorded zero Camera Tweaks DXGI factory/swapchain hooks, a committed startup hint, Auto locale `uk`, DPI 120 and mouse input delivery. The camera log recorded 3:1 aspect, Gameplay FOV 140, cinematic exit recovery and subsequent ADS/zoom changes; the user reported the transition was visually smooth.
- Native Frame Generation toggling has been user-tested with the integrated DComp Overlay in the tested setup. This is not a blanket compatibility guarantee for every driver, Frame Generation mod or third-party graphics wrapper.
- The full offline test suite passed 47/47 inventoried harnesses; localization/resource/font/glyph audits passed for all 18 catalogs and 182 canonical keys. Offline results do not prove runtime rendering of every glyph or every graphics/device-recovery path.

## Limitations and evidence notes

- SDR/HDR visual parity remains under evaluation. The current Overlay uses a premultiplied-alpha BGRA8 surface and does not apply an explicit HDR/scRGB or SDR-white-level transform.
- The long post-cinematic transitional-FOV mapping has deterministic regression coverage, but the supplied exact-hash runtime session recovered nearly immediately and did not exercise that longer interpolation sequence.
- Weapon/viewmodel FOV can remain incorrectly framed after certain cinematics, loads or gameplay-camera rebuilds. This is a separate game-side path and is not modified by this release; [Weapon Viewmodel FOV](https://www.nexusmods.com/stalker2heartofchornobyl/mods/2422) can be used alongside this mod.
- In windowed mode, a custom resolution whose aspect is not represented by a native game aspect mode may be resized when native `AspectRecalculation` returns to `Auto`. This does not mean arbitrary-aspect `HorPlus` is unsupported.
- Future game patches may require updated signatures. Validation failures are handled safely, but signature resolution does not guarantee compatibility with future patches.

Do not install this release alongside old `STALKER2UltrawideFix.asi` or `STALKER2GameplayAspectFix.asi` files. The release archive contains the production ASI, default INI, README, license and third-party notices; diagnostic builds, research files, logs and historical binaries are excluded.

For full documentation, see the [repository README](https://github.com/Elhait/STALKER-2-Camera-Tweaks-and-Ultrawide-for-UE-5.5.4/blob/main/README.md).
