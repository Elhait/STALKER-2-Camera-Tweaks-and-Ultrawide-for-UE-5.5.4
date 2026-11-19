# Runtime camera-context isolation audit

Date: 2026-11-17. Scope: temporary production FOV state across completed
Dialogue/Cinematic and save loads in one game process. Source inspection only;
no production modification, test execution, native probe or new ASI in this stage.
Plan: research/plans/RUNTIME_CAMERA_CONTEXT_ISOLATION_2026-11-17.md.

Follow-up clarification supersedes the original stage ordering: ordinary
Dialogue/Cinematic termination is P0; world lifetime is not its prerequisite.
The first repair now uses a still-current Gameplay reference to retire a
stationary post-recovery Candidate at the actual writer boundary. Separate
validation evidence will record the implementation and tests; the original
audit below remains the pre-fix inventory. Same-address save-load identity
remains an independent limit, not a reason to defer evidenced lifecycle repairs.

## Evidence and correction to earlier wording

Capture and hashes: research/evidence/multisave-cinematic-baseline-2026-11-17-012536/.
The user confirms generation 6's problematic scene was started in a second save.
The diagnostic record itself exposes receiver replacement, not a LoadGame event.
These are complementary sources of evidence, not interchangeable identities.

The Candidate was newly created at 01:31:00.827 AFTER the old Dialogue recovery
completed at .825. It was not simply the previous accepted Dialogue left active.
It remained pending after .841, without further setter callbacks before the next
Cinematic. At 01:31:22.508 a new primary receiver appeared. At 01:31:37.346 it
supplied native 130.311 with actual Gameplay correction 153.931. Admission stayed
TransitionAmbiguous. Generation 6 selected authored 90 -> 126.87 at 01:31:48.927.

## Confirmed defects and conflicting local contracts

1. ClassifyDialogueWriterContext (dialogue_boundary_transaction.hpp:190) applies
   a global baseline ambiguity veto for every non-Inactive phase, including a
   Candidate with no proven relationship to the current receiver.
2. Candidate has no writer-driven termination/reassessment path when callbacks
   stop. CandidateTracker::Observe can cancel it, but requires another callback.
   ResetDialogueBoundaryState exists; there is no save-load caller for it.
3. A Gameplay context-change callback exists: EvaluateGameplayContextChange,
   ProcessGameplayHorPlusWriter and ProcessGameplayAspectRecalculationWriter
   invoke resetDialogueIfActive. The actual runtime callback explicitly refuses
   when previousSource != source (runtime.cpp:7081) and requires the candidate
   to affect the current source. It was designed to preserve interleaved cameras.
   Thus the existing escape route cannot resolve this recorded source replacement.
4. Classification is captured before ProcessCameraWriter runs that reset path.
   Even a successful reset later in the writer does not retroactively establish
   native provenance for its already classified sample. A fix must not just
   relabel that same potentially setter-corrected input GameplayEligible.

The old-baseline source guard is working and must stay. The failure is denial
of legitimate new admission, not permission to reuse the old source's baseline.

## Available observation boundaries

| Current signal | What it establishes | What it does not establish |
| --- | --- | --- |
| Primary writer RSI/RBX, input, aspect, flags, thread | One native writer invocation and address relation | Loaded world, camera allocation lifetime, user's game/menu state |
| Cinematic ENTER/EXIT and aspect-store target | Accepted event, selected generation and source association | Guaranteed EXIT before save load, native world identity |
| Dialogue setter/provider armer and setter return | Captured component/receiver, values, thread and handoff evidence | Save load or completion if no further callback arrives |
| Writer identityEpoch | Observed source OR output address change | World generation; same-address allocation reuse; save-load detection |
| Settings epochs | Changes in the mod's effective settings | Game-world change; generations remain compatible across a load |
| Coordinator | Mod's own state machine | Independent confirmation of the newly loaded engine mode |
| Camera WorldFov/FirstPersonFov, aspect, flags | Numeric/bitfield observations at named boundary | A trusted LoadGame or world-lifetime notification |
| Overlay window rebind | Game-window/presenter/input lifetime | Loaded save/world lifetime |
| Post-store/helper readback | Correlated output observation for one invocation | World ownership or future native state |

HookSet, separate zoom hooks, RuntimeState and the currently installed adapters
contain no world pointer/lifetime token or save-load notification. This is a
finding about current captured/published inputs, not a claim that the engine has
no suitable signal. Merely creating a new contextEpoch counter would not supply
the missing evidence.

## Temporary-state inventory

| Owner/state | Creation and scope | Existing termination/replacement behavior | Audit disposition |
| --- | --- | --- | --- |
| Dialogue Candidate | Setter callback; callback source, optional validated/observed receiver, settings and cinematic generation | Further callback cancels/promotes; explicit Dialogue reset; Cinematic boundary resets | Confirmed callback-silence lifetime gap and global foreign-writer veto |
| Active Dialogue, active policy, correction activation | Accepted descent or confirmed provider handoff | Ascending/recovery callbacks, incompatibility/reset; foreign interleaved writer intentionally does not cancel | No claim of a new runtime failure; load without old callbacks is unverified |
| Dialogue RearmPending | Recovery that has not reached native endpoint | RecoveryRearm::Observe complete/cancel or reset | Source-bound in callback path; no independent world boundary |
| PostExitProviderHandoff | Authorized armer, then correlated setter/writer, generation/source/component/thread/settings | Incompatible evidence, explicit reset, recovery/admission consumption; endpoint baseline block is receiver/thread/generation scoped | Better scoped than Candidate; load/address-reuse behavior not globally proven |
| PostCinematicRecoveryExclusion | Accepted EXIT with captured recovery owner | Validated native recovery, source change cancellation, explicit reset/new ENTER | Has writer terminal paths; does not identify a loaded world |
| Cinematic generation/association | Accepted ENTER and aspect-store source | Accepted EXIT/new generation; conflict refuses association/admission | Active generation can remain locally active if load omits old EXIT; no safe automatic new-mode decision established |
| Enter activation and prior corrected writer witness | Accepted activation; source/thread/epochs/aspect/flags/component | Retire/RetireForReentry; compatible successor only; source/epoch conflicts fail closed | Existing repeated-generation guards stay; identical-address reuse across loads is not identified |
| Cinematic recovery owner/atomic handoff/continuation | Accepted EXIT with admitted source or documented compatible carry | Endpoint, explicit cleanup, receiver-change retirement while CinematicExiting; handoff cancelled by active Dialogue | Already has source-scoped writer retirement; this does not retire unrelated Dialogue Candidate or active Cinematic |
| Gameplay baseline | Eligible native writer observation | Current=false on foreign source or untrusted same-source sample; fresh eligible observation updates | Rejects stale source correctly; readmission starved by classifier in captured case |
| Gameplay replay/aspect restoration | Accepted Gameplay writes and pending mode/enable transitions | Replay and settings transition rules; current source/readability checks | Process-held state; no independent world reset; no additional defect established here |
| Camera State/original captures/neutral zoom facts | Named publication/observation boundary | Per-fact freshness, source and correlation guards; readbacks retired on new transaction | Projection only; must not become a load detector or infer new ownership |
| Writer post-store/helper frames | Thread-local invocation with monotonically increasing transaction id | Replacement marks missing readback; exact source/output/thread/latest checks; post-helper consumes pending observation | No cross-context transaction mismatch observed; not a world identity guarantee |

There is no reason to reset settings, hooks, workers or Overlay as a world-context
repair. ResetAllRuntimeResources/Shutdown are process/camera-core teardown, not
save-load handling, and cannot be reused as such.

## Why tests passed

The current Dialogue boundary harness explicitly expects an unverified Candidate
to make BOTH its observed receiver and a foreign receiver TransitionAmbiguous
(tests/plugin/dialogue_boundary_transaction_harness.cpp:235–240). Other tests
intentionally preserve accepted Dialogue across an interleaved foreign camera.
The camera-writer reset test also expects the current sample to remain excluded
after lifecycle reset; a later explicitly eligible sample then updates baseline.

Those local safety properties do not prove eventual new-context admission. The
suite omitted their composition: completed provider Dialogue -> new pending
Candidate -> no setter callbacks -> primary replacement -> continued native
Gameplay writes -> Cinematic ENTER. Prior fixed-seed Cinematic generation tests
did not generate this independent Dialogue callback silence/load sequence.

An expanded test must not just call ResetDialogueBoundaryState in its fixture
to simulate load and then claim production load isolation works. It must drive
the actual supported context observation adapter. Same-save reload at unchanged
addresses particularly cannot be modeled as an invented writer identity change.

## Required isolation contract

- A pending candidate is a source-scoped hypothesis, not global ownership.
- It may neither correct another context nor veto it without conflict evidence.
- It needs an evidence-bound terminal path independent of another setter call.
- Proven active Dialogue and genuine ambiguous/unknown ownership retain their
  safety checks. Receiver change alone cannot make a sample Gameplay.
- A confirmed context replacement retires incompatible temporary states through
  their existing owners before new policy selection/publication. User settings
  and installed capabilities survive. No engine writes to retired pointers.
- A new Cinematic/Dialogue can begin without a Gameplay sample. Use an explicit
  no-reference behavior, never the old context's baseline or corrected witness.
- Compatible short EXIT/ENTER continuity remains valid; ordinary EXIT alone is
  not a world invalidation event.

## Evidence limit and next bounded stage

Current inputs cannot guarantee general load-during-Cinematic/Dialogue isolation
or same-save/same-address reload detection: load and ordinary camera switching
can produce the same captured address/value sequence without an old EXIT. No
current hook publishes the distinguishing engine-lifetime fact. A global reset
on RSI change would hide this gap while breaking legitimate interleaving.

Before production edits, establish a reliable context-replacement fact in the
already intercepted native dataflow, or document a separately scoped observation
addition if no existing boundary supplies it. Then red regressions must include:
the recorded Candidate silence sequence; loads originating in Gameplay, active
Cinematic and active Dialogue; reload of the same save; new save immediately in
Cinematic/Dialogue; compatible reentry; interleaved cameras; receiver/epoch/source
conflicts; true provider handoff; unchanged settings; Diagnostics OFF/ON.

Stage 1 audit is complete. Stage 2 context-boundary evidence is incomplete;
production repair and new runtime claims remain pending. No tests/builds were
launched during this audit, and no game files were modified.
