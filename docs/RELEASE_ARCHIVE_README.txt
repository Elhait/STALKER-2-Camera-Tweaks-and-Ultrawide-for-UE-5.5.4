STALKER 2 CAMERA TWEAKS v3.2.0

Gameplay, cinematic and dialogue camera/FOV adjustments for S.T.A.L.K.E.R. 2: Heart of Chornobyl (Unreal Engine 5.5.4).

Gameplay uses the game's selected FOV and supports arbitrary aspect ratios. RuntimeCombine is the recommended default: it synchronizes the Gameplay camera aspect to the current viewport, verifies the change, and only then applies Hor+ correction. Cinematics provide Auto and forced 16:9, 21:9 and 32:9 framing. Dialogue offers Adaptive, Reduced and Disabled correction modes.

Gameplay, Cinematics, Dialogue and Overlay each have an independent enable setting. Camera modules coordinate camera ownership during transitions when enabled. Disabling Overlay does not disable camera correction.

INSTALLATION

1. Close the game.
2. Install Ultimate ASI Loader as dsound.dll in Stalker2\Binaries\Win64.
3. Remove older STALKER2UltrawideFix.asi and STALKER2GameplayAspectFix.asi files so they are not loaded alongside this mod.
4. Copy STALKER2CameraTweaks.asi into Stalker2\Binaries\Win64. The mod creates STALKER2CameraTweaks.ini beside the ASI on first launch and retains recognized settings when migrating older layouts. The INI is user-owned and is not included in the release archive.
5. Start the game.

Open or close the in-game Overlay with Delete (the default toggle key). Press Esc while it is open to dismiss it. Settings are saved in STALKER2CameraTweaks.ini. Manual INI changes require a game restart.

The runtime log, STALKER2CameraTweaks.log, is written beside the game executable.

For full documentation, known limitations and support, visit:
GitHub: https://github.com/Elhait/STALKER-2-Camera-Tweaks-and-Ultrawide-for-UE-5.5.4
Nexus Mods: https://www.nexusmods.com/stalker2heartofchornobyl/mods/2416

See LICENSE.md and THIRD_PARTY_NOTICES.md for license and third-party notices.
