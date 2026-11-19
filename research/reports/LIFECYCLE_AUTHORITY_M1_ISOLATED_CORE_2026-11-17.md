# Lifecycle Authority M1: isolated core and offline binding review

Date: 2026-11-17. **M1 offline implementation and focused validation complete.**
This is not M2 and does not connect live callbacks, build/install an ASI, or
change game behavior. M2 cutover and M3 release/runtime remain NO-GO.

## Implemented

`src/plugin/lifecycle_authority_core.hpp` is a private, isolated C++ relation.
It is not included by `runtime.cpp` or any production adapter. Public input is
`Accept(EventFacts)`; it evaluates a private working copy and exposes only the
committed result and a by-value read projection. Rejected events do not commit.
The core imports no production callback, resolver, memory reader or engine API.

The relation models scoped Cinematic, Recovery, Dialogue and provider claims,
Gameplay baseline trust, effective settings/policy snapshots, and per-call
correction intents/results. Re-arming the same component/thread/generation tuple
allocates a new Authority event ID and retires the previous claim. Setter and
writer observations require the exact active event ID plus the explicit causal
join fact; tuple equality alone is rejected. The Authority ID remains our event
identity, not an Unreal activation/lifetime token.

FOV observations use separate provenance and boundary enums. Original capture,
prepared Dialogue/Cinematic input, primary writer input, planned output,
post-store readback and post-helper readback occupy distinct witnesses. A
prepared input does not overwrite an original capture. `EffectIntent` binds
scope, receiver, generation, writer transaction, settings epoch, dependency
revision and planned output. Post-store and post-helper results are stored
independently and only accepted with the matching transaction identity,
receiver, generation, epoch and dependency revision. Missing, duplicate,
N-1, wrong-receiver, wrong-generation or unnamed observations are rejected; a
planned value is never reported as an effective readback.

Gameplay baseline admission requires explicit Gameplay-role proof and finite
`OriginalNative` FOV at the named Gameplay writer-input boundary. Prepared
Dialogue input cannot refresh that baseline. A retained baseline does not by
itself re-authorize Gameplay correction after the Dialogue owner ends. Source
replacement and same-source receiver replacement invalidate the old baseline.
Blend/recovery completion alone does not synthesize Dialogue termination.

The test target is registered in the existing test registry and CTest path.
Its CMake object dependency explicitly tracks the isolated core header so a
header edit rebuilds this harness even under the project's unscanned Ninja
profile.

## Regression and oracle relation

`tests/plugin/lifecycle_authority_m1_harness.cpp` exercises 45 assertions:

- selected R1 ownership projection: initialization, Cinematic enter/exit,
  provider arming/setter/writer transfer, retained Gameplay reference and
  latched Dialogue policy;
- exact same-tuple rearming, stale old setter rejection and rejection of a
  tuple-only causal join;
- missing Gameplay-role/provenance rejection, source and receiver replacement,
  source-scoped Candidate behavior, and provider quarantine;
- original vs prepared FOV separation, planned vs effective values, same-call
  readback correlation, distinct post-store/post-helper witnesses and negative
  N-1/receiver/generation/missing-boundary/missing-correlation controls;
- settings revocation, partial effect observation and unproven Dialogue END.

The R1 model/result hashes are unchanged. The projection record is a reviewed
mapping of selected harness assertions to R1, **not** differential execution of
the JavaScript oracle. Same-tuple rearming and named FOV readback boundaries
extend beyond R1 and are not claimed as R1 coverage.

## Validation performed

- CMake configure of `build-artifacts/cm/test-suite`: PASS.
- Focused MSVC/Ninja build of `lifecycle_authority_m1_harness`: PASS (2 build
  steps). The expected `vctip.exe` compiler helper outlived the command root;
  the supervisor terminated that owned helper and verified process-tree
  cleanup. No project-owned test/build process remained afterward.
- Direct harness execution: `PASS`, 45 assertions.
- Focused CTest `lifecycle_authority_m1`: 1/1 PASS.
- `test_cmd_audit.ps1`: PASS; 60 registry targets, 60 repository harnesses,
  63 runner invocations, 117 native cases, 234 native child runs, 3 resource
  targets, 0 missing inputs.
- R1 model/result hashes were not modified.

The full `test.cmd` suite was not run. No production ASI was built or copied,
no live game run was performed, and no full non-graphics suite result is
claimed.

## Native binding limits retained for M2

The binding sheet under `research/evidence/lifecycle-authority-m1-2026-11-17/`
records current evidence and unresolved capabilities. In particular:

- an independent Gameplay-role proof for arbitrary new sources is not
  established;
- no general Dialogue terminal event is established after setter callbacks
  cease;
- component/thread/generation compatibility does not uniquely identify a
  native arming episode after component reuse; a causal native join is not
  established for every supported path.

These limits are explicit Unknowns, not symbolic facts an adapter may invent.
This report completes only the authorized isolated M1 work. It does not
authorize callback cutover (M2), release, or runtime testing (M3).
