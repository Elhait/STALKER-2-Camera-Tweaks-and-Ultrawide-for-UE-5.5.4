# Lifecycle Authority: isolated shadow replay

Date: 2026-11-17. This is an offline, test-only comparison step after M1.
The Authority is still not included by `runtime.cpp`; no production callback,
writer, setter, baseline store, recovery path, ASI, or game file was changed.

## What was connected

The existing `lifecycle_authority_m1_harness` has an opt-in
`--shadow-replay` path. It feeds captured, named event facts into the isolated
Authority and exercises the production `PostExitProviderHandoff` helper for
the observed armer/setter/writer relation. The normal M1 invocation remains
unchanged at 45 assertions. The shadow path is not a live observer and does
not independently discover or promote native proof.

The first red replay exposed a model gap: a validated provider arm did not
create even a scoped Dialogue Candidate, so later confirmed provider descent
could not activate Dialogue without an adapter-invented event. The private
Authority reducer now turns `ProviderArmed` into a source-scoped Candidate;
only another provider arm may supersede that Candidate, and it cannot replace
an already Active Dialogue. This changes only the isolated core used by tests;
R1 and all production sources remain untouched.

## Captured generation-5 provider handoff

Source: preserved PID 21200 runtime capture in
`research/evidence/multisave-cinematic-baseline-2026-11-17-012536/`.

- Armer: generation 5, receiver `0x22c91546700`, component `0x22e0e5c5320`,
  thread 21200, native start `130.311`, provider target `70`.
- The writer before a matched setter was not accepted as provider evidence.
- The setter matched the active armer on component, receiver, thread and
  endpoints; its callback value and setter argument matched at `130.264`.
- The following writer sample matched that setter on the same receiver/thread.
  The production log records the bounded handoff output `153.925` toward the
  captured Reduced target `149.071`.
- Shadow Authority accepted the same bounded chain, selected Dialogue/Reduced,
  retained the compatible Gameplay reference, and activated Dialogue only at
  the later observed provider descent endpoint.

Result: **the supported generation-5 path does not need an Unreal episode ID**
when the adapter binds setter and writer to the currently accepted arming event
through the validated, ordered callback relation. The local Authority event ID
is only that adapter correlation; it is not claimed to be native activation
identity.

Limit: the replay does not prove that two indistinguishable callbacks from
different activations can be separated if an old callback arrives after a new
arm with the same receiver/component/thread/endpoints tuple. The native
callback carries no independent episode token. The adapter must retire a
claim on supersession or incompatible evidence and must not promote tuple
equality alone into a causal join.

## PID7372 same-source recovery completion

The replay covers the repair ordering from
`research/evidence/same-save-recovery-completion-2026-11-17-020947/`: the
source-owned native recovery endpoint is admitted in its completion
transaction, then a neutral setter candidate arrives immediately, with no
extra Gameplay writer between them. The next Cinematic ENTER uses the same
receiver and retains the `130.311` Gameplay reference; the captured contract
expects the `GameplayNativeNumericBaseline` target `153.931`.

Shadow result: **PASS**. This supports a bounded same-source transition. It
does not authorize carrying this proof to a different receiver.

## New receiver and Dialogue scope

The current Dialogue classification helper returns `GameplayEligible` for an
otherwise unowned writer after the Dialogue phase is inactive. The shadow
Authority rejects a new-source Gameplay sample when the adapter has no
independent `gameplayRoleProven` fact, leaving the old receiver baseline
invalid. The harness records this as an expected divergence:

```text
current = GameplayEligible
Authority = Unknown; old baseline = Invalid
```

This is the remaining user-visible gate. If no independent source-bound proof
is found, GameplayHorPlus cannot safely use the old baseline after receiver
replacement; a later Cinematic may lack its Gameplay reference. The Authority
must keep that state Unknown until the product accepts a bounded fallback
policy. Reusing the old coordinator classification would hide the missing
binding rather than solve it.

The replay also confirms the model can keep Active Dialogue scoped to its
source while admitting another source that independently proves Gameplay. A
Dialogue terminal event without terminal evidence is rejected. Existing
supported terminal evidence remains bounded: same-source setter trajectory
reaching its observed native target, explicit accepted Cinematic ownership
transfer, or a source-scoped reset/replacement. Callback silence alone remains
insufficient; no universal Dialogue END binding is claimed.

## Validation and status

- Test target compiled: `lifecycle_authority_m1_harness` only.
- `--shadow-replay`: **29 assertions PASS** after the red-to-green candidate
  transition correction.
- Default M1 invocation: **45 assertions PASS**.
- Canonical registry contract audit: **60 targets / 63 invocations / 0 missing
  inputs PASS**.
- The focused runner verified process-tree cleanup. The full suite, live ASI,
  and game runtime were not run.

Disposition: the bounded generation-5 provider path and PID7372 same-source
completion can be represented in the shadow model without taking production
control. **M2 remains NO-GO** until independent new-receiver Gameplay proof is
either bound or a limited user-visible fallback is explicitly accepted, and
the native adapter’s supersession limits are made explicit. This replay does
not change M1’s frozen oracle or establish live callback equivalence.
