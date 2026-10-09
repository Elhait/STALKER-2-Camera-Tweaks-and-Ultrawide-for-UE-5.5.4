STALKER 2 Ultrawide and Camera Tweaks for UE 5.5.4
Version 1.0.0

[b]ABOUT[/b]

A unified ultrawide, cinematic and camera/FOV fix for S.T.A.L.K.E.R. 2: Heart of Chornobyl on Steam.

The mod corrects gameplay camera behavior on 16:9, 21:9 and 32:9 displays, fixes cinematic aspect ratio and Hor+ FOV, adds custom cinematic framing, and provides FOV-aware dialogue zoom controls. Version 1.0.0 consolidates the gameplay and cinematic camera-state handling, adds GameplayHorPlus cinematic framing, and preserves the atomic post-cinematic recovery handoff. The current combined runtime validation covers Steam 2.0.6.

[b]COMPATIBILITY WITH CAMERA / FOV MODS[/b]

Compatibility with other mods that modify the game's camera, FOV, dialogue zoom, cinematic framing, aspect-ratio camera behavior, or related camera state is not guaranteed.

Bug reports must be reproduced with STALKER2CameraTweaks as the only installed camera/FOV-related mod. Other unrelated mods are fine unless they modify the same camera systems.

[b]FEATURES[/b]

[list]
[*]Fixes gameplay aspect transitions on 16:9, 21:9 and 32:9 with an atomic camera-state apply.
[*]Applies a single atomic gameplay handoff at the start of post-cinematic native FOV recovery.
[*]Automatically re-arms after gameplay camera rebuilds, including death and load.
[*]Preserves the selected gameplay FOV.
[*]Provides gameplay correction modes: AspectRecalculation and HorPlus, which preserves the game's original gameplay FOV changes and adapts them in real time to the current aspect ratio.
[*]HorPlus supports arbitrary valid runtime aspects, including ADS and binocular FOV trajectories.
[*]Corrects cinematic aspect ratio using the game's runtime camera state.
[*]Provides default GameplayHorPlus cinematic FOV that follows the selected Gameplay FOV, with NativeHorPlus as an independent alternative.
[*]Supports custom cinematic framing: Auto, Native, 16:9, 21:9 and 32:9.
[*]Allows 16:9 users to preview wider cinematic framing with letterbox bars.
[*]Adds three FOV-aware dialogue zoom correction modes: Adaptive, Reduced and Disabled. Dialogue correction can also be turned off to leave the game's camera behavior unchanged.
[*]Provides optional read-only Diagnostics telemetry for research sessions; it is disabled by default and is not recommended for normal gameplay.
[*]Provides optional F9/F10/F11/F12 runtime hotkeys: F9 gameplay, F10 cinematic aspect, F11 cinematic FOV and F12 dialogue.
[*]Creates and synchronizes the INI configuration automatically.
[*]Uses guarded signature resolution and fails safely when validation does not pass.
[/list]

[b]CUSTOM CINEMATIC FRAMING[/b]

The cinematic framing can be selected independently from the physical display:

[list]
[*]Auto - follows the game's current runtime camera aspect.
[*]Native - leaves the game's original cinematic aspect and FOV behavior untouched.
[*]16:9 - forces 16:9 cinematic framing.
[*]21:9 - forces 21:9 cinematic framing.
[*]32:9 - forces 32:9 cinematic framing.
[/list]

For example, forcing 32:9 on a 16:9 display produces a wider cinematic presentation with black bars above and below. The cinematic FOV is handled automatically for the selected framing policy.

[b]CINEMATIC FOV MODES[/b]

The cinematic FOV mode is independent from the cinematic aspect policy:

[list]
[*]GameplayHorPlus - default with Gameplay.Mode=HorPlus; follows changes to the game's Gameplay FOV while preserving authored cinematic variation. If the required gameplay context is unavailable, it falls back to NativeHorPlus.
[*]NativeHorPlus - applies Hor+ directly to the authored/native cinematic FOV and does not follow the player's Gameplay FOV setting.
[/list]

Use `Cinematics.FovMode=GameplayHorPlus` or `Cinematics.FovMode=NativeHorPlus`. F11 cycles these modes for the next cinematic when hotkeys are enabled; an active cinematic is not rebuilt retroactively.

[b]DIALOGUE ZOOM[/b]

Version 1.0.0 updates the unified ASI for Steam 2.0.6 while retaining dialogue zoom that adapts to the actual gameplay FOV:

[list]
[*]Adaptive - preserves the game's original optical zoom strength relative to the current gameplay FOV.
[*]Adaptive - default; preserves the game's original optical zoom strength relative to the current gameplay FOV.
[*]Reduced - applies half of the Adaptive optical zoom strength.
[*]Disabled - keeps the gameplay FOV during dialogue.
[/list]

The native dialogue transition timing is preserved, including smooth recovery when leaving dialogue.

[b]CONFIGURATION[/b]

The mod automatically creates STALKER2CameraTweaks.ini beside the game executable.

Default configuration:

[code]
[Gameplay]
; Enables the gameplay aspect-ratio correction.
;
; true  - enable the selected gameplay correction mode.
; false - leave the game's original gameplay camera/aspect behavior untouched.
Enabled=true

; Gameplay correction mode: HorPlus or AspectRecalculation.
;
; HorPlus - default and recommended mode. Preserves the game's native Gameplay FOV changes and adapts them in real time to the current runtime aspect ratio. Supports arbitrary/custom aspect ratios and native FOV changes such as ADS and binocular zoom.
;
; AspectRecalculation - alternative mode that uses the game's native aspect/projection transition to correct Gameplay framing while preserving the selected Gameplay FOV.
;
; On custom windowed aspect ratios that are not represented by a native game aspect mode, AspectRecalculation may return the window to the display's native aspect/size when the game restores Auto. This limitation does not apply to HorPlus.
;
; HorPlus examples when idle:
; Gameplay FOV 90°:  16:9 -> 90°; 21:9 -> approximately 106.69°; 32:9 -> approximately 126.87°.
; Gameplay FOV 100°: 16:9 -> 100°; 21:9 -> approximately 116.04°; 32:9 -> approximately 134.48°.
; Gameplay FOV 110°: 16:9 -> 110°; 21:9 -> approximately 124.95°; 32:9 -> approximately 141.41°.
Mode=HorPlus


[Cinematics]
; Controls how cinematics are framed independently from your physical display.
;
; Auto   - use the current runtime viewport aspect ratio. Supports arbitrary valid aspect ratios and is recommended for most users.
;
; Native - leave the game's original cinematic aspect and FOV behavior untouched without cinematic FOV/aspect correction from the mod. On ultrawide displays, Native may look similar or identical to 16:9 because this is how the game currently presents its cinematics without intervention from the mod.
;
; 16:9   - force 16:9 cinematic framing.
;
; 21:9   - force 21:9 cinematic framing, regardless of the physical display.
;
; 32:9   - force 32:9 cinematic framing, regardless of the physical display.
;
; Forced modes can also be used on displays with a different aspect ratio. For example, 32:9 on a 16:9 display produces a wider cinematic presentation with black bars above and below.
; When using Gameplay HorPlus, forcing a cinematic aspect that differs from the actual Gameplay aspect intentionally introduces an aspect/FOV transition when entering or leaving Cinematics.
; For seamless HorPlus transitions, use Auto or force the same aspect ratio that is actually used during Gameplay.
AspectRatio=Auto

; Cinematic FOV mode: GameplayHorPlus or NativeHorPlus.
;
; GameplayHorPlus - default and recommended FOV mode for Gameplay HorPlus. Uses the current native Gameplay FOV as the cinematic baseline, so changing the FOV setting in the game also changes the resulting cinematic FOV while preserving the cinematic's authored FOV variation.
;                  Examples for authored cinematic FOV 90° with Gameplay FOV 90°: 16:9 -> 90°; 21:9 -> approximately 106.69°; 32:9 -> approximately 126.87°.
;                  With Gameplay FOV 112.6°, the same cinematic becomes approximately 112.6° at 16:9, 118.51° at 21:9 and 143.13° at 32:9.
;                  Falls back safely when the required Gameplay context is unavailable.
;
; NativeHorPlus   - alternative mode that applies Hor+ to the native/authored cinematic FOV independently of the player's Gameplay FOV setting.
;                  For authored cinematic FOV 90°: 16:9 -> 90°; 21:9 -> approximately 106.69°; 32:9 -> approximately 126.87° regardless of Gameplay FOV.
FovMode=GameplayHorPlus


[Dialogue]
; Controls the camera zoom applied during dialogue.
;
; Adaptive - preserve the game's original optical zoom strength relative
;            to the current gameplay FOV.
;            Example: 90° gameplay FOV -> 70° during dialogue.
;            Example: 110° gameplay FOV -> approximately 90° during dialogue.
;
; Reduced  - apply half of the Adaptive optical zoom strength.
;            Example: 90° gameplay FOV -> approximately 80° during dialogue.
;            Example: 110° gameplay FOV -> approximately 100° during dialogue.
;
; Disabled - disable additional dialogue zoom and preserve normal gameplay framing.
;            Example: 90° gameplay FOV -> 90° during dialogue.
;            Example: 110° gameplay FOV -> 110° during dialogue.
Zoom=Adaptive


[Diagnostics]
; Enables optional read-only runtime telemetry. It is not recommended for normal use of the mod; enable it only for research sessions and additional runtime logging.
; false - keep diagnostic telemetry disabled.
; true  - enable CameraState, ZOOM and HorPlus FOV telemetry.
Enabled=false


[Hotkeys]
; Optional runtime controls for changing and comparing modes without restarting
; the game. Changes made through hotkeys are saved to this configuration.
;
; Hotkeys are intended primarily for testing, comparison and configuration;
; they can remain disabled during normal gameplay.
; true  - enable all runtime hotkeys listed below.
; false - disable all runtime hotkeys.
Enabled=false

; Cycle the gameplay correction mode immediately: AspectRecalculation -> HorPlus -> AspectRecalculation.
; Supported keys: F1-F12, 0-9 and A-Z.
GameplayCycle=F9

; Cycle the cinematic aspect mode for the next cinematic: Auto -> Native -> 16:9 -> 21:9 -> 32:9 -> Auto. Does not affect a cinematic that is already playing.
; Supported keys: F1-F12, 0-9 and A-Z.
CinematicCycle=F10

; Cycle the cinematic FOV mode for the next cinematic: GameplayHorPlus -> NativeHorPlus -> GameplayHorPlus. Does not affect a cinematic that is already playing.
; Supported keys: F1-F12, 0-9 and A-Z.
CinematicFovCycle=F11

; Cycle the dialogue zoom correction mode for the next dialogue: Adaptive -> Reduced -> Disabled -> Adaptive. Does not affect a dialogue that is already in progress.
; Supported keys: F1-F12, 0-9 and A-Z.
DialogueCycle=F12
[/code]

Manual INI changes require a game restart. Optional runtime hotkeys are intended mainly for testing and comparing settings; they are disabled by default. F9 switches the gameplay correction mode immediately, F10 selects the aspect policy for the next cinematic, F11 selects its FOV mode, and F12 selects the zoom mode for the next dialogue. An active cinematic or dialogue is not rebuilt after a hotkey press.

Supported hotkey bindings: F1-F12, 0-9 and A-Z.

[b]REQUIREMENTS[/b]

[list]
[*]Steam version of S.T.A.L.K.E.R. 2: Heart of Chornobyl.
[*][url=https://github.com/ThirteenAG/Ultimate-ASI-Loader]Ultimate ASI Loader (x64)[/url], with dsound.dll placed in Stalker2\Binaries\Win64.
[*]Production implementation runtime-tested on Steam 2.0.6.
[*]Static resolver validation: Steam 2.0.2, 2.0.3, 2.0.4 and 2.0.5.
[*]Unreal Engine target: 5.5.4.
[/list]

[b]INSTALLATION[/b]

[list=1]
[*]Install Ultimate ASI Loader and place dsound.dll in Stalker2\Binaries\Win64.
[*]Remove old STALKER2UltrawideFix.asi, STALKER2GameplayAspectFix.asi and their old INI/log files. Do not load them together with the new ASI.
[*]Extract STALKER2CameraTweaks.asi and STALKER2CameraTweaks.ini into the same Win64 folder.
[*]Launch the game.
[/list]

The plugin creates STALKER2CameraTweaks.log in the game folder. Include this log when reporting a compatibility problem; it records the mod and game SHA-256 values and signature-resolution status.

[b]TESTED[/b]

[list]
[*]16:9, 21:9 and 32:9 gameplay, including startup, aspect switching and death/load camera rebuilds.
[*]Auto, Native, forced 16:9, 21:9 and 32:9 cinematic framing.
[*]Runtime aspect changes after changing resolution during the same session in Auto mode.
[*]Dialogue zoom correction modes Adaptive, Reduced and Disabled on Steam 2.0.6.
[*]Sequential dialogue cycles and cinematic-to-dialogue state isolation.
[*]NativeHorPlus and GameplayHorPlus cinematic FOV modes, including F11 switching.
[*]F9 live switching between AspectRecalculation and HorPlus, including ADS/binocular transitions.
[*]Save/load camera recreation, cinematic EXIT/recovery and dialogue coexistence.
[*]Configurable runtime hotkeys and INI template synchronization behavior.
[*]Resolver portability statically validated across Steam 2.0.2–2.0.5. Runtime validation of the current production implementation is limited to 2.0.6.
[/list]

[b]KNOWN ISSUES[/b]

[list]
[*]Weapon/viewmodel FOV can be incorrect after a cinematic, load or gameplay-camera rebuild. Aiming down sights or opening a menu refreshes the game's viewmodel state. This is a separate game-side issue and is not fixed by this release.
[*]In windowed mode, a custom resolution whose aspect is not represented by a native game aspect mode may be resized by the game's native AspectRecalculation return to Auto. This does not mean arbitrary-aspect HorPlus is unsupported.
[*]For a more consistent weapon/viewmodel correction, [url=https://www.nexusmods.com/stalker2heartofchornobyl/mods/2422]Weapon Viewmodel FOV[/url] can be used alongside this mod.
[*]Do not combine this release with the old gameplay ASI or experimental cinematic ASI files.
[/list]

[b]GAME UPDATE COMPATIBILITY[/b]

Gameplay, cinematic and dialogue code locations are resolved dynamically using guarded signatures and instruction validation rather than fixed patch-specific addresses.

Game updates that only relocate validated code may continue to work without a mod update, while structural changes may require new signatures. If validation fails or becomes ambiguous, the affected hook fails safely rather than installing against an unknown code path.

The production implementation was runtime-tested on Steam build 2.0.6. Static resolver validation across Steam 2.0.2–2.0.5 confirms resolver portability but does not establish runtime support for the older builds. The game's native post-cinematic FOV recovery remains untouched.

[b]DEFENDER NOTICE[/b]

Microsoft Defender may occasionally flag unsigned ASI files. The release binary was reviewed and classified as “Not malware.” If an old detection remains, update Microsoft Defender security intelligence and rescan the file.

[b]SUPPORT DEVELOPMENT[/b]

This mod is maintained by [url=https://github.com/Elhait/STALKER-2-Ultrawide-Fix-for-UE-5.5.4]Elhait[/url].

If the mod helped you and you would like to support future updates:

[list]
[*][url=https://ko-fi.com/elhait]Ko-fi[/url]
[*][url=https://donatello.to/Elhait]Donatello[/url]
[/list]

[b]SOURCE CODE & RESEARCH[/b]

[list]
[*][url=https://github.com/Elhait/STALKER-2-Ultrawide-Fix-for-UE-5.5.4]GitHub source and research history[/url]
[*][url=https://www.nexusmods.com/stalker2heartofchornobyl/mods/2337]WIDEBOY Fixes by BigChenga[/url] - research reference used during dialogue FOV investigation; this mod's dialogue implementation was independently reverse engineered and runtime validated.
[*]Third-party libraries and code attribution are listed in THIRD_PARTY_NOTICES.md.
[/list]
