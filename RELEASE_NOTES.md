# STALKER 2 Camera Tweaks v4.0.1

## Camera and FOV

- Gameplay HorPlus resumes when you change the game's FOV between dialogues, including after turning Gameplay correction off and back on with the same camera.
- A pending Dialogue classification can clear when the mod has independent evidence that Gameplay has resumed. Dialogue recovery can also finish at a verified new native FOV target.
- Cinematic FOV stays continuous when the game restarts a confirmed blend after a dialogue insert. The mod accepts up to 0.6° of progress toward the same target. A temporary change to projection flags is checked again at the first eligible Writer before the previous correction can be reused.

## Reliability and diagnostics

- Camera hooks are checked for overlapping patches before activation. Hook validation rejects guarded memory pages and checks that the optional Dialogue return hook is outside the main patch.
- An already opened fallback log stays usable if formatting its diagnostic message fails.
- With diagnostics enabled, the log shows whether Cinematic Writer evidence survived EXIT, whether the next activation confirmed the start, and which correction the first Writer chose.

## Validation

In-game checks confirmed smooth Cinematic, Dialogue and Gameplay transitions and same-camera Gameplay recovery after changing the FOV between dialogues. The exact earlier Cinematic FOV drop and a native target change during active Dialogue recovery passed offline regression tests.

The canonical non-graphics suite passed **64/64** invocations. One hardlink subcase was skipped. WARP was outside this run. A local Microsoft Defender scan found no threats in the final ASI and ZIP. v4.0.1 is expected to resolve the reported ASI Loader error 4556. Confirmation on a system that reproduced the Windows Application Control block is pending.

The archive includes the ASI, installation README and licenses. Replacing the ASI preserves your existing INI and settings. See the [installation guide](README.md) and [testing notes](TESTING_AND_RESEARCH.md).

# STALKER 2 Camera Tweaks v4.0

Version 4.0 brings Gameplay, Cinematic and Dialogue camera handling under one committed lifecycle authority. It also improves camera-state reporting, runtime diagnostics and automated validation while retaining the established correction modes.

## Highlights

- **Unified camera lifecycle:** Gameplay writers, Dialogue setters, Cinematic transitions, provider handoffs, recovery, settings changes and shutdown now share one lifecycle owner. Overlay, zoom and telemetry consume committed read-only state.
- **Safer FOV corrections:** Lifecycle decisions, one-shot writer permissions, native writes and read-back observations are distinct. Diagnostics being disabled does not change correction behavior, and unavailable read-back tracking does not prevent an otherwise authorized correction.
- **Gameplay receiver changes:** A new receiver can receive the applicable HorPlus correction without inheriting the previous receiver's Gameplay baseline. In `RuntimeCombine`, aspect synchronization is read back before the first eligible FOV correction.
- **Cinematic and Dialogue continuity:** Confirmed Dialogue provider handoff can apply the configured Dialogue policy after a Cinematic exits while Gameplay baseline protection remains active. Cinematic ENTER uses the selected policy and an eligible reference; prepared Dialogue output is not treated as an uncorrected native value.
- **Camera State clarity:** The read-only panel distinguishes retained Gameplay reference, directly captured Native FOV, Writer Input, Planned Output and confirmed Effective FOV. Values without matching evidence are shown as historical or unavailable rather than inferred.
- **More reliable logs:** Runtime, Startup and Overlay logs include process identity. A reusable `.PID.log` fallback handles sharing conflicts, and repeated writer/Cinematic entries are aggregated within bounded logs.
- **Expanded automated checks:** Added lifecycle transition generation, native-shaped adapter and read-back checks, diagnostics parity, startup/logging coverage and release-package contract tests.

## Existing behavior retained

Gameplay `HorPlus`, `AspectRecalculation` and `RuntimeCombine` remain available. Cinematic aspect and FOV policies, the `GameplayHorPlus` and `NativeHorPlus` modes, and Dialogue `Native`, `Adaptive`, `Reduced` and `Disabled` policies remain configurable independently. The established FOV correction formulas were not intentionally changed as part of this release.

## Game-version compatibility

The v4 resolver validates unique signature matches and relevant instruction or byte contracts, then fails closed when validation is missing, ambiguous or invalid. This prevents an unrecognized executable from being treated as compatible, but does not by itself establish support for every game version.

The v4 required camera/provider resolver set statically matched on retained Steam 2.0.2–2.0.6 images and the installed image reported as 2.1. This is not a v4 in-game launch or behavior test: the installed game-directory ASI was 3.2.2.0, and earlier runtime records also used earlier artifacts. Optional Zoom observer signatures are absent on 2.0.2–2.0.4; camera correction remains available, but Camera State cannot report those zoom observations. No 2.0.0 or 2.0.1 executable image was available for checking. See the [Supported Build Manifest](docs/SUPPORTED_BUILD_MANIFEST.md) and [v4 resolver audit](research/evidence/v4-resolver-cross-build-2026-11-19/README.md) for the exact boundaries.

## Validation and known limits

- The recorded non-graphics suite for this change set completed **64/64 invocations without failures**. One hardlink subcase was skipped after Windows returned error 5; WARP graphics checks were not run.
- **6/6 focused CTest checks passed.** Offline tests cover selected lifecycle and receiver-replacement contracts; they do not prove every possible game callback schedule.
- Earlier in-game observations on Steam build 2.0.6 covered Gameplay, Cinematic, Dialogue, recovery and Overlay behavior. The receiver-replacement fix has offline regression coverage but has not been confirmed in-game.
- Occasional visible hitches were observed during transitions. Their cause and relationship to a particular change have not been established. The available writer logs do not yet correlate all camera data to the final displayed frame.
- No claim is made that offline validation guarantees visual continuity for every cinematic or dialogue sequence.

For configuration, installation and current compatibility evidence, see [README.md](README.md). The detailed change record and test boundaries are in [CHANGES_SINCE_3.2.2_to_4.md](CHANGES_SINCE_3.2.2_to_4.md) and [TESTING_AND_RESEARCH.md](TESTING_AND_RESEARCH.md).
