# P0 + U2 controlled runtime capture

## Candidate

Use only the ordinary Production Release candidate below, under the standard
ASI filename. It includes the existing bounded U2 shadow/capture components.
No FOV policy or transition-mapping production behavior was changed for this
capture. A Diagnostics-only log addition now prints `writerContext` alongside
the current Gameplay baseline projection sequence.

- Candidate: `build-artifacts/p0-u2-runtime-2026-11-13/STALKER2CameraTweaks.asi`
- SHA-256: `2D952B287FB9DB55A7FA789F767F5CA9E6395EF1D41F0BB6EDA5BF7C3D23295A`
- Size: 3,866,112 bytes
- File/Product version: `3.2.2.0` / `3.2.2`
- Build: Production Release, startup timeline OFF, discovery OFF
- Source HEAD: `54212ee8bb94f451d9ff5c45a2b97bf23b757c2e`; working tree was dirty

The candidate has not been installed. Back up the currently installed ASI,
then copy this file into the game's `Stalker2\Binaries\Win64` directory using
the standard filename `STALKER2CameraTweaks.asi`. Keep the backup outside the
ASI loader's file extensions. Do not install a second diagnostic-profile ASI.

In the existing `STALKER2CameraTweaks.ini`, set:

```ini
[Diagnostics]
Enabled=true
```

Keep the existing Gameplay, Cinematics, Dialogue and aspect settings unchanged
through the primary sequence. The instrumentation is read-only; Diagnostics
does not change correction policy or lifecycle decisions.

## Startup capability check

Launch the game and inspect `STALKER2CameraTweaks.log` beside the game EXE.
Before running the transition, confirm all of these markers report available
or validated:

- `POST_EXIT_BLEND_ACTIVATION_CAPABILITY`
- `U2_SHADOW_CAPABILITY`
- `CINEMATIC_CONSUMER_CAPABILITY` with `spans_valid=1`
- `FOV_CAPTURE_CONTROL available=1`
- `U2_CONTROL_CAPABILITY available=1`

If one is unavailable, stop the scenario and send the startup section of the
log. A partial observation run will not establish a missing path.

## Primary sequence: Cinematic 1 → Dialogue → Cinematic 2

1. Load the save immediately before Cinematic 1. Before entering it, run this
   from the repository root in PowerShell:

   ```powershell
   $game = Get-Process -Name Stalker2-Win64-Shipping
   & .\research\tools\ArmCinematicFovCapture.ps1 -GameProcessId $game.Id
   ```

   Confirm that the command says the next accepted Cinematic ENTER will start
   the one-shot capture. The window is 60 seconds and 192 combined records;
   it cannot be restarted in the same process after it is consumed.

2. Perform the exact sequence that shows the first jump: Cinematic 1 exits to
   the affected Dialogue, then enter Cinematic 2. Continue through Cinematic 2
   EXIT and Gameplay recovery if this fits inside the same capture window.
   Do not change settings/aspect or arm another capture mid-sequence.

3. Note whether a visible FOV jump occurs at each boundary separately:
   Cinematic 1 → Dialogue, Dialogue → Cinematic 2, and Cinematic 2 → Gameplay.
   If possible, record the Overlay FOV/aspect shown immediately before and
   after each boundary. Overlay values are telemetry, not rendered-view proof.

This capture is intended to correlate the validated provider request, blend
activation, Dialogue setter sample, writer invocation, cinematic consumer
outcomes and baseline projection. It does not prove the active view consumed a
writer value, nor does it identify every native write to `camera+0x230`.

## Gameplay baseline controls

After the log confirms Cinematic 2 recovery completed and coordinator is back
in Gameplay with the post-cinematic exclusion inactive, run:

```powershell
& .\research\tools\ArmCinematicFovCapture.ps1 -GameProcessId $game.Id -Control
```

Start a normal Dialogue promptly. Confirm `U2_CONTROL_BEGIN phase=1` and that
its coordinator/exclusion fields show a clean Gameplay control. This is the
post-cinematic comparison.

For phase 2, load a normal Gameplay save in the same process, confirm Gameplay
is active without a cinematic transition in this episode, run the same command
again, then start a separate Dialogue. Confirm `U2_CONTROL_BEGIN phase=2`.
Each control window is 20 seconds / 128 provenance records. Do not repeat the
command to extend a phase.

For P0, the important correlated lines are `FOV_OBSERVATION` and
`GAMEPLAY_BASELINE_PROJECTION`. They now include the same `input_sequence` and
`writerContext`, plus baseline disposition/current sequence. A Dialogue-bound
or ambiguous writer must not replace the baseline; a subsequent eligible
Gameplay writer should be able to update it. Also retain the adjacent
`DIALOGUE_BOUNDARY_PROVENANCE`, `POST_EXIT_PROVIDER_DIALOGUE_*` and
`CINEMATIC_BLEND_*` records.

## Return evidence

After the scenario and control phases, exit the game normally. Return the full
`STALKER2CameraTweaks.log`, not selected excerpts, and include:

- whether each of the three visible boundaries jumped;
- Overlay FOV/aspect values or screenshots if available;
- whether the one-shot capture reached its natural completion or hit its
  60-second/192-record limit;
- whether both control phases began in the intended states.

The first log review will determine whether a separate bounded hardware
watchpoint is still needed for the P0 final producer of `camera+0x230`. That
watchpoint needs the actual receiver address and exact writer interval from
this run; do not choose an address from a historical log.
