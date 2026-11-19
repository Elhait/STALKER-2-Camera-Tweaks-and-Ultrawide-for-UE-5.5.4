# M1 hardening result

2026-11-17. Isolated core only. Production callbacks, R1, ASI and installed
game were not edited. M2/M3 remain NO-GO.

## Red evidence and repair

Before changing the core, the focused counterexamples failed on Candidate
termination, independent Cinematic effect blocked by invalid Gameplay baseline,
and supersession not revoking an earlier effect. See
`../evidence/lifecycle-authority-hardening-2026-11-17/red.json`.
The fourth wrong-generation negative initially passed because the baseline gate
rejected all effects; after repair it exercises the generation gate itself.

Candidate and Active Dialogue now have source-bound explicit termination. It
clears arming and setter/writer permissions, without creating Gameplay evidence.
Foreign receivers cannot terminate the claim; Gameplay cannot overwrite Candidate.

Effects declare Gameplay-reference dependency. Dependent effects require a
compatible Current/Retained reference. Independent Cinematic/Dialogue effects
are not vetoed by an invalid Gameplay reference. The offline caller must declare
the actual dependency; production policy binding remains an M2 obligation.

ENTER/EXIT, completion, provider arm, Dialogue activation/termination, replacement
and fresh Gameplay admission revoke affected scope intents. Settings revoke
intents including partially observed ones. Cinematic/recovery/provider Dialogue
preparation checks generation. Superseded readback cannot refresh projection.
The core still models intents/observations, not live native effect execution.

## Admission and retention

Unknown, PolicyAdmitted and NativeProven are distinct. Compatibility admission
requires an explicit request, validated callback/source relation, original input
at the named Gameplay writer boundary, valid identities and absence of conflicting
claims in that source scope. Authority evaluates these rules itself. This does
not establish an engine role witness or alter production classification.

Caps: 128 effects, 32 armings, 64 scopes. Retired armings are removed; revoked or
fully observed effect history may be pruned under pressure. Pending observations
are never silently evicted. Full pending capacity rejects new intent preparation.
Correlated EffectClosed explicitly ends the observation window, including
Diagnostics OFF/missing readback, without inventing Applied. Replacement retires
the explicitly superseded scope. Repeated arming and 400 effect windows are tested.
These are storage bounds, not measured performance. Adapter closure and retirement
must be bound before production use; capacity refusal is not an approved live
behavioral fallback.

## Numeric coverage and validation

The existing harness now links real production Cinematic selector/mapping,
Reduced target, HorPlus and projection mapping. Generation-5 shadow computes
153.925/149.071; PID7372 passes the retained Authority reference into the real
selector, checks its origin and computes 153.931. Stale reference fallback is
checked. This is selected function integration, not full runtime-adapter equivalence.

- Sequential focused MSVC/Ninja build PASS, supervisor timeout 60 seconds.
- Default M1: 66 assertions PASS, including 21 hardening assertions.
- Shadow: 31 assertions PASS.
- Separate hardening: 21 assertions PASS; these duplicate the default's new cases.
- Canonical registry audit PASS: 60 targets, 63 invocations, 0 missing inputs;
  tracked test-registration diff whitespace check PASS.
- All build/test process trees: supervisor cleanup verified. Owned compiler helper
  cleaned when it outlived its build root.
- Frozen R1 model/result SHA-256 unchanged; exact hashes in r1-identities.json.
- Full suite, CTest, live callbacks, game, graphics and release build not run.

Core/harness and test-only source/dependency registration were reviewed;
pre-existing changes preserved. Remaining gates: adapter terminal, provenance,
dependency and finalization bindings; full supported transaction equivalence;
then separately approved M2 cutover. Native +0x628/+0x740 research remains separate.
