# P0/U2 Runtime Capture Assessment — 2026-11-15

## Capture identity and limits

- User-reported sequence: run a cinematic with the game's FOV setting at 90;
  reload the save, change the setting to 100, and run the next cinematic.
- Runtime identity in the log: ASI SHA-256
  `6675F735171860745B7492F22F94DF603BDB3A5B0E43C2CAB00D82A74FE69D5C`;
  game SHA-256
  `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Diagnostics were enabled. The source log is 16,776,371 bytes and ends with
  `LOG_LIMIT_REACHED`; records after that marker and a final session summary
  are unavailable. This assessment makes no claim about rendered FOV because
  the user did not report a visual observation with this capture.

## Findings

1. In the first loaded-save cinematic, generation 1 selected a usable Gameplay
   baseline: native `90.3115°`, transformed `127.119°`, authored ENTER `90°`.
   This is consistent with `GameplayHorPlus` using the live qualified
   Gameplay reference.
2. Before the save reload, post-cinematic exclusion was active, but writer
   samples were still marked `GameplayEligible`. Baseline projections updated
   from approximately `90.30°` down to `70°`; generation 2 then selected
   `70° / 108.941°` as its Gameplay baseline. The Dialogue callback guard
   alone had not prevented writer-side admission.
3. After reload, the receiver identity epoch changed. The prior Gameplay
   baseline remained stored but was unusable for the new receiver. During the
   changed-setting pass, native `camera+0x230` reached approximately
   `100.311°`, while `camera+0x234` stayed at `90°` on the observed receiver.
   Therefore `+0x234` did not track the changed setting on this captured path;
   the evidence does not establish its meaning elsewhere.
4. Cinematic ENTER generations 3 and 4 used `GameplayHorPlus` with no usable
   baseline, `reference_origin=AuthoredEnter`, authored FOV `90°`, and a
   transformed target. This is the existing fallback, not proof that the
   Gameplay baseline for the reloaded receiver was acquired.
5. Diagnostics reported complete correlation for the logged writer records
   before truncation, but truncation prevents treating its periodic counters
   as a final session summary.

## Disposition

- Do not integrate `+0x234` into production policy from this capture.
- Repair baseline admission so a suppressed post-cinematic trajectory cannot
  replace a valid Gameplay baseline while the Gameplay FOV transform remains
  active.
- Prevent an empty carried cinematic generation from authorizing a stale
  Gameplay baseline for a fresh ENTER.
- Both repairs are now covered by offline harnesses. Runtime confirmation on
  the new ordinary production candidate remains pending.
