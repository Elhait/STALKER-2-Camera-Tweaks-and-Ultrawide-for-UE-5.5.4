# Dialogue between Cinematics: provider handoff after recovery

## Runtime evidence

Preserved user-provided log: [`STALKER2CameraTweaks.log`](STALKER2CameraTweaks.log), SHA-256 `D8D0AA28C3414E987DCC2EF4E0E2677646723ADB5E9E8AA1E1A6B01FB607302A`.

The log reports Dialogue enabled with `Reduced`, Gameplay `RuntimeCombine`, and Cinematic `GameplayHorPlus`. Cinematic generation 1 exits at 13:55:36.620. Its native HorPlus recovery completes at 13:55:36.631 while the post-cinematic exclusion remains active. The provider armer then captures generation 1, the same receiver, thread, start `130.311°`, target `70°`, and the exact provider setter sample is matched at 13:55:36.738.

After that match, the writer records the provider trajectory under `coordinator=Gameplay` and `writerContext=ProviderTransitionObserved`. The expected provider handoff marker is absent. At the native `70°` endpoint, the writer reports `decisionReason=GAMEPLAY_FOV_CHANGED` and `plannedAfter=actualAfter=108.941°`, the ordinary Gameplay HorPlus result. Thus the captured Dialogue `Reduced` policy did not own this trajectory. This is a writer-output diagnosis; the log alone does not establish rendered-view FOV.

## Root cause

`TryMapPostExitProviderDialogueSample` accepted provider samples only while the coordinator was `CinematicExiting` and the recovery owner still had live recovery authority. In the reproduced sequence, the exact provider activation arrived after Cinematic recovery had completed: the coordinator was already `Gameplay`, while the matching completed owner and baseline-protection exclusion were intentionally retained. Consequently, the validated provider trajectory bypassed the specialized Dialogue mapping and fell through to normal Gameplay HorPlus.

This does not require loosening Gameplay baseline admission or accepting arbitrary Dialogue callbacks. The existing evidence binds the trajectory to its activation, generation, receiver, thread, settings epoch, native start/target and monotonic writer samples.

## Change and regression

The provider mapper now accepts the second, bounded state only when all of these hold: coordinator `Gameplay`, the same recovery owner is valid and marked complete, the writer context is `ProviderTransitionObserved`, and the existing exact activation/evidence, source, thread, settings, baseline and exclusion checks pass. The shared writer invokes that mapping before ordinary Gameplay policy dispatch. At the matched Dialogue endpoint it retires the completed recovery owner and exclusion.

The camera-writer transaction harness now replays both an intermediate post-recovery provider sample and its endpoint. Before the production change, both new assertions failed; after the change, the harness passes. Existing FOV formulas and mode selections were not changed.

## Validation and limits

- Focused `camera_writer_transaction_harness`: PASS, process exit 0; supervisor verified cleanup.
- Ordinary production ASI target compiled and linked at `build-artifacts/cm/candidate/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi` (3,973,632 bytes; SHA-256 `2B24953544B5B217D4AE2B9B1BF676EF8FEE5645146D80062B62EBCFCD6B8BDA`). The initial supervisor invocation reported its compiler helper `vctip.exe` outliving the command and returned timeout status after Ninja completed the link; it cleaned the process tree. The incremental production target then completed successfully, and a process check found no `cmake`, `ninja`, `cl`, or `vctip` process remaining.
- Full `test.cmd` and in-game confirmation have not been completed for this change. The ASI was built locally but not installed or run in the game.
- A later generation-2 provider attempt in the capture has no valid recovery owner and is outside this completed-owner handoff repair. It remains fail-closed and needs separate evidence if it reproduces the same user-visible issue.

## Status

The reproduced generation-1 handoff defect is fixed and deterministically validated offline. Runtime confirmation with the ordinary ASI remains pending; the issue is not claimed fully closed until that capture confirms the Dialogue destination is applied.
