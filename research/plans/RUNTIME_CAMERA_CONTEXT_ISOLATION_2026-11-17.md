# Runtime camera lifecycle termination and context isolation

## Active user process gate — analysis before further repair

Current design continuation: LIFECYCLE_AUTHORITY_DESIGN_2026-11-17.md. User has
accepted a single transition authority over multiple related states, not one
universal enum and not writer-only option B. Design inventory/model/corpus review
is authorized; concrete transition rules and implementation remain unapproved.

User instruction c3b5ba81 (2026-11-17) supersedes automatic progression from a
new failure directly to regression/production changes. Current phase: READ-ONLY
LIFECYCLE ANALYSIS plus research/plan records only. Do not change production
code, tests, architecture contracts, settings, or build another ASI in this phase.
The already built 67E56485 artifact is not runtime-validated by the older capture.

Required next deliverable: full known-event chronology, successful/failed path
comparison, first proven invariant violation, competing root-cause hypotheses,
adjacent transition review, architectural alternatives/risks and test-model gaps.
Discuss that deliverable with the user and obtain an explicit selected solution
before implementation or regression changes. Existing code/contracts/tests are
evidence to evaluate, not reasons to preserve a flawed authority distribution.
Do not treat 62/62 PASS or a single successful scene as system-wide proof.

The implementation statuses below are historical batch results, not permission
to continue repairing production during this analysis/discussion phase.

Status: Batch 1 runtime partially confirmed; Batch 2 recovery-boundary repair
IMPLEMENTED / OFFLINE VALIDATED; new runtime repeat pending.
Validation: research/reports/DIALOGUE_CANDIDATE_TERMINATION_VALIDATION_2026-11-17.md.
Report: research/reports/RUNTIME_CAMERA_CONTEXT_ISOLATION_AUDIT_2026-11-17.md.
Production edits require supported lifecycle evidence and red regressions first.
LoadGame/world identity is P2 research, not a prerequisite for P0 termination.
This task does not declare every receiver change a save load.

Runtime follow-up PID 7372 (mod 3D30F286): same-save second scene fails because
the exact native recovery-completion writer commits coordinator=Gameplay while
retaining writerContext=RecoveryOwned. Baseline becomes non-current before the
immediate neutral setter Candidate. Batch 2: reproduce without an extra Gameplay
writer, then reconcile the completed source-owned recovery context before
baseline projection. Preserve provider endpoint, exclusion, foreign-source and
uncompleted recovery gates; no stale baseline resurrection. Run focused/full
regressions and standard Release, then verify this exact boundary in runtime.

Priority: P0 source-scoped temporary state and termination without another setter;
P0 fresh Gameplay admission after confirmed completion; P1 long same-save camera
sequences; P1 save-load isolation controls; P2 extra world lifetime observation
only for distinctions not established by existing native boundaries.

First bounded repair: a stationary pending Candidate can be retired by a real
writer when its validated receiver/epoch/generation, native target/start and
sample agree with the still-current production Gameplay baseline and aspect,
with Gameplay coordinator, no exclusion/provider endpoint and no active policy.
This is not a FOV-only expiration or a resurrection of a non-current baseline.
It terminates the captured post-recovery candidate before it can invalidate
the trusted reference. True descent, unknown/foreign identity, stale reference,
active Dialogue and Cinematic/Recovery remain protected. Add the actual runtime
adapter red reproduction before edits, then negative controls and sequences.

## Evidence and intended outcome

The user's failing cinematic began in the second loaded save in one process.
Preserved capture: research/evidence/multisave-cinematic-baseline-2026-11-17-012536/.
A new pending Dialogue Candidate after completed recovery outlived its callbacks
and prevented Gameplay baseline admission on a replacement primary receiver.

New confirmed camera context must not inherit incompatible temporary ownership,
ambiguity or corrected-value provenance. Preserve user settings, installed hooks
and process-lifetime resources. Preserve short EXIT/ENTER continuity within a
compatible context. Native generation, settings epoch and writer identity epoch
must not be presented as a confirmed world lifetime.

## Bounded stages

1. Inventory current observation signals and all temporary production FOV states:
   creation, scope, terminal paths without another initiating callback, receiver
   replacement behavior and tests. Record unsupported distinctions explicitly.
2. Define the narrow reset/retention contract from observed signals. Do not
   invent LoadGame notifications, timeout/callback-count expiration or global
   resets on receiver change. If signals cannot distinguish save load from
   legitimate camera replacement, document exactly what cannot be decided.
3. Red production-entry regressions for pending Candidate without more callbacks;
   load-equivalent captured boundary sequences from Gameplay/Cinematic/Dialogue;
   same-save reload and immediate restored Cinematic/Dialogue without Gameplay;
   negative controls for compatible reentry and true provider handoff.
4. Only after boundary evidence and regression contract are sufficient, repair
   existing owners minimally. No parallel manager, FOV formula changes, mode
   removal, settings resets or forced Gameplay-before-Cinematic assumption.
5. Focused and full tests, Diagnostics OFF/ON, ordinary Release ASI link only.
   Runtime validation remains separate. Supervise serial test/build process
   trees and verify cleanup; never replace the game ASI.

## Expected files and limits

Audit: runtime.cpp, dialogue_boundary_transaction.hpp, camera_writer_transaction.hpp,
cinematic_lifecycle_transaction.hpp, dialogue_state.*, post_exit_provider_handoff.hpp,
cinematic_enter_activation.hpp, cinematic_fov.*, cinematic_recovery_retirement.hpp,
gameplay_baseline.*, fov_observation.*, existing registered harnesses and hook set.
Any production edit is confined to a demonstrated contract defect in these owners.
New native observation points require their own evidence/plan; this task does not
silently authorize speculative hook installation.

Acceptance: recorded defect reproduced automatically; incompatible old states
cannot authorize or indefinitely veto new-context policy; old receiver baseline
is rejected and legitimate fresh baseline can be admitted; loading during active
Cinematic/Dialogue does not require the missing old EXIT; short same-context
EXIT/ENTER keeps continuity; readback is transaction-correlated; settings survive.
An unobserved save-load boundary remains an evidence gap, not a simulated PASS.

## Current disposition

- Audit of creation/scope/terminal paths: complete for the listed owners; original
  report retains the pre-fix inventory and explicit unknowns.
- Recorded stationary post-recovery Candidate defect: red reproduced through the
  actual adapter; minimal writer-bound terminal repair complete. This does not
  claim termination of a real descent/active Dialogue from missing callbacks alone.
- Same-session sequence and 20 negative controls: complete in bounded offline
  fixtures, including embedded choice, short reentry and native FOV changes.
- Full suite: 62/62 PASS, one explicit hardlink SKIP; native adapter 234 OFF/ON
  child runs. Release build and process-tree cleanup verified.
- Runtime validation of the new ASI: pending. No game-folder artifact installed.
- Batch 2: captured coordinator=Gameplay/writerContext=RecoveryOwned mismatch
  reproduced red by removing the fixture's extra post-completion writer. Fixed
  classification at the proven source-owned completion boundary. Exact completion
  publication/reference sequence and immediate setter are now asserted. Full
  suite 62/62 PASS, hardlink SKIP 1; report/evidence:
  research/evidence/same-save-recovery-completion-2026-11-17-020947/assessment.md.
- General load during active Dialogue/Cinematic and same-address reload: follow-up;
  native context identity is still not established. This does not block the above
  P0 repair. No global state reset or fictional LoadGame event was introduced.
- General lifecycle guarantee for every possible callback ordering: not claimed
  by the finite tests; preserve independent finding/repair stages when new
  concrete sequences contradict the current owners' contracts.
