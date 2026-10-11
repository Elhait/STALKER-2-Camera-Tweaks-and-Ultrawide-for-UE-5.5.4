# STALKER 2 Camera Tweaks v3.1.0

Gameplay, cinematic and dialogue camera/FOV adjustments for *S.T.A.L.K.E.R. 2: Heart of Chornobyl*.

## What's new

### Passive diagnostics in the standard ASI

The ordinary `STALKER2CameraTweaks.asi` now includes the supported passive runtime diagnostics. Enable `[Diagnostics] Enabled=true` in the INI and restart the game to collect bounded observations and diagnostic logs. Diagnostics are observational: they do not change camera correction, ownership or lifecycle decisions. With Diagnostics disabled, diagnostic-only samples, counters, reads, predictions, locks and logging are skipped.

No separate diagnostic ASI is needed for these supported diagnostics. Experimental and forensic tools remain separate.

### Reliability improvements

- Zoom callbacks now contain exceptions while preserving their native continuation and register context.
- Hook patch state and executable-code lifetime handling have been hardened to avoid releasing hook storage when patched code may still reference it.
- Startup initialization now contains fatal exceptions and guards its cleanup attempt, keeping readiness and correction gates closed on fatal failure.
- Cinematic continuation and other passive observations are gated so disabling Diagnostics avoids diagnostic-only work.

These changes do not introduce new camera modes or alter the documented Gameplay, Cinematic or Dialogue correction policies.

### Build and test infrastructure

The project now uses an explicit production source registry and an incremental CMake + Ninja build behind the existing `build.cmd` entry point. The canonical `test.cmd` runner retains the harness inventory and sequential execution while correctly treating every nonzero process exit—including crash exit codes—as a failure.

## Existing v3.0 features

Version 3.1.0 retains the v3.0.0 user-facing features, including `RuntimeCombine` as the recommended Gameplay default, independently configurable Gameplay/Cinematics/Dialogue/Overlay modules, cinematic framing and FOV modes, Dialogue zoom modes, and the localized Overlay. See [README.md](README.md) for configuration and installation details.

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
4. Copy `STALKER2CameraTweaks.asi` and `STALKER2CameraTweaks.ini` into the same `Win64` folder.
5. Start the game.

Runtime logs are written beside the game executable. The release archive contains the production ASI, default INI, README, license and third-party notices. Research files, build outputs, logs and historical binaries are excluded.

## Validation and limitations

- The current source changes passed the canonical automated test suite, production and diagnostic builds, and localization/resource audits. These checks do not substitute for an in-game test of the exact release artifact.
- The latest in-game checks available in this workspace apply to an earlier build, not the current 3.1.0 candidate. In-game validation of the exact final 3.1.0 package remains outstanding.
- This validation does not guarantee compatibility with every graphics driver, Frame Generation setup or third-party graphics wrapper.
- Weapon/viewmodel FOV may remain incorrectly framed after some cinematics, loads or Gameplay camera rebuilds. This separate game-side camera path is not changed by the mod.
- In windowed mode, a custom resolution not represented by a native game aspect mode may be resized when `AspectRecalculation` returns to `Auto`. This does not mean arbitrary-aspect HorPlus or RuntimeCombine is unsupported.
- Future game patches may require updated signatures. Validation failures are handled safely, but signature resolution does not guarantee compatibility with future patches.
