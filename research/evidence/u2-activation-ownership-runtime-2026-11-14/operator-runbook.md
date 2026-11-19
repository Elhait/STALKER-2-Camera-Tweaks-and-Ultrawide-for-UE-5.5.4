# U2 activation ownership — controlled runtime pass

## Candidate

Use one ordinary Production Release ASI under the standard filename:

- `build-artifacts/p0-u2-runtime-candidate-2026-11-14/STALKER2CameraTweaks.asi`
- SHA-256: `0BB27A9F536436C07F7EDFEF89061A040B775FE52FAFCC702EECC41DAFD21853`
- 3,910,144 bytes; startup timeline OFF; dialogue discovery OFF.
- Diagnostics stays controlled by `[Diagnostics] Enabled=true` in the existing INI. Do not install a second ASI.

Back up the currently installed ASI, then install this candidate as
`STALKER2CameraTweaks.asi` in the game's `Stalker2\Binaries\Win64` folder.
Keep Gameplay, Dialogue, Cinematic and aspect settings unchanged for the test.

## One primary sequence

1. Start the game and confirm the startup log identifies the candidate and
   that these capabilities are available/validated:
   `POST_EXIT_BLEND_ACTIVATION_CAPABILITY`, `U2_SHADOW_CAPABILITY`,
   `CINEMATIC_CONSUMER_CAPABILITY spans_valid=1`,
   `FOV_CAPTURE_CONTROL available=1`, and `U2_CONTROL_CAPABILITY available=1`.
   If a capability is missing, stop and send the startup log.
2. Load the save immediately before Cinematic 1. In PowerShell, first move to
   the repository root, then arm the one-shot capture:

   ```powershell
   Set-Location 'E:\Work\Slaker2 mods\01-Projects\STALKER-2-Ultrawide-Fix-for-UE-5.5.4'
   $game = Get-Process -Name Stalker2-Win64-Shipping
   & .\research\tools\ArmCinematicFovCapture.ps1 -GameProcessId $game.Id
   ```

   Confirm it says the next accepted Cinematic ENTER arms capture. The capture
   is bounded to 60 seconds / 192 combined records and can be consumed once.
3. Without loading another save between these events, perform
   `Cinematic 1 → Dialogue → Cinematic 2`, then continue through Cinematic 2
   EXIT and Gameplay recovery if it fits in the capture window. Note separately
   whether the visible FOV jumps at either requested boundary and at
   Cinematic 2 → Gameplay.
4. After the log shows recovery completed and Gameplay eligibility returned,
   arm the post-recovery control and start one ordinary Dialogue promptly:

   ```powershell
   & .\research\tools\ArmCinematicFovCapture.ps1 -GameProcessId $game.Id -Control
   ```

   This checks that the baseline remains protected through the recovery
   endpoint and that later eligible Gameplay samples resume admission. No
   save reload is needed for this control.
5. Exit the game normally and return the full
   `STALKER2CameraTweaks.log`, plus whether each of the three boundaries
   visibly jumped and whether both captures reached their expected phases.

The log will be evaluated for activation/source/thread/generation/epoch
association, the complete admitted writer sample sequence, corrected start and
destination policy, baseline dispositions, and per-invocation application.
Writer output remains distinct from rendered-view evidence. This pass does not
perform a hardware watchpoint; the final producer/def-use relation for
`camera+0x230` remains a separate P0 runtime question if the new log still needs
it.
