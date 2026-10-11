# STALKER 2 Camera Tweaks v3.1.0 — Nexus changelog (draft)

## What's new

- **Passive diagnostics are included in the standard ASI.** Set `[Diagnostics] Enabled=true` in the INI and restart the game to enable bounded runtime observations and logs. Diagnostics do not change camera correction behavior; when disabled, diagnostic-only work is skipped. No separate diagnostic ASI is needed for these supported diagnostics.
- **More robust native hooks and startup failure handling.** Zoom callback exceptions are contained, hook executable storage is retained when patched code may still reference it, and fatal startup failures are handled without presenting the camera correction system as ready.
- **Improved cinematic diagnostic efficiency.** Diagnostic-only traces and observations are gated, so they do not perform their extra work when Diagnostics is disabled.
- **Faster, incremental development builds and safer tests.** The existing `build.cmd` now uses CMake + Ninja, while `test.cmd` remains the canonical test entry point and correctly reports crash exits as failures.

Version 3.1.0 retains the user-facing camera features introduced in v3.0.0, including RuntimeCombine, independent module settings, cinematic framing and Dialogue zoom modes.

## Compatibility and notes

- The current 3.1.0 candidate has passed automated tests, production/diagnostic builds and localization/resource audits. The exact final release artifact still needs its own in-game validation.
- Disabling the Overlay may help isolate compatibility issues with third-party graphics components, but it does not guarantee that every Frame Generation, VRR/G-Sync or overlay issue will be resolved.
- The game's weapon/viewmodel FOV can still appear incorrectly framed after some cinematics, loads or camera rebuilds. This is a separate game-side behavior and is not changed by this mod.
- Remove older `STALKER2UltrawideFix.asi` and `STALKER2GameplayAspectFix.asi` files before installing. Do not load them alongside `STALKER2CameraTweaks.asi`.
