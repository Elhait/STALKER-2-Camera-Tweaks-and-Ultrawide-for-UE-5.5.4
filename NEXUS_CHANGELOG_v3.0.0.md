# STALKER 2 Camera Tweaks v3.0.0 — Nexus changelog (draft)

## What's new

- **RuntimeCombine is now the recommended default Gameplay mode.** It combines aspect recalculation and Hor+ correction: the Gameplay camera aspect is synchronized with the current viewport and verified before FOV correction is applied. It supports arbitrary valid aspect ratios and responds to resolution changes.
- **More reliable cinematic transitions.** Cinematic FOV correction and recovery have been hardened, including applying the currently selected aspect policy on the next cinematic entry after a settings change.
- **More control over each camera feature.** Gameplay, Cinematics and Dialogue have independent enable switches. Dialogue offers `Adaptive`, `Reduced` and `Disabled` correction modes; disabling Dialogue correction leaves the game's camera behavior unchanged.
- **The Overlay can be fully disabled.** Set `[Overlay] Enabled=false` in the INI and restart the game. This does not disable camera correction or the separate camera hotkeys.
- **Improved in-game diagnostics.** The Camera Integration panel and expanded Camera State view distinguish current, retained and historical values more clearly. The technical panel uses consistent aspect-ratio formatting.
- **Updated settings and language support.** The generated INI uses the current defaults and clearer descriptions. The Overlay includes 18 languages, with expanded tooltip and embedded-font coverage.
- **Additional camera lifecycle safeguards.** Gameplay, cinematic and dialogue observations, ownership transitions and recovery are more carefully validated to avoid applying corrections to unconfirmed camera data.

## Compatibility and notes

- Runtime-tested with S.T.A.L.K.E.R. 2 Steam build `2.0.6` and Unreal Engine `5.5.4`.
- Remove older `STALKER2UltrawideFix.asi` and `STALKER2GameplayAspectFix.asi` files before installing. Do not load them alongside `STALKER2CameraTweaks.asi`.
- The game's weapon/viewmodel FOV can still appear incorrectly framed after some cinematics, loads or camera rebuilds. This is a separate game-side behavior and is not changed by this mod.
