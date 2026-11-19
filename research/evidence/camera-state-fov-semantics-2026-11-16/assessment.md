# Camera State FOV semantics — PID 21296

Read-only assessment of user screenshots and the preserved .PID logs.
Artifact 2E93B844C5BE2E3F383C2EBD90F99B966F7C854661A4E8711A104EA065EA9368.
No production edits, tests, build or game-directory mutations in this stage.

## Gameplay FOV unavailable

`PublishCameraStateSnapshotLocked` publishes the dedicated Gameplay FOV only
after a neutral Gameplay sample qualifies. `neutralContext` requires
`zoomObserverAvailable && zoomObservationValid && !zoomActive` among its guards.
Runtime initializes snapshotZoomObservationValid=false when the observer is
installed, and sets it true only after a valid zoom callback read. No zoom
transition records appear in this capture; stable Gameplay records show
lastZoomSequence=0. Thus neutral reference evidence is never established.
The UI displays this dedicated evidence fact, not GameplayBaselineStore.
Both Cinematic ENTERs independently report usable GameplayNativeNumericBaseline
130.311 -> 153.931. Missing Overlay Gameplay FOV is not a missing production
baseline or a failure of the working correction.

Do not repair this by inventing a neutral zoom state or blindly marking a
historical baseline Current. Define separately an admitted Gameplay reference
and a confirmed neutral Gameplay sample, with their own source/trust/freshness.
If the UI intends to show the reference used by GameplayHorPlus, publish the
existing authoritative baseline under that explicitly defined semantic role.

## Native FOV is currently boundary-local input, not pristine game FOV

The snapshot assigns evidence.nativeFov from observation.inputFov, tagged
NativeNumericObservation. Full publication and partial FOV transactions can
update this field at different boundaries. `NativeRegisterInput` is capture
provenance, not proof that upstream game state has not been modified by the
mod. The label/tooltip does not make that upstream distinction clear enough.

| State | Observed camera/input | Mod writer output |
| --- | --- | --- |
| Loaded Gameplay | ~130.311 | ~153.931 |
| Cinematic 1 | ~130.311 | ~153.931 |
| Confirmed Reduced Dialogue endpoint | ~122.087 | ~149.071 |
| Cinematic 2 endpoint | ~153.931 | ~153.931 |

Dialogue's original provider target is 70. The active Reduced setter changes
its native argument to ~122.087, which reaches the camera field before writer
HorPlus. Cinematic 2 capture explicitly records native_start=122.087,
native_target=153.931, corrected_start=149.071 and corrected_target=153.931.
The cinematic ENTER policy has already supplied the transformed target to the
native blend. Hence the later camera field can contain 153.931 without any
copy-back from output+0x30 or second HorPlus application.

Authored cinematic ENTER remains 90; it is a different reference from the
dynamic camera field. The true unmodified current FOV cannot generally be
recovered by relabeling either +0x230 or +0x234; unmodified provider inputs
must be preserved at their actual observation boundaries when available.

## Next bounded contract

Separate admitted Gameplay reference, authored/provider input before setter
correction, current writer input after upstream preparation, planned output
and readback. Use existing Camera State and observations. Maintain Unavailable
where evidence is missing; retain provenance, validity, owner and freshness.
Do not change validated transition policies merely to make displayed numbers
match an assumed native meaning. Any implementation needs red regressions for
the no-zoom Gameplay publication and mode-dependent upstream-corrected inputs.
