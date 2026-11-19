# Deferred finding: rendered FOV jump on Cinematic → Dialogue

## Status

Deferred by user on 2026-11-15. Do not investigate or change camera behavior
for this symptom in the current post-helper diagnostic stage.

## Evidence and status

- User observation: a visible FOV jump occurred only on the Cinematic →
  Dialogue transition, in one transition direction. The exact rendered frames
  and magnitude are not recorded here.
- Runtime log:
  [`u2-writer-site-inventory-2026-11-15`](../evidence/u2-writer-site-inventory-2026-11-15/).
  It establishes correlated primary-writer and post-helper values for the
  tested sequence. The helper was inactive throughout.
- The log shows writer-output continuity at the Dialogue → next Cinematic
  boundary. This does not prove rendered-view continuity and does not explain
  the user's reported Cinematic → Dialogue jump.

## Impact

One reported rendered FOV discontinuity remains unresolved at Cinematic →
Dialogue. No causal link to the post-copy helper, writer mapping, or a specific
policy is established by the current capture.

## Scope boundary

The current stage remains limited to diagnostic chronology and post-helper
observation. Do not make FOV policy, baseline, recovery, or transition changes
for this finding while it is deferred.

## Revisit condition

Resume when the user explicitly reprioritizes the Cinematic → Dialogue rendered
transition for investigation. At that point, correlate the user's reported
visual jump with transaction-level records and obtain a targeted visual
runtime observation before proposing a behavior change.
