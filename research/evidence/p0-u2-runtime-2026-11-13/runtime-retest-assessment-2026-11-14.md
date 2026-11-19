# P0 endpoint fix and U2 retest — 2026-11-14

## Artifact and run identity

- Runtime `modSha256` exactly matches the P0 endpoint-fix Release candidate: `4BB15CE7EBC500B117F482CD1EF200929EEDE1C0F9C1577DAD42D1A28CA8CF0F`.
- Game image SHA-256: `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Full log: `STALKER2CameraTweaks-retest-2026-11-14.log`; 3,888 lines, 1,371,278 bytes, SHA-256 `ED04EDE69BC886963AAA4AA1DBEE2CC093F6896E63885F15BAD70B01A4311496`.
- Selected records: `focused-runtime-records-2026-11-14.txt`.

## P0 endpoint admission — runtime PASS for this path

The runtime reached the provider endpoint at `00:05:20.652`: input70.2322, target70, tolerance0.25, `writer_pair_changed=0`. The new diagnostic therefore identifies endpoint completion, not receiver retirement.

The three following matching endpoint samples were70.109,70.0334 and70.0019. All were recorded with `writerContext=TransitionAmbiguous`; each baseline projection was `RETAIN`, preserving native baseline90.3115 at baseline sequence1340. No ~70 baseline `UPDATE` appears in this run.

Gameplay admission resumed: at `00:06:13.060`, control phase1 began with coordinator Gameplay and exclusion inactive; at `00:06:15.072`, an eligible Gameplay sample updated baseline to90.3069. Phase2 began clean at `00:06:59.107`; a later eligible sample updated baseline to90.3032. This confirms the quarantine was scoped to the matching native-target endpoint and did not remain sticky.

This is integrated runtime confirmation for the exercised endpoint path. It does not prove global native/game origin for every value or the final machine instruction that writes `camera+0x230`.

## U2 transition observations — still unresolved

Both cinematic consumer records reached `InitializerObservationReached`, but each reports `native_start_commit=unverified`. The second ENTER had native input90 and forwarded target126.87. Its pre-enter writer input was70. The first CinematicActive writer stayed70 and the Cinematic HorPlus writer reported `applied=NO`.

The generation2 native blend activation observed start70 and target126.87. Its shadow records report `missing-activation-or-context`, `sample_space=unknown`, `shadow_branch=unobserved`, `shadow_mapping_verified=0`, and `rendered_start=unobserved`. Subsequent native samples eased upward from70. These establish writer-level transition inputs and the missing correction evidence; they do not establish what the rendered view showed.

The post-EXIT provider handoff for Reduced Dialogue was applied and targeted108.941, with first observed mapped output125.927 from sample125.915. Exact initializer commit, active-view consumption and visual FOV remain unproven. The supplied log contains no user's visual assessment of either boundary.

## Status

- P0 post-exit endpoint baseline contamination: fixed offline and confirmed in this runtime path.
- P0 final producer/def-use and broad native-origin proof: still open.
- U2 writer continuity and rendered/view continuity: still open; no production U2 mapping change is justified by this log alone.
- Game artifact was manually loaded by the user; matching runtime hash confirms candidate identity. No further game run was started by the assistant.
