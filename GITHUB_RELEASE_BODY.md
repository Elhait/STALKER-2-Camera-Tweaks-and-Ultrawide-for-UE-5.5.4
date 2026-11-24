# STALKER 2 Camera Tweaks v4.0.1

- Restores Gameplay HorPlus after changing the game's FOV between dialogues, including when Gameplay correction is turned off and back on with the same camera.
- Preserves Cinematic FOV continuity through rapid re-entry after dialogue inserts when the camera evidence confirms the same blend segment.
- Improves native hook validation and backup logging. The mod checks for overlapping hook patches, rejects guarded executable memory, and keeps a valid fallback log open if an optional diagnostic message fails.

The updated camera behavior was verified in game on the installed Steam build. The canonical non-graphics suite passed 64/64 invocations. One hardlink subcase was skipped, and WARP was omitted. The exact earlier Cinematic counterexample and a change to the native recovery target during an active dialogue were verified offline.

The archive includes the ASI, installation README and licenses. Your existing INI and settings are retained when you replace the ASI. v4.0.1 is expected to resolve the reported ASI Loader error 4556. Confirmation on a system that reproduced the Windows Application Control block is pending.

See the [installation guide](README.md) and [testing notes](TESTING_AND_RESEARCH.md).
