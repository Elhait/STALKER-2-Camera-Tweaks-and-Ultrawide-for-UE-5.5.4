# Same-save second cinematic: recovery completion context mismatch

Date: 2026-11-17. User reports the second cinematic in the SAME save failed;
the formerly failing scene after loading another save worked. This corrects
the assumption that save loading is the principal boundary of this failure.

## Capture identity

Canonical STALKER2CameraTweaks.log, PID 7372, start 02:09:47.248.
Captured mod SHA-256:
3D30F28619EFFA64DAAF9256A683BB19C6C2BB8C6760A3113F733B3B368F2262.
Game SHA-256:
2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479.
Preserved log SHA-256:
8100D1F50DF7352EA8C8DDE5F0F6A7EF76E1A4A88AD039CFD3BF3FA780B80578.
RuntimeCombine / GameplayHorPlus / Reduced, Diagnostics ON, aspect 32:9.

## Confirmed sequence

- Generations 1–4 select the expected Gameplay native reference 130.311 and
  target 153.931, including writer-empty short reentry.
- 02:11:48.965: native input 130.313 reaches the first scene's EXIT endpoint.
  Recovery completes, exclusion clears, coordinator commits Gameplay. Diagnostic
  writer transaction 2259 nevertheless has writerContext=RecoveryOwned.
- ProcessCameraWriter forwards the pre-recovery classification into Gameplay
  projection. GameplayBaselineStore rejects RecoveryOwned as a fresh Gameplay
  observation and makes the same-source baseline non-current. This happens
  before a new Dialogue Candidate exists.
- 02:11:48.968: a neutral setter creates Candidate at 130.311 target/start.
- 02:11:48.981: next writer remains TransitionAmbiguous. The previous patch's
  retirement function correctly refuses the already non-current reference.
  There is no separate eligible Gameplay writer before the next cinematic.
- 02:11:53.797: generation 5 chooses authored 90 -> 126.87 because the Gameplay
  reference is unusable. Activation starts from corrected 153.931 and converges
  to that wrong target. EXIT summary confirms result range 126.87–153.931 and
  final 126.87, matching the user's narrowing report.
- After ordinary Dialogue recovery later in this same process, the earlier
  Candidate retirement patch does run at 02:12:34.247 and .260.
- After receiver replacement/loading, generations 6–9 use the correct
  GameplayNativeNumericBaseline and 153.931 target. This demonstrates the
  earlier patch's covered path; it does not validate the missing ordering above.

Final periodic diagnostic counters: 7,969 transactions/post-stores/post-helper
correlations, zero readback/correlation mismatches, zero queue-full drops,
three contended-event drops, pending zero. Do not claim a lossless full event
history or final worker shutdown. The causal boundary records above are present.

## Source cause and repair contract

runtime.cpp captures WriterContext before ProcessCameraWriter resolves recovery.
ProcessHorPlusRecovery can commit Gameplay on this invocation; the later Gameplay
dispatch previously preserved the retired RecoveryOwned classification.

Batch 2 reconciles that committed context before projection ONLY for completed
source-owned native recovery, action ResumeGameplay, coordinator Gameplay,
matching prior authoritative source, cleared exclusion, no provider evidence or
provider Dialogue continuation, and incoming RecoveryOwned classification.
Foreign/uncompleted recovery and provider/exclusion paths retain their gates.

This is fresh admission from a validated native recovery endpoint, not setting
current=true on an old baseline or relabeling a setter-corrected Dialogue sample.
No FOV formula, native hook, mode, clock/callback-count expiry or world detector
changes. The existing Candidate retirement then has its required current proof.

## Regression gap and evidence

The previous same-session fixture supplied an additional Gameplay writer after
completion and before the setter. That writer could repair the lost reference,
so the fixture missed the real callback ordering.

The regression now asserts current/usable reference and GameplayEligible
publication from the EXACT completion transaction, with matching observation
sequence, then invokes the neutral setter immediately without an extra writer.
This initially failed against the captured build's implementation with
"Recovery completion committed Gameplay with a non-current baseline".
The focused case passes after the production fix. Both assertions remain in
the registered long same-process scenario, exercised with Diagnostics OFF/ON.
Native engine field updates are emulated; this is not rendered-view evidence.

Final suite/build/artifact evidence is recorded alongside this report when
complete. A new runtime repeat of the same-save first/second scene boundary
remains pending; game-folder ASI installation is not performed by the agent.

Final test.cmd: 62/62 PASS, zero FAIL, one explicit hardlink SKIP (error=5).
Runtime adapter: 234 fresh child processes, including the tightened completion
ordering in Diagnostics OFF/ON. non_graphics=0; WARP_case_omitted=0. Supervisor
exit 0, no timeout, CleanupVerified=true. See recovery-completion-suite.json.

Standard Release ASI built successfully; root and production candidate SHA-256:
67E56485DAE42842331FD034AD1A6B574E63265C69CEF54F0B779513BB714DDE.
Build exit 0, no timeout, cleanup verified. Final host check found no compiler,
CMake/Ninja, vctip, harness or owned exit-status/shutdown probe processes.
No game files were modified. git diff --check passed (line-ending warnings only).
