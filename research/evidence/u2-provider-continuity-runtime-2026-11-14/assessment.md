# U2 provider continuity runtime review — 2026-11-14

## Evidence identity

- User supplied runtime log SHA-256: `E3DEF917AA08846839A1830898A11A738816BA9694169C4638880BAA1A9B4B49` (471,716 bytes).
- Runtime identity: ASI `A439FC24FFD46907A91F5056A0B9AF0BFEA8D629CAADA5F867119E24CDE74BEE`; game image `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Settings: Gameplay RuntimeCombine, Cinematics Auto/NativeHorPlus, Dialogue Reduced, Diagnostics enabled.
- Archived full log: `STALKER2CameraTweaks.log` in this directory.

## Confirmed writer observations

1. Generation 1 ENTER used native start `90.3115`, target `126.87`, corrected start `127.119`; its admitted writer sequence reached `126.87` with `production_mutation=1`.
2. At the post-EXIT provider handoff, the exact matched sample at `126.004` mapped from corrected continuity start `126.876` to output `126.866`. This first sample is continuous with the prior corrected cinematic endpoint at writer level.
3. That exact setter/writer pair is one-shot evidence. Immediately afterward, raw native samples descend `125.836 → … → 70.1896`. `GAMEPLAY_FOV_SOURCE_TRACE` marks subsequent post-cinematic samples `applied=false reason=POST_CINEMATIC_RECOVERY_HOLD`; the `CAMERA_FOV_STATE.output` values are projections and do not establish a mutation.
4. Dialogue boundary observations during this interval repeatedly report `boundary_action=ExclusionReset`, `result_early_return=1`, `result_mutated=0`, and return the callback's incoming value. Thus the current log confirms that the handoff correction does not own the complete provider trajectory.
5. The provider endpoint completes at native `70.1896`; the Gameplay baseline remains `90.3115 / 127.119` and is retained. Later Gameplay recovery is again eligible, so this run does not show a new baseline contamination.
6. Generation 2 ENTER starts at native `70` toward `126.87`, but has no current corrected start. Its samples are explicitly `missing-corrected-start` and `production_mutation=0`; the saved Gameplay baseline is not used as a substitute.

## User-observed presentation and evidence limit

The user reports visible FOV widening and then narrowing at mode transitions, and another cutscene after loading a save also began with a jump. The writer evidence above is consistent with an uncorrected provider trajectory on EXIT and a fail-closed uncorrected ENTER2 trajectory. It does not identify the active rendered view or prove which callback's value the renderer consumed at the visible frame. No rendered-view continuity PASS is claimed.

## Disposition

- P0 baseline admission remains protected in this run; no ~70-degree baseline update was observed.
- The previous provider-start fix solves only the first paired sample. The whole native provider trajectory is still not mapped by this candidate.
- C2 remains fail-closed because a current corrected start was not established.
- Do not add a cooldown or borrow a historical Gameplay baseline. Before the next production mapping, establish the policy-qualified Dialogue destination and the current corrected start for the active receiver, then bind all intermediate writer samples to the exact activation and cancel on source/thread/generation/settings/cut/rearm/segment mismatch.
- Runtime visual discontinuities remain open; the user has already completed this run, so no repeated Dialogue or save reload is requested by this record.

No production source was changed during this log review. No tests or builds were run.