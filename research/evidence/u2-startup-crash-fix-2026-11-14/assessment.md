# Startup crash fix — 2026-11-14

## Finding

The two startup dumps show an access-violation write in `InstallCinematicAspect`, at the `memcpy` that saves the four original immediate bytes before creating the SafetyHook. The destination resolved into the ASI's read-only `.rsrc` mapping, so this is a bad destination address supplied by the global `g_cinematicOriginalImmediate` reference alias. The crash occurs before the cinematic aspect hook is installed and before the new camera writer diagnostic capture can begin.

## Change

`InstallCinematicAspect` now copies directly into `g_runtime.cinematicOriginalImmediate`, the backing field on the runtime state object. The problematic global reference alias was removed. No FOV policy, writer decision, or diagnostic behavior was changed.

## Artifact

The repaired Production Release ASI is isolated at `build-artifacts/u2-camera-state-diagnostic-crash-fix-2026-11-14/production/STALKER2CameraTweaks.asi`. SHA-256 and PE identity are recorded in `candidate-manifest.json`. It has not been installed into the game.

The compile and link steps completed. The process-tree supervisor reported timeout code 124 because the Visual Studio `vctip.exe` descendant remained after linking; cleanup was verified and no active build processes remained. No tests or game launch were run. Runtime startup verification is still required.
