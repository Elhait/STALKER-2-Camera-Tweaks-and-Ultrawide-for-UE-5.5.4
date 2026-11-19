# Post-EXIT blend post-setter observation

Date: 2026-11-12. Status: observer included in ordinary production Release;
focused ON/OFF checks passed; one runtime capture analyzed.

## Objective

Add one bounded, diagnostic-only observation immediately after the validated
native camera setter. Compare the existing pre-call blend sample with a
post-return readback of `camera+0x230`, while preserving explicit limits on what
that correlation proves. No production FOV/recovery policy changes are in scope.

## Completed

- Added the unique `.text` resolver and instruction validation for the native
  indirect call and its continuation; no SHA/version gate or fixed RVA.
- Added bounded per-thread invocation pairing and active activation/generation
  checks before post-call readback.
- Included the observer in the ordinary production profile. Hook installation
  and event collection remain gated by `[Diagnostics] Enabled` in the INI.
- Added adapter tests for resolver failures, pairing/mismatch/overflow,
  activation retirement, disabled diagnostics, and stale in-flight return.
- Current executable SHA-256:
  `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
  Signature was unique in `.text` at raw offset `0xCCDA73` and validated.
- Focused diagnostic ON/OFF adapter tests: PASS. ON stress: 10,000 callbacks,
  13 lines / 8,359 bytes. OFF: no observer reads, logs, or pairing.
- Corrected standard production Release rebuilt with the observer included; hash
  `39422DF9331083D420902004B739D96C4DFC748E6412AC9B5BA1A3AF5BE8E0D9`. Artifact
  details are in [the observation report](../reports/POST_EXIT_BLEND_POST_SETTER_OBSERVATION_2026-11-12.md).
- Runtime log identity matches that production hash. For generation 2 / epoch 3,
  all six matched samples have exact setter-argument/post-setter field equality;
  five writer records correlate field/input to the same samples. The log also
  records Recovery→Gameplay at `90.3088 → 127.117`, followed by the terminal
  setter sample `90.3115` and Gameplay output `127.119`.
- Caller attribution, FOV space, easing branch, intervening writes, same native
  setter/writer invocation, and rendered FOV remain unproven. Do not change
  recovery math based only on this capture.

## Validation limitation

`test.cmd -NonGraphics` passed its registry audit (59 harnesses / 62
invocations) but stopped at invocation 10/62 in `config_persistence_harness`
with `hardlink_fixture_setup_error=5` and
`staging_name_ownership_and_hardlinks=FAIL`. The cause was not determined; the
remaining invocations did not run. Do not describe the canonical suite as fully
passing.

The agent did not launch WinDbg, WARP, or graphics fixtures, and did not install
an ASI. One user-run manual runtime capture with `[Diagnostics] Enabled=true` is
complete and recorded in the report.

## Evidence boundary

A setter-return readback matching the pre-call sample would show that the field
held that value at the validated continuation. It would not prove FOV-space
provenance, easing branch, exclusive ownership, no intervening writes, exact
writer causality, or rendered FOV. No production recovery change should follow
from that equality alone.



