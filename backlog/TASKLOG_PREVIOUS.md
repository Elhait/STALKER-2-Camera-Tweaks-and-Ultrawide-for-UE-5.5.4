# Task Log

This document records evidence-backed research outcomes, completed implementations and exceptional repository housekeeping. Ordinary README, release-text, index and link-maintenance edits are intentionally not logged here.

## 2026-09-18 — DialogueClassifierV2 Batch 1 recovery-stage correction

- Scope: correct only the premature post-cinematic exclusion release exposed by
  the first runtime validation; Batch 2/3 and native Dialogue ownership
  research remain out of scope.
- Paths changed: `src/dialogue/dialogue_state.hpp`,
  `src/dialogue/dialogue_state.cpp`,
  `tests/dialogue/post_cinematic_exclusion_harness.cpp`, and the Batch 1
  design/task records.
- Evidence: the prior runtime build released exclusion on the first post-EXIT
  sample already equal to the captured target. HorPlus then produced an early
  Dialogue Candidate; AspectRecalculation likewise released without proving
  that native recovery had started.
- Completed: explicit two-stage lifecycle. `WaitingForRecovery` keeps the
  exclusion active at an equal/near-target first sample; a later same-source
  finite departure beyond the existing recovery epsilon enters `Recovering`;
  only subsequent convergence releases the exclusion. Invalid samples and
  source replacement remain fail-safe cancellation paths.
- Validation: `test.cmd` PASS, including neutral-first-sample, recovery-start
  and convergence coverage; `build.cmd` PASS with only existing external
  Zydis C4201 warnings. Game/runtime was not launched after this correction.
- Remaining: one runtime validation of the corrected build across HorPlus and
  AspectRecalculation cinematic EXIT paths.
- Deferred: Dialogue Candidate/Active hardening, selected/active policy
  snapshot semantics and direct native Dialogue ownership access.
- Blocked: none for this bounded correction.
- Patch summary: prevented neutral post-EXIT samples from releasing the
  Dialogue exclusion before native recovery is positively observed.
- Changelog summary: tightened the Batch 1 post-cinematic Dialogue guard;
  no HorPlus, AspectRecalculation or Dialogue policy behavior was otherwise
  changed.

## 2026-09-18 — DialogueClassifierV2 Batch 1 post-cinematic exclusion

- Scope: implement only the confirmed post-cinematic Dialogue false-positive
  exclusion; no classifier hardening or hotkey policy snapshot.
- Paths changed: `src/plugin/runtime.cpp`, `src/dialogue/dialogue_state.hpp`,
  `src/dialogue/dialogue_state.cpp`, `tests/dialogue/`, `test.cmd`, and the
  practical repair design report.
- Validation: Phase 0 source contract passed; `test.cmd` passed all existing
  harnesses plus `post_cinematic_exclusion=PASS`; `build.cmd` passed with only
  external Zydis C4201 warnings. `git diff --check` passed. Game/runtime was
  not launched.
- Completed: explicit post-cinematic exclusion state; arm on EXIT; native
  pass-through during recovery; deterministic same-source target convergence;
  invalid/source-change cancellation; ENTER/runtime reset; production helper
  harness coverage.
- Remaining: one runtime validation of HorPlus and AspectRecalculation after
  cinematic EXIT.
- Deferred: Dialogue Candidate/Active hardening and selected/active policy
  snapshot semantics (Batch 2/3); positive native ownership seam.
- Blocked: none for Batch 1; runtime behavior remains unvalidated.
- Not runtime-validated: all injected ASI behavior in the game.
- Patch summary: added only the bounded recovery exclusion and its deterministic
  helper tests; existing HorPlus and AspectRecalculation choreography remains.
- Changelog summary: Dialogue post-cinematic false-positive guard added;
  no new runtime mode or config change.

## 2026-09-15 — v1.0.0 Batch 4.1 production regression matrix

### Scope and non-goals

- Create the bounded Batch 4 regression matrix before any game execution.
- Protect the frozen v0.6.0 Gameplay, Cinematics, Dialogue, configuration and
  resolver contracts, plus intentional Batch 3 safety semantics.
- No production code, release metadata or runtime configuration changes.

### Changed paths

- Added `backlog/active/V1_0_0_BATCH4_PRODUCTION_REGRESSION_TASK_PLAN.md`.

### Result

- Matrix covers gameplay initialization/re-arm, normal aspect/FOV behavior,
  atomic post-cinematic handoff, cinematic policies/Auto switching, dialogue
  policies/transitions, configuration/persistence, resolver startup and focused
  safety harnesses.
- Manual execution is bounded to four core integrated scenarios, with at most
  two conditional scenarios only if specific rows cannot be closed from the
  core runs, harnesses and existing evidence.
- Each row identifies expected observable behavior, protected prior contract,
  required evidence and whether static/harness or in-game validation is needed.
- Steam 2.0.5 remains the only runtime validation target.

### Status

- Completed: pre-execution Batch 4.1 matrix and execution gates.
- Remaining: Batch 4 in-game regression execution.
- Deferred: README/version/release metadata until final regression PASS.
- Blocked: none.

### Patch summary

Defined a bounded, evidence-based production regression matrix for v1.0.0
without launching the game or changing production behavior.

### Changelog summary

None; regression planning only.

## 2026-09-15 — v1.0.0 Batch 4 regression: Gameplay disabled

### Scope

- Record the first contradictory Batch 4 result after the otherwise passing
  21:9 and hotkey regression runs.
- No production fix or broad re-test in this entry.

### Evidence

- Steam 2.0.5 and current ASI identities match the preflight hashes.
- Configuration log reports `Gameplay.Enabled=0` and
  `Gameplay=DISABLED`.
- The same startup installs a read-only camera observer because cinematic
  features are enabled.
- User observed that the aspect-ratio setting no longer responds correctly and
  that the fix appears active despite Gameplay being disabled.
- The symptom is intermittent and is tied to the first launch after disabling
  Gameplay and restarting the game; leaving and re-entering the game can make
  the behavior correct without a source or configuration change.
- A subsequent restart reproduced the same failure, upgrading this from a
  one-off/intermittent report to repeatable post-restart regression evidence.
- The latest log contains cinematic subsystem initialization only; no cinematic
  ENTER/EXIT, aspect-store application, atomic handoff or recovery event was
  observed in the reproducing run.
- Static audit found no Gameplay-disabled observer memory mutation or reachable
  gameplay replay write. The globally installed cinematic aspect-store hook is
  an unguarded write-path candidate, but no store-hit log exists in the
  reproducer.
- The unconditional `Gameplay aspect fix loaded` message is confirmed
  misleading status logging, not proof of Gameplay activation.
- Follow-up menu reproduction produced no `Cinematic aspect store:` event while
  `AspectRatio` clicks produced UI sounds and save-on-exit; the option became
  adjustable only after re-entering the menu. The cinematic aspect-store firing
  hypothesis is rejected for this interaction; camera-observer hook interaction
  and native settings apply/re-entry lifecycle remain open.

## 2026-09-15 — v1.0.0 final safety findings verification

### Scope

- Re-verify the original P0/P1/P2 findings against the current production tree.
- Review reachable pre-cinematic work for the reported Gameplay-disabled
  stutter.
- No production changes, game launch or new probes.

### Result

- Worker lifecycle: `FIXED + VERIFIED`.
- Initialization rollback: `FIXED + VERIFIED`.
- Cinematic failed-write path: `INTENTIONAL BEHAVIOR + VERIFIED`.
- `PersistConfigValue`: `FIXED + VERIFIED`.
- `SynchronizeManagedConfigTemplate`: `REOPENED — NEW EVIDENCE`; it still uses
  destructive live-file truncation during startup template synchronization.
- Loader-lock cleanup: `FIXED + VERIFIED`; normal DLL unload remains unclaimed.
- Legacy `helper.hpp`: `ARCHITECTURAL DEBT`; no reachable stable-runtime write
  defect confirmed.
- Gameplay-disabled observer cost: `PLAUSIBLE CANDIDATE`; each camera-writer
  hit can perform `VirtualQuery + memcpy` through `SafeRead` before any
  cinematic.

### Status

- Completed: read-only verification report.
- Remaining: bounded persistence-template fix and/or targeted observer-cost
  validation.
- Deferred: Batch 4 final closure and v1.0.0 release documentation.
- Blocked: release gate remains stopped by the open regression and findings.

### Patch summary

Re-verified current safety findings and identified a separate destructive
startup template-write path plus a plausible hot-path stutter candidate.

### Changelog summary

None; verification only, no production behavior changed.

## 2026-09-15 — v1.0.0 template persistence safety fix

### Scope

- Replace destructive live-file template synchronization with staging and
  atomic replacement semantics.
- Add bounded template synchronization failure-injection cases.
- No observer, helper, feature or release changes.

### Changed paths

- `src/config/config_template.hpp`
- `src/config/config_template.cpp`
- `src/plugin/runtime.cpp` (template failure logging callback wiring)
- `tests/config/config_persistence_harness.cpp`
- `backlog/active/V1_0_0_TEMPLATE_PERSISTENCE_SAFETY_FIX_TASK_PLAN.md`

### Validation

- Template normal synchronization: `PASS`.
- Template staging failure preserves live INI: `PASS`.
- Template replacement failure preserves live INI: `PASS`.
- Existing persistence cases: `PASS`.
- Worker lifecycle harness: `PASS`.
- Feature status harness: `PASS`.
- `build.cmd`: `PASS`.
- `git diff --check`: `PASS`.

### Result

- Confirmed template persistence defect: `FIXED + VERIFIED`.
- Live-file destructive fallback: removed.
- Observer hot-path candidate: unchanged and remains a separate open batch.
- Batch 4 release gate: remains stopped.

### Patch summary

Managed template synchronization now commits through a validated staging file
and preserves the last-known-good INI on every tested failure path.

### Changelog summary

Configuration template synchronization now fails closed without truncating the
existing user configuration.

## 2026-09-15 — v1.0.0 observer hot-path design audit

### Scope

- Audit the Gameplay-disabled camera-writer observer before any production
  modification.
- Compare context-derived, reduced-frequency and observer-removal designs.
- No source changes, game launch or probe.

### Result

- `XMM0` carries FOV for the validated `MOVSS [RBX+0x30], XMM0`; it cannot
  replace the aspect read from the camera state.
- `Cinematics=Auto` resolves from the Win32 client viewport, not
  `g_lastObservedAspect`.
- The observer cache has no demonstrated role in current Auto calculation.
- Preferred design: do not install the shared camera-writer observer when
  Gameplay is disabled; keep independent cinematic hooks unchanged.
- The observer remains a plausible stutter candidate, not a proven causal
  explanation.

### Status

- Completed: bounded static design audit.
- Remaining: separate implementation batch and one targeted runtime validation.
- Deferred: full Batch 4 regression and release gate.
- Blocked: release closure remains stopped until the Gameplay-disabled
  regression is resolved or otherwise classified.

### Patch summary

Established that the observer cache is unnecessary for the current Auto aspect
resolver and selected a bounded observer-removal design.

### Changelog summary

None; design audit only, no production behavior changed.

### Classification

- Batch 4 regression: `CONFIRMED by user observation`.
- Exact causal owner: `OPEN`; the log proves observer installation but not a
  direct gameplay write in this run.
- Startup/re-entry sensitivity: `CONFIRMED by user observation`; this points to
  a lifecycle/state interaction but does not identify the causal writer.
- Restart reproduction: `CONFIRMED by repeated user observation`.
- Cinematic lifecycle in reproducer: `NOT OBSERVED`; post-cinematic paths are
  excluded from this causal branch.
- Batch 4 final closure: `STOPPED`.

### Status

- Completed: regression recorded with expected-vs-actual evidence.
- Remaining: bounded causal audit of `Gameplay=false + Cinematics=Auto`.
- Deferred: any production fix and final release/documentation gate.
- Blocked: Batch 4 closure pending causal audit and targeted validation.

### Patch summary

Recorded a configuration-independence regression where Gameplay disabled still
leaves a cinematic-related camera observer installed and the user reports that
native aspect settings no longer respond.

### Changelog summary

None; regression investigation only.

## 2026-09-15 — v1.0.0 Batch 4 R1 gameplay result

### Scope

- Validate the integrated R1 startup/gameplay scenario after the v1.0.0
  refactor and Batch 3 safety changes.

### Result

- Gameplay aspect correction: `PASS`.
- Gameplay correction after death or save reload re-arms correctly: `PASS`.
- User also observed cinematic and dialogue behavior working in the same
  regression run; their detailed scenario rows remain tracked under R2/R4.

### Status

- R1 gameplay rows G1–G3: `PASS` by direct runtime observation.
- R1 startup/resolver rows S1/S2 and configuration row X1: retained from the
  preflight/startup evidence and not reclassified from visual observation alone.
- Next phase: wait before executing R2.

### Patch summary

Recorded successful gameplay correction and post-death/save-reload re-arm
behavior for the first integrated regression scenario.

## 2026-09-15 — v1.0.0 Batch 3 final safety closure review

### Scope and non-goals

- Compare the approved Batch 3 safety candidates with the completed evidence
  and final classifications.
- Do not reopen Batch 3.1–3.5 or invent new safety candidates.
- No production source changes.

### Changed paths

- Added `research/reports/V1_0_0_BATCH3_FINAL_CLOSURE_REVIEW.md`.

### Final result

- Worker/shutdown lifecycle: confirmed issue addressed and harness-validated.
- Initialization rollback: feature-local degradation and shared-fatal cleanup
  validated.
- Cinematic failed write: intentional fail-closed behavior; no defect.
- Config persistence: confirmed data-loss defect fixed and failure-injection
  validated.
- Hook rollback: no orphaned production hook or incomplete cleanup defect
  confirmed.
- Loader-lock path: no blocking detach teardown; normal unload remains
  explicitly unclaimed.
- Generic helper safety: no separate reachable defect remained established.

### Validation and limits

- Existing lifecycle, feature-status and config-persistence harnesses: PASS.
- Production build after the final Batch 3 implementation: PASS.
- `git diff --check`: PASS.
- Full in-game regression remains Batch 4 and is not claimed by this closure.

### Status

- Completed: all approved Batch 3 safety candidates classified and resolved or
  intentionally preserved.
- Remaining: Batch 4 production regression validation.
- Deferred: new safety work only if contradictory evidence appears.
- Blocked: none.

### Patch summary

Closed the approved Batch 3 safety phase after reconciling all planned audit
findings with implementation evidence and focused harness results.

### Changelog summary

None; this is a phase-closure record. The relevant persistence safety fix is
already recorded in the Batch 3.4 implementation entry.

## 2026-09-15 — v1.0.0 Batch 4 R3/R4 result

### Scope

- Validate cinematic framing policies and dialogue policies through the
  enabled F9/F10 hotkeys on Steam 2.0.5.

### Evidence

- Loaded ASI SHA-256: `E3191767E826297E61AFA98E0B85E5389A9897D5B56EE7AAF17E7A75B3AFA6DD`.
- Loaded game SHA-256: `E7B481A97C02D80581FAB0BECE940214A88EBE30211088A00129845A039F9293`.
- Startup status: Gameplay, CinematicAspect, CinematicFOV, Dialogue and
  Hotkeys all `AVAILABLE`.
- F9 selected `Native`, `16:9`, `21:9`, `32:9` and `Auto`; each policy was
  followed by cinematic ENTER/EXIT and recovery evidence.
- F10 exercised `Disabled`, `Native`, `Adaptive` and `Reduced`; each policy
  produced dialogue lifecycle exit/recovery evidence.
- Persistence log entries were emitted for every hotkey policy selection.

### Result

- R3 forced framing series: `PASS`.
- R4 dialogue policy series and transitions: `PASS`.
- R2 Auto cinematic/EXIT recovery: `PASS` from the same run.
- R5 Auto hot-switch: not required; existing validated evidence already covers
  runtime aspect switching and no contradictory evidence appeared.
- R6 separate hotkey scenario: not required; F9/F10 were exercised directly.

### Status

- Completed: R1–R4 regression scenarios.
- Remaining: targeted audit of `Gameplay.Enabled=false + Cinematics=Auto`
  and revalidation of the independent Gameplay contract.
- Blocked: final Batch 4 closure and release/documentation gate pending the
  open regression.

### Patch summary

Validated all cinematic framing and dialogue policies after the v1.0.0
refactor using the production hotkey paths.

## 2026-09-15 — v1.0.0 Batch 4 final 21:9 cinematic check

### Scope

- Validate the complete cinematic policy series on the 21:9 runtime aspect
  `2.38889` after the R3/R4 regression run.

### Evidence

- ASI and Steam 2.0.5 executable identities match the preflight hashes.
- Startup status: Gameplay, CinematicAspect, CinematicFOV, Dialogue and
  Hotkeys all `AVAILABLE`.
- F9 sequence covered `Native`, `16:9`, `21:9`, `32:9` and `Auto`.
- `Auto` and configured `21:9` used runtime aspect `2.38889` and transformed
  authored FOV `90` to approximately `106.688`.
- Every tested cinematic produced ENTER/EXIT and recovery evidence, including
  `AtomicExitHandoffArmed` and gameplay return.

### Result

- 21:9 cinematic policy series: `PASS`.
- 21:9 Auto Hor+ framing: `PASS`.
- 21:9 EXIT/recovery: `PASS`.
- No regression or contradictory evidence found.

### Status

- Batch 4 runtime matrix R1–R4: `PASS`.
- Remaining: targeted audit of `Gameplay.Enabled=false + Cinematics=Auto`
  and revalidation of the independent Gameplay contract.
- Blocked: final Batch 4 closure and v1.0.0 documentation/release gate
  pending the open regression.

### Patch summary

Completed the final 21:9 cinematic compatibility check for all supported
framing policies without production changes.

## 2026-09-14 — v1.0.0 Batch 3.5 hook installation rollback classification

### Scope and non-goals

- Audit partial hook installation and rollback after the Batch 3.2 changes.
- No production source changes, hook-order changes or transactional `HookSet`
  redesign.

### Changed paths

- Added `research/reports/V1_0_0_BATCH3_5_HOOK_INSTALLATION_ROLLBACK_CLASSIFICATION.md`.
- No production source files changed.

### Classification

- Dialogue, cinematic aspect, cinematic FOV and gameplay hook initialization
  each have bounded local failure cleanup.
- The outer shared-fatal path resets workers, diagnostic hooks, production
  hooks, cinematic aspect state and gameplay availability.
- No orphaned production hook or incomplete rollback path was confirmed.
- `HookSet` remains an intentional passive ownership container; global
  transactionality is not justified by current evidence.

### Validation and limits

- Static control-flow review completed for `plugin::Initialize`,
  `ResetAllRuntimeResources` and `hooks::HookSet`.
- No production build or runtime test was needed because production code was
  unchanged.
- Existing feature-local graceful degradation and shared-fatal semantics remain
  the accepted contracts.

### Status

- Completed: bounded hook installation/rollback classification.
- Remaining: none for this candidate.
- Deferred: transactional HookSet redesign unless new evidence demonstrates a
  reachable cleanup gap.
- Blocked: none.

### Patch summary

Confirmed that current feature-local and shared-fatal rollback paths cover the
reviewed production hook installation failures; no code change was justified.

### Changelog summary

None; static safety classification only.

## 2026-09-14 — v1.0.0 Batch 3.4 non-destructive config persistence fix

### Scope and non-goals

- Remove the destructive direct-write fallback after failed atomic replacement.
- Preserve the last-known-good INI on staging or replacement failure.
- No config-format, default, parsing, managed-template or runtime feature changes.

### Changed paths

- Updated `src/config/config_repository.cpp`.
- Added `tests/config/config_persistence_harness.cpp`.
- Updated `research/reports/V1_0_0_BATCH3_4_CONFIG_PERSISTENCE_FAILURE_CLASSIFICATION.md`.

### Implementation

- Temporary open/write failures now attempt staging cleanup before returning
  `false`.
- A failed `MoveFileExW` no longer falls back to opening the live INI with
  `std::ios::trunc`.
- The source read stream is closed before staging/replacement so the normal
  atomic replacement path can succeed on Windows.
- The replacement failure is logged, the staging file is removed where
  possible, and persistence returns `false` while preserving the original.
- No fallback remains after failed replacement; this is intentional because a
  non-destructive fallback could not be established within this bounded batch.

### Validation

- Config persistence harness: `normal=PASS`.
- Staging/write failure preservation: `PASS`.
- Replacement failure preservation: `PASS`.
- Production `build.cmd`: `PASS`.
- Existing lifecycle and feature-status harnesses: `PASS`.
- `git diff --check`: `PASS`.
- In-game regression and filesystem power-loss behavior: not claimed; Batch 4
  covers runtime regression.

### Status

- Completed: confirmed data-loss defect removed with non-destructive failure semantics.
- Remaining: none within Batch 3.4.
- Deferred: unrelated Batch 3 safety candidates and full in-game regression.
- Blocked: none.

### Patch summary

Removed the live-file truncating fallback and added observable persistence
failure tests proving that the last-known-good INI remains intact.

### Changelog summary

Configuration persistence now fails closed when atomic replacement is
unavailable instead of risking an empty or partial live INI.

## 2026-09-14 — v1.0.0 Batch 3.4 config persistence failure classification

### Scope and non-goals

- Audit the post-Batch-2 `config::PersistConfigValue` failure paths.
- Determine whether a failed persistence operation can destroy the last-known-good INI.
- No production code changes, config-format changes, default changes or runtime test.

### Changed paths

- Added `research/reports/V1_0_0_BATCH3_4_CONFIG_PERSISTENCE_FAILURE_CLASSIFICATION.md`.
- No production source files changed.

### Evidence and classification

- The completed temporary file is first passed to `MoveFileExW`.
- After a failed replacement, the fallback opens the original INI with
  `std::ios::out | std::ios::trunc`.
- That open truncates the last-known-good file before the fallback write has
  succeeded; a later write/flush failure returns `false` with an empty or
  partial original possible.
- Classification: config persistence data-loss path is `CONFIRMED DEFECT`.
- Temporary-file cleanup on the earlier temporary-write failure is recorded as
  `ARCHITECTURAL DEBT`, not expanded in this batch.

### Validation and limits

- Source control flow reviewed in `src/config/config_repository.cpp`.
- Caller behavior reviewed in `src/plugin/runtime.cpp`.
- No production build was run because this batch made no production changes.
- No failure-injection harness was added; implementation and focused validation
  are the next bounded batch.

### Status

- Completed: bounded audit and failure classification.
- Remaining: design and implement a non-destructive persistence failure path.
- Deferred: broad config cleanup and unrelated Batch 3 safety candidates.
- Blocked: none.
- Not runtime-validated: filesystem fault behavior remains to be tested after implementation.

### Patch summary

Confirmed that the direct-write fallback can truncate the last-known-good INI
before a later write failure, violating the persistence safety invariant.

### Changelog summary

None; this was a safety audit with no production behavior change.

## 2026-09-12 — Clarify boolean values in production INI

### Scope and non-goals

- Add plain-language explanations for `true` and `false` under the
  `[Gameplay] Enabled` and `[Hotkeys] Enabled` settings.
- Synchronize the distributed INI and the ASI-generated default template.
- No runtime logic, defaults, hooks, FOV behavior or resolver changes.

### Changed paths

- Updated `release-assets/STALKER2CameraTweaks.ini`.
- Updated the embedded INI template in
  `src/experimental_cinematic_21_9_combined_fix_204.cpp`.
- Rebuilt `STALKER2CameraTweaks.asi` and refreshed the 0.6.0 archive.
- Completed and archived `INI_BOOLEAN_HELP_TEXT_TASK_PLAN.md`.

### Validation and limits

- Build: PASS with the established Visual Studio 2022 x64 toolchain.
- New ASI SHA-256:
  `7F88F7A3547AB88D6E5358DE7E28757692DC4F651D72B6D31FD0BAC9E3351ACE`.
- Embedded template contains both boolean explanations and the v0.6.0 header.
- Production markers remain present; research/deferred markers remain absent.
- Release asset and extracted archive contain the same new ASI SHA.
- Runtime log validation on Steam 2.0.5: PASS. The new SHA was loaded, all
  production hooks initialized, and gameplay plus RecoveryStart atomic applies
  behaved as expected with no legacy staged replay.

### Status

- Completed: user-facing boolean help text and template synchronization.
- Completed: runtime sanity check of the new comment-only binary.
- Production behavior: unchanged by design.

### Patch summary

Clarified boolean configuration values for non-technical users and kept the
packaged INI synchronized with the automatically generated template.

## 2026-09-12 — Prepare v0.6.0 release package

### Scope and non-goals

- Execute the release preparation playbook for 0.6.0 through A8.
- Promote only the runtime-validated production binary and documented atomic
  gameplay/RecoveryStart behavior.
- No source, build configuration or production behavior changes.
- No publication, commit, tag, upload or native FOV bypass research.

### Changed paths

- Updated `README.md`, `RELEASE_NOTES.md`, `TESTING_AND_RESEARCH.md`,
  `GITHUB_RELEASE_BODY.md` and `NEXUS_DESCRIPTION.md` for 0.6.0 claims.
- Replaced `release-assets/STALKER2CameraTweaks.asi` with the exact
  runtime-tested production binary.
- Updated release-assets README and INI metadata.
- Created `release-assets/STALKER2CameraTweaks-UE5.5.4-v0.6.0.zip`.
- Moved nine historical release ZIPs to
  `build-artifacts/archive/release-assets-history/` without deletion.
- Created and completed `RELEASE_0.6.0_EXECUTION_TASK_PLAN.md`, archived under
  `research/completed/`.

### Validation and Git state

- Production ASI SHA-256: `A0A5D0823C79B3A6E785F7244439402E5CED04CE37CE670F75FE8CCFE1932372`.
- Release-assets ASI and extracted archive ASI match that SHA byte-for-byte.
- Archive contains exactly: ASI, INI, README, LICENSE and third-party notices.
- A0–A8 release gates: PASS.
- Branch: `main`; HEAD: `8b9a329`.
- Working tree contains extensive pre-existing user changes and research
  artifacts; no unrelated paths were modified by this release batch.
- Release-facing `git diff --check`: PASS. Existing unrelated TASKLOG
  trailing whitespace remains untouched.

### Completed / remaining / deferred

- Completed: evidence/claims, production binary provenance, plan/archive
  classification, documentation, release assets, archive extraction and final
  consistency review.
- Remaining: user publication and any Git commit/tag approval.
- Deferred: native cinematic FOV bypass; interpolation owner remains unresolved.
- Not runtime-validated: Steam builds older than 2.0.5.

### Patch summary

Prepared the 0.6.0 release package around the exact production binary already
validated in-game, with claims limited to runtime Steam 2.0.5 evidence and
static resolver portability across Steam 2.0.2–2.0.5.

### Changelog summary

0.6.0 documents and packages the atomic gameplay correction and single
post-cinematic RecoveryStart handoff, removing the mod's secondary flick while
preserving native FOV recovery.

## 2026-09-12 — Integrate Combined Atomic Cinematic/Gameplay Handoff into production build

### Scope and non-goals

- Promote the runtime-validated Combined Atomic Cinematic/Gameplay Handoff
  behavior into the production `STALKER2CameraTweaks.asi` build.
- Replace the active staged gameplay transition with the atomic
  `1.777778 / flags 0x4` apply and use the first confirmed descending native FOV
  sample as the one-shot `RecoveryStart` trigger.
- No native FOV recovery, `CameraComponent +0x230`, physical setter
  `RVA 0x205FCC8`, cinematic FOV formula, dialogue behavior or user-selected
  gameplay FOV changes.
- No Pass1–Pass4 instrumentation, debugger code, timer/deferred replay or
  native FOV bypass in the production build.

### Changed paths

- Updated `build.cmd` to enable only the validated atomic gameplay and
  RecoveryStart handoff paths and link `bcrypt.lib`.
- Updated `src/experimental_cinematic_21_9_combined_fix_204.cpp` so the old
  staged Auto-restore branch is excluded from the atomic production build.
- Archived completed plan as
  `research/completed/PRODUCTION_COMBINED_ATOMIC_HANDOFF_INTEGRATION_TASK_PLAN.md`.
- Rebuilt `STALKER2CameraTweaks.asi`.

### Validation and Git state

- Build: PASS with Visual Studio 2022 MSVC x64 toolchain.
- Artifact: `STALKER2CameraTweaks.asi`, 1,105,408 bytes.
- Artifact SHA-256:
  `A0A5D0823C79B3A6E785F7244439402E5CED04CE37CE670F75FE8CCFE1932372`.
- Binary string checks: legacy staged/deferred/research trace markers absent;
  `AtomicReplayApplied` and `RecoveryStart` present.
- `git diff --check`: PASS for the changed source/build paths.
- Repository: `main` at `8b9a329`; working tree contains extensive pre-existing
  user changes and untracked research/build material. No unrelated paths were
  modified by this task.
- In-game production runtime validation: PASS on Steam 2.0.5.

### Completed / remaining / deferred

- Completed: production build selection now matches the validated combined
  atomic candidate behavior; old staged path is not present in the artifact;
  no native FOV writes were introduced.
- Completed: user-run production validation of normal gameplay, final
  cinematic EXIT and dialogue coexistence on the unchanged production binary.
- Deferred: native cinematic FOV bypass; its interpolation owner remains
  unresolved and outside this integration.
- Blocked: none for build delivery.

### Patch summary

Promoted the validated atomic gameplay aspect apply and first-downward-sample
RecoveryStart handoff into the production ASI build without changing native FOV
ownership or dialogue/cinematic FOV behavior.

### Changelog summary

Production candidate now uses one atomic gameplay framing correction and one
atomic post-cinematic RecoveryStart handoff, with the legacy staged `0x5`
transition removed from the production artifact. Runtime validation confirms
one natural native FOV recovery remains and the mod's secondary post-cinematic
flick is absent.

## 2026-09-01 — Reject direct ADS-register primitive transition

### Scope and non-goals

- Validate the refined read-only 2.0.4 primitive/ADS observer against the post-cinematic weapon/viewmodel correction.
- No camera, primitive or render-state writes; no production ASI changes; no `MarkRenderStateDirty` promotion.

### Changed paths

- Updated `src/weapon_viewmodel_primitive_ads_observer_204.cpp` with ADS register probes.
- Rebuilt the isolated diagnostic ASI.
- Updated `WEAPON_VIEWMODEL_FOV_POST_CINEMATIC_CAUSAL_TRACE_TASK_PLAN.md` with the runtime result.

### Validation and evidence

- Camera-writer, mesh-assignment, ADS IN and ADS OUT signatures resolved uniquely on Steam 2.0.4 and all hooks installed.
- At ADS IN/OUT markers, camera `+0x234` remained `90` and `+0x262` remained `0x1`.
- ADS register values remained stable. `RSI` had `+0x265=0x2` and parent `+0x265=0x0`; `RAX` and `RCX` did not form valid primitive/parent pairs.
- No inspected pointer or `+0x265` transition was observed at the reported correction boundary.

### Completed

- Rejected the direct ADS-register primitive transition as the causal explanation in this dataset.

### Remaining / deferred

- Actual rendered weapon/viewmodel primitive remains unresolved.
- Native primitive setter or direct render-refresh anchor requires separate bounded validation.
- `MarkRenderStateDirty` remains deferred until a concrete native refresh event is identified.

### Patch summary

Used the refined read-only observer to eliminate the shared-camera scalar and inspected ADS-register primitive-transition hypotheses.

### Changelog summary

Diagnostic-only research; stable gameplay/cinematic behavior unchanged.

## 2026-09-01 — Organize reverse-engineering research archive

### Scope and non-goals

- Separate active task plans from completed, deferred and rejected research after the published v0.4.0 release.
- Preserve all historical plans and evidence; no source, ASI, INI, release archive or build artifact changes.
- No new reverse-engineering or runtime validation.

### Changed paths

- Added `backlog/README.md` and moved six active plans into `backlog/active/`.
- Added `research/README.md`.
- Moved completed plans into `research/completed/`, deferred plans into
  `research/deferred/` and negative/blocked branches into `research/rejected/`.
- Updated `TESTING_AND_RESEARCH.md` with research progression and archive links.
- Updated workspace `AGENTS.md` to describe the unified stable release scope.
- Preserved `backlog/TASKLOG.md` at the backlog root.

### Validation and Git state

- Repository: `main`, baseline `a63c292` (`release: prepare STALKER2UltrawideFix v0.4.0`).
- Working tree contains the planned path migration and documentation changes;
  no unrelated implementation changes were observed.
- `git diff --check` passed; no runtime validation was applicable.

### Result

- Completed: active backlog and research archive are separated and indexed.
- Deferred: future curation of `research/reports/` and `research/evidence/`.
- Blocked: none.

Patch summary: reorganized 77 historical task plans without deleting research content and added navigation for active and archived work.

Changelog summary: clarified the research archive and documented the progression from gameplay writer discovery to the unified dynamic Auto policy.

## 2026-09-01 — Remove standalone cinematic FOV toggle for 0.3.1

### Scope and non-goals

- Make cinematic FOV behavior part of the selected `AspectRatio` policy.
- Migrate existing INI files by removing obsolete `Cinematics.FovCorrection`
  and its exact generated comment.
- No new RE, signature changes, gameplay algorithm changes, runtime game test,
  release upload or Git history mutation.

### Changed paths

- Updated `src/experimental_cinematic_21_9_combined_fix_204.cpp`.
- Updated public README, release notes, Nexus description and GitHub release
  body to remove the obsolete option.
- Updated generated/test and release INI files.
- Added `backlog/CINEMATIC_FOV_POLICY_CLEANUP_031_TASK_PLAN.md`.
- Rebuilt `build-artifacts/test-asi/STALKER2UltrawideFix.asi`.

### Implementation result

- `Native` now bypasses both cinematic aspect and FOV correction.
- `Auto` and forced aspect modes use their selected aspect policy for the
  existing cinematic FOV path.
- New INI files contain only `Gameplay.Enabled` and `Cinematics.AspectRatio`.
- Existing INI files are migrated in place without rewriting unrelated keys or
  comments; only the obsolete key and its exact generated comment are removed.

### Git review

- Read-only status and diff review performed; `git diff --check` passed.
- Existing staged, unstaged and untracked user changes were preserved. No Git
  staging, commit, reset, checkout or history rewrite was performed.

### Validation and limits

- Unified ASI build succeeded with the existing Visual Studio build script.
- Candidate ASI SHA-256 before the 16:9/21:9 cache correction:
  `2C58405AA9ABE6EC78B2DEB1F01B2C5DB431535AE268327FA05B8E804FD384EE`.
- Follow-up correction: preserve the cached ultrawide aspect across gameplay
  Auto restore while still accepting native 16:9 from a readable runtime
  object. Rebuild succeeded; current candidate SHA-256 is
  `6D1825FD0EE1EB8CC42D0A6D80E70A5B42788098069B40DBB4393377F246F5A5`.
- Follow-up correction: in `Auto` mode, the cinematic aspect store now uses
  the cached runtime camera aspect, matching the FOV boundary instead of
  reading the object's already-native 16:9 value after the native store.
  Rebuild succeeded; latest candidate SHA-256 is
  `C657D9EE55C1B3951441331CA8F29525DD4ACFFB5544AEDFC8DC1703A2292D82`.
- Follow-up correction: valid aspects from the gameplay/runtime camera,
  including native 16:9 after a hot resolution change, now refresh the Auto
  cache. The cinematic store remains excluded as a cache source. Build
  succeeded; follow-up candidate SHA-256 is
  `74496AD5EE4374C688410018017EFDE7A84B5151BFC1BCB65982D5728A2AD4FB`.
- Confirmed `FovCorrection` is absent from public documents and generated INI;
  source references are limited to migration and compatibility handling.
- No injected runtime test was performed in this batch. User validation of
  `Native`, `Auto` and forced framing remains required.

### Completed / remaining / deferred

- Completed: policy cleanup, INI migration implementation, documentation and
  candidate build; corrected the `Auto` 16:9 resolver so invalid zero-aspect
  values cannot reach the cinematic store; aligned the `Auto` aspect-store
  target with the runtime aspect used by cinematic FOV.
- Remaining: regression of `Native` and forced framing modes for the 0.3.1
  release decision.
- Completed: one-session hot-resolution `Auto` test across 16:9, 21:9 and 32:9
  with cinematic EXIT recovery into gameplay.
- Deferred: packaging/publishing `0.3.1`, weapon/viewmodel FOV and dynamic
  resolution behavior.

### Follow-up refinement — exclude self-authored Auto restore from cache

- Runtime evidence showed that the broad valid-aspect cache update treated the
  fix's own `Auto restore` to native 16:9 as a new runtime aspect, so later
  cinematics stayed at 16:9.
- Added source-aware suppression for that self-authored restore while keeping
  valid 16:9 from a new gameplay/runtime camera source eligible for caching.
- Rebuilt successfully; follow-up candidate SHA-256 is
  `7404D5288E8F60E925C497B055143EF97EA79798E511850DE908ABBB32376453`.
- `git diff --check` passed.

### Runtime validation — hot aspect changes

- Validated candidate SHA-256
  `7404D5288E8F60E925C497B055143EF97EA79798E511850DE908ABBB32376453` in one
  session through `16:9 → 21:9 → 32:9 → 16:9 → 21:9 → 32:9`.
- Cinematic `Auto` resolved each runtime aspect correctly: `1.77778`,
  `2.38889`, `3.55556`, then the same sequence again.
- Matching cinematic FOV values were observed: `90`, `106.688`, `126.87`.
- Each tested cinematic EXIT recovered to gameplay and re-entered the normal
  gameplay transition without a restart. User reported all tested views were
  visually correct.
- This closes the hot-resolution cache regression for the validated 2.0.4
  executable. Weapon/viewmodel FOV remains a separate known game issue.

### Policy regression — native 5120×1440 display

- Validated `Native`, forced `16:9`, forced `21:9` and forced `32:9` with the
  same candidate and game executable identity.
- `Native` correctly bypassed cinematic aspect/FOV hooks while gameplay hooks
  remained active.
- Forced cinematic modes produced `1.77778 / FOV 90`, `2.33333 / FOV 105.392`
  and `3.55556 / FOV 126.87`, respectively. The user reported all views as
  visually correct.
- Follow-up policy correction: forced `21:9` now uses canonical `3440×1440`
  aspect `2.3888889`, matching `Auto` on a real 3440×1440 display instead of
  the abstract mathematical `21/9` value.
- Rebuilt successfully; new candidate SHA-256 is
  `A763D3D275991FC2CDBC34A3CB5585FEF0A1CC13E0ECF0B2367188D123338ABF`.
- `git diff --check` passed. The previous matrix remains evidence for
  unchanged branches.
- Focused forced-`21:9` runtime validation passed: `Global cinematic ENTER`
  and `Cinematic aspect store` both used `aspect=2.38889`, with transformed
  FOV `106.688`. The user reported the view was visually correct.
- This completes the planned cinematic policy regression for the validated
  Steam 2.0.4 / UE 5.5.4 target. `Native` and forced policies are now runtime
  validated alongside the previously completed `Auto` hot-switch matrix.

### Patch summary

Removed the contradictory standalone cinematic FOV toggle and made FOV behavior
follow the selected cinematic aspect policy, with safe migration of old INIs.

### Changelog summary

Version 0.3.1 simplifies cinematic configuration: `Native` preserves native
cinematic behavior, while `Auto` and forced framing modes apply matching Hor+
FOV automatically. The 16:9 `Auto` black-screen case is fail-safe corrected.

## 2026-09-01 — Create reusable Nexus description for 0.3.0

### Scope and non-goals

- Create a reusable Nexus Mods description for `STALKER2UltrawideFix.asi`
  version `0.3.0`.
- No source, ASI, INI or release archive changes; no external Nexus upload.

### Changed paths

- Added `NEXUS_DESCRIPTION.md` with current features, configuration,
  installation, compatibility, limitations and Defender notice.
- Added `backlog/NEXUS_DESCRIPTION_0_3_0_TASK_PLAN.md` for this bounded
  documentation task.

### Git review

- Read-only Git review and `git diff --check` were performed.
- Existing staged, unstaged and untracked user changes were preserved; no Git
  staging, commit, reset, checkout or history rewrite was performed.

### Validation and limits

- Confirmed the description uses version `0.3.0`, Steam `2.0.4` and UE `5.5.4`.
- Confirmed obsolete 2.0.3/gameplay-only claims are absent except for the old
  ASI filename intentionally retained in upgrade instructions.
- The description reflects existing runtime evidence; no new build or game
  validation was performed.

### Completed / remaining / deferred

- Completed: reusable Nexus publication text.
- Remaining: copy the text into Nexus and update it there when publishing.
- Deferred: future-patch compatibility, dynamic resolution behavior and the
  separate weapon/viewmodel FOV issue.

### Patch summary

Added a maintained Nexus-ready description for the unified 0.3.0 release.

### Changelog summary

Replaced the obsolete gameplay-only publication text with current unified
gameplay, cinematic and custom framing documentation.

## 2026-09-01 — Prepare version 0.3.0 release package

### Scope and non-goals

- Prepare the user-facing `0.3.0` package for `STALKER2UltrawideFix.asi`.
- Update release documentation, include the generated INI and create a clean
  distributable archive.
- No upload to GitHub/Nexus, Git staging/commit, source changes or new runtime
  experiment was performed in this packaging task.

### Changed paths

- Updated `release-assets/README.md` for the unified gameplay/cinematic fix.
- Added `release-assets/STALKER2UltrawideFix.asi`.
- Added `release-assets/STALKER2UltrawideFix.ini`.
- Added `release-assets/STALKER2UltrawideFix-UE5.5.4-v0.3.0.zip`.
- Updated `backlog/RELEASE_0_3_0_TASK_PLAN.md` with completed batch status.

### Package contents

The archive contains exactly:

- `STALKER2UltrawideFix.asi`
- `STALKER2UltrawideFix.ini`
- `README.md`
- `LICENSE.md`
- `THIRD_PARTY_NOTICES.md`

The historical `STALKER2GameplayAspectFix-2.0.2-v0.1.0-elhait.zip` remains
untouched and is not part of the new package.

### Git review

- Read-only Git review was performed after packaging.
- Existing staged, unstaged and untracked user changes were preserved; no
  staging, commit, reset, checkout or history rewrite was performed.
- The changed release and task-log paths were compared with the approved plan;
  unrelated pre-existing worktree changes were left untouched.

### Validation and limits

- Archive creation succeeded and its exact five-file contents were verified.
- `git diff --check` passed for the reviewed worktree.
- SHA-256: ASI
  `949B61998A49FB04276D91B64BC5D3F087989999CA2779ACD8C703E97DBF7607`.
- SHA-256: INI
  `C74C4E3383A2A539FDF296AF980D2F474C03F37537AE40D7D07F1D6CEAC79C1C`.
- SHA-256: archive
  `53708362200137C95E0EC4D3B2D566CDB1596877042093185A38498F9E424F82`.
- This batch did not rebuild or inject the ASI; the package reflects the
  previously validated Steam 2.0.4 runtime evidence.

### Completed / remaining / deferred

- Completed: README update, INI inclusion, versioned archive and package
  inspection.
- Remaining: user upload/publication to GitHub/Nexus.
- Deferred: weapon/viewmodel FOV issue, dynamic resolution policy and support
  validation for future game patches.
- Not changed: historical release archive, Git history and unrelated staged
  user work.

### Patch summary

Prepared the `0.3.0` release package for the unified configurable
`STALKER2UltrawideFix.asi`, including the default INI and user-facing
installation/compatibility documentation.

### Changelog summary

Version `0.3.0` packages gameplay ultrawide correction, cinematic aspect/FOV
support and custom cinematic framing modes for the validated UE 5.5.4 / Steam
2.0.4 environment.

## 2026-09-01 — Implementation history before `STALKER2UltrawideFix`

### Scope and intent

This entry records the evidence and implementation path leading to the unified
`STALKER2UltrawideFix.asi`. It is based on the repository diff from
`HEAD 5d3d157` (`Create FUNDING.yml`), the completed task plans, Ghidra
reports and user-supplied 2.0.4 runtime logs. The working tree contains
intentional staged, unstaged and untracked user work; this entry does not
normalize or remove it.

### Diff inventory

The read-only diff against `HEAD` contains 223 changed paths:

- 48 C++ source files, including stable gameplay code, the unified source and
  separate diagnostic/research artifacts;
- 63 task plans and status documents;
- 56 test build scripts;
- 53 object/build outputs;
- documentation and supporting project files.

The large diff is a combined research/build snapshot, not a claim that every
path belongs in the release package. The production-relevant implementation
areas are `src/gameplay_aspect_fix.cpp`, the unified
`src/experimental_cinematic_21_9_combined_fix_204.cpp`, its build script and
the generated `STALKER2UltrawideFix.ini`. `src/cutscene_letterbox_fix.cpp`
and the other diagnostic sources remain research history or isolated test
artifacts.

### Work completed before the unified implementation

#### 1. Stable gameplay baseline

- Established the existing gameplay aspect correction and its two-pass
  constrained/Auto-restore contract.
- Confirmed preservation of the player's selected FOV.
- Replaced the original fixed-RVA assumption with a unique executable `.text`
  signature resolver and Zydis validation of `MOVSS [RBX+0x30], XMM0`.
- Preserved fail-closed behavior for ambiguity, decode failure, invalid
  instruction form and hook setup failure.
- Investigated the 21:9 startup/death-load regression. Runtime evidence showed
  that the old 32:9-only predicate rejected the actual 21:9 aspect
  `2.38889`.
- Generalized the predicate to finite aspects wider than native 16:9 and kept
  the observed source aspect through the constrained pass.
- User runtime-tested 21:9 startup, `21:9 → 16:9 → 21:9`, death/load rebuild
  and 32:9 regression successfully.

#### 2. Early cinematic and FOV research

- Reconstructed the legacy 2.0.3 cinematic transition and letterbox/FOV
  boundaries, including the shared ENTER/EXIT topology.
- Tested and rejected durable `+0x230`/state-based, delayed, timer, polling,
  generic dispatcher, interpolation-shape and broad renderer approaches.
- Used read-only runtime correlation to reject unrelated static candidates,
  including `FUN_1422FC35A`, `FUN_1405EDA3A`, `FUN_1431FA182`,
  `FUN_14027A5E4`, `FUN_146880C06`, `FUN_1404A4CCE`, `FUN_142D08BB0` and
  `FUN_1476C41A6`.
- Closed the scalar-shape ranking method after demonstrating that structural
  FP similarity did not establish live cinematic FOV provenance.

#### 3. Current-build live FOV recovery

- Mapped legacy transition hub `FUN_142EE14BC` to the strong 2.0.4 descendant
  `FUN_142EE68DA` through shared helpers, resolver topology and exact
  direction-specific vtable slot pairs.
- Confirmed the current ENTER boundary where authored `XMM0=90.0` reaches the
  shared live-FOV consumer and the EXIT boundary where `[RDI+0x38]` carries the
  native gameplay target.
- Runtime evidence confirmed both source patterns on the same transition
  context and thread.
- A read-only feasibility test transformed ENTER FOV `90.0` to
  `126.869896` for 32:9 and produced the expected wider cinematic result,
  without durable camera-state writes.

#### 4. Current-build aspect recovery

- Reconstructed the legacy aspect/letterbox role and identified the exact
  current ENTER native store equivalent of `RVA 0x6B7CB05`.
- A read-only provenance hook confirmed the store executes with `RAX` equal to
  the authoritative cinematic inner and the expected pre-store lifecycle
  flags.
- Immediate-patch feasibility replaced only the native `1.7777778` immediate
  with runtime `3.5555556` at 5120x1440, preserving native control flow and
  lifecycle side effects.
- The aspect source was corrected from desktop dimensions to the runtime game
  camera aspect after discovering that a 5120x1440 desktop can contain a
  3440x1440 game window.

#### 5. Combined cinematic integration research

- Combined the already validated aspect immediate and transient live-FOV
  boundaries with the generalized gameplay correction in isolated artifacts.
- User runtime-tested correct 21:9 and 32:9 cinematic framing, including
  2560x1440 with forced 32:9 letterbox framing.
- Investigated the post-EXIT seam. Native FOV recovery and gameplay B/C were
  both confirmed necessary; atomic same-invocation B/C scheduling did not
  remove the visible intermediate presentation state. That coordinator path
  was closed as a production solution and is not presented as solved.
- Kept the native EXIT recovery, gameplay algorithm and stable gameplay source
  contract intact.

#### 6. Unified configuration and identity

- Added automatic creation of `STALKER2UltrawideFix.ini`.
- Added independent gameplay enablement and cinematic policy configuration.
- Replaced cinematic boolean aspect control with `AspectRatio=Auto`,
  `Native`, `16:9`, `21:9` or `32:9`, while retaining independent
  `FovCorrection`.
- Preserved legacy `AspectFix`/`FovFix` parsing as compatibility fallback.
- Added startup SHA-256 logging for the loaded ASI and game executable.
- Corrected the file reader after the first `unavailable` result; a subsequent
  runtime log confirmed uppercase hashes and successful resolver installation.
- Removed high-frequency coordinator recovery/suppression spam while keeping
  state behavior unchanged.

### Resulting implementation state

The unified artifact now contains:

- signature-resolved gameplay camera correction;
- runtime-camera cinematic aspect policy and native immediate replacement;
- signature-resolved cinematic ENTER/EXIT live-FOV transform;
- coordinator isolation between cinematic lifecycle and gameplay replay;
- automatic INI creation and policy parsing;
- startup mod/game SHA-256 identity logging;
- fail-closed resolver and hook setup behavior.

### Validation and limits

- Build validation succeeded for the unified ASI and its test output.
- User runtime evidence confirms the unified 2.0.4 path at 21:9 and 32:9,
  including forced cinematic framing modes and gameplay re-entry behavior.
- The tested Steam game executable identity was
  `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`.
- Signature resolution is more resilient than fixed RVAs but does not itself
  guarantee a future patch. No 2.0.5 support claim is made.
- The weapon/viewmodel FOV-after-load issue remains a separate known game-side
  issue.
- The brief post-cinematic presentation seam remains a known limitation of the
  experimental cinematic integration.

### Completed / remaining / deferred

- Completed: evidence-led gameplay correction, cinematic boundary recovery,
  unified 2.0.4 implementation, configuration, identity logging and supplied
  runtime validation.
- Remaining: final release packaging and user-facing release documentation.
- Deferred: weapon/viewmodel FOV research, dynamic resolution policy and fresh
  validation for any executable newer than 2.0.4.
- Not changed by this documentation record: source behavior, build outputs,
  Git staging and commit history.

### Patch summary

The unified ASI was built only after separate gameplay and cinematic boundary
reconstruction, runtime validation, rejection of unsafe alternatives and
resolution of the 21:9 lifecycle regression. The final implementation combines
the validated mechanisms without the earlier desktop-aspect or fixed-RVA
assumptions.

### Changelog summary

Documented the complete path from the stable gameplay resolver and bounded
cinematic research to the configurable `STALKER2UltrawideFix` implementation,
including evidence limits and deferred issues.

## 2026-09-01 — Reconcile completed backlog and testing summary

### Scope

- Review every task plan in `backlog`.
- Move only completed or explicitly closed plans into `backlog/complete/`.
- Update `TESTING_AND_RESEARCH.md` with the confirmed 2.0.4 gameplay,
  cinematic, configuration and compatibility status.
- No source, build, ASI, INI or runtime behavior changes.

### Changed

- Moved 32 completed/closed task plans to `backlog/complete/`.
- Added `backlog/COMPLETE_BACKLOG_AND_TESTING_SUMMARY_TASK_PLAN.md`.
- Reworked `TESTING_AND_RESEARCH.md` to document the unified
  `STALKER2UltrawideFix.asi`, its policy configuration, 2.0.4 evidence,
  signature limits, known issues and release checklist.
- Preserved active, pending, deferred and unresolved plans in `backlog/`.

### Git review

- Read-only status review performed after the archival move.
- Existing staged and untracked user changes were preserved; no Git staging,
  commit, reset or history rewrite was performed.
- The moved plans appear as staged/deleted plus untracked archived copies where
  the worktree already contained user-staged additions; this was not altered.

### Validation and limits

- Confirmed 32 exact source plans moved without destination collisions.
- Confirmed 41 task plans now exist in `backlog/complete/` and 31 remain as
  active or deferred plans in `backlog/`.
- Checked the updated summary against supplied runtime evidence and existing
  task plans.
- `git diff --check` passed for the repository worktree.
- No build, game launch, ASI injection or new runtime validation was performed.

### Completed / remaining / deferred

- Completed: backlog classification, archival of completed/closed plans and
  testing/research summary update.
- Remaining: active plans in `backlog/`, including unresolved projection,
  pending signature follow-up, post-EXIT handoff classification and logging
  cleanup runtime confirmation.
- Deferred: weapon/viewmodel FOV research and future-patch compatibility.
- Not runtime-validated by this documentation batch: no new implementation or
  runtime behavior.

### Patch summary

Archived completed research and implementation plans and synchronized the
project testing summary with the validated 2.0.4 state.

### Changelog summary

The backlog now separates completed/closed work from active research, and the
testing summary documents the unified configurable gameplay/cinematic fix.

## 2026-08-30 — Runtime-informed top-tier semantic reranking

### Scope

- Re-rank the retained 2.0.4 top-500 using completed runtime negatives.
- Decompile-review the tied top tier and the `FUN_14569...` family.
- No runtime artifact, game launch, hook, write or stable/global ASI change.

### Changed

- Added the reranking plan, helper and generated report.
- Added a read-only Ghidra decompile helper and top-tier review report.

### Git review

- Existing uncommitted research/source/build-artifact changes were preserved.
- Stable gameplay and global experimental ASI files were not modified.
- Recent HEAD at review: `5d3d157 Create FUNDING.yml`.

### Validation and limits

- Existing 2.0.4 inventory was reused; no full executable re-analysis was run.
- Top-500 input was deduplicated to `351` function/entry rows.
- Top-tier decompilation ran read-only against the verified 2.0.4 program hash.
- Static review does not establish runtime ownership.

### Completed

- Initial shortlist was closed and runtime-informed ranking completed.
- Most tied candidates were deprioritized as subsystem-specific, large/generic
  or lacking lifecycle semantics.
- Held shortlist: `FUN_14027A5E4`, `FUN_146880C06`, and the `FUN_14569...`
  family represented by `FUN_14569CAD2`.

### Remaining / deferred

- No candidate is runtime-promoted; each held candidate requires a separate
  read-only plan before testing.
- Native cinematic FOV transition owner remains unresolved.

### Patch summary

Applied runtime-informed penalties and completed semantic review of the tied
top tier without expanding into runtime instrumentation.

### Changelog summary

Initial static-ranked shortlist was replaced by a smaller held shortlist based
on subsystem and lifecycle plausibility; no production code changed.

## 2026-08-30 — `FUN_1431FA182` runtime correlation gate

### Scope

- Run one isolated, read-only 2.0.4 runtime correlation pass for
  `FUN_1431FA182`.
- No game-state writes, correction logic, stable gameplay changes or global
  experimental ASI changes.

### Changed

- Added the candidate-specific task plan and runtime evidence review under
  `02-Research/Ghidra/reports`.

### Git review

- Existing uncommitted research/source/build-artifact changes were preserved.
- Stable gameplay and global experimental ASI files were not modified.
- Recent HEAD at review: `5d3d157 Create FUNDING.yml`.

### Validation and limits

- User-supplied runtime log shows successful installation and valid ENTER/EXIT
  anchors on the same `inner=0x27B7E02A6C0` and thread `19768`.
- Candidate hit count was zero over approximately `12.4 s`; no write-test was
  performed.

### Completed

- `FUN_1431FA182` was rejected for the tested cinematic lifecycle.
- The initial three-candidate runtime shortlist is exhausted.

### Remaining / deferred

- Native cinematic FOV transition owner remains unresolved.
- Runtime-informed reranking of the top-500 requires a separate plan.

### Patch summary

Recorded a clean read-only negative runtime verdict for `FUN_1431FA182`.

### Changelog summary

`FUN_1431FA182` was silent during the validated cinematic lifecycle and was
rejected without modifying production or correction code.

## 2026-08-30 — `FUN_1405EDA3A` runtime correlation gate

### Scope

- Run one isolated, read-only 2.0.4 runtime correlation pass for the static
  reserve candidate `FUN_1405EDA3A`.
- No game-state writes, correction logic, stable gameplay changes or global
  experimental ASI changes.

### Changed

- Added the candidate-specific task plan and runtime evidence review under
  `02-Research/Ghidra/reports`.

### Git review

- Existing uncommitted research/source/build-artifact changes were preserved.
- Stable gameplay and global experimental ASI files were not modified by this
  pass.
- Recent HEAD at review: `5d3d157 Create FUNDING.yml`.

### Validation and limits

- User-supplied runtime log shows successful installation and valid ENTER/EXIT
  anchors on the same `inner=0x2D701A50100` and thread `7188`.
- Candidate hit count was zero over approximately `11.55 s`; no write-test was
  performed.

### Completed

- `FUN_1405EDA3A` was rejected for the tested cinematic lifecycle.

### Remaining / deferred

- `FUN_1431FA182` remains the next reserve candidate, pending a separate plan.
- Native cinematic FOV transition owner remains unresolved.

### Patch summary

Recorded a clean read-only negative runtime verdict for `FUN_1405EDA3A`.

### Changelog summary

`FUN_1405EDA3A` was silent during the validated cinematic lifecycle and was
rejected without modifying production or correction code.

## 2026-08-27 — Close cinematic constrained-vs-native projection state diff research

### Scope

- Analyze the 2.0.3 Ghidra project on the durable E: path.
- Compare the confirmed `16:9 / 2560x1440 / correct framing` and `32:9 / 5120x1440 / incorrect framing` cinematic states.
- Validate only `FUN_140186BE8` as the remaining projection-like candidate.
- No stable gameplay changes, A/B changes, runtime tracer, build or source implementation.

### Changed

- Added and updated `backlog/CINEMATIC_CONSTRAINED_NATIVE_PROJECTION_STATE_DIFF_TASK_PLAN.md`.
- Added read-only Ghidra helpers under `02-Research/Ghidra/ghidra-scripts` and stored analysis logs under `02-Research/Ghidra/workspace`.
- No Ghidra project data or stable ASI source was intentionally modified.

### Git review

- Canonical repository: project root.
- Branch/HEAD: `main@179222d` (`chore: organize build artifacts and research workflow`).
- Read-only Git review performed with an explicit per-command `safe.directory` override; global Git configuration was not changed.
- Existing user modifications and untracked research/build artifacts remain untouched.

### Validation and limits

- Executable: game 2.0.3, SHA-256 `81961B7281C7CF528CE49C549CE086FCC684BD676F32FAF042BC743D939E3C69`.
- `FUN_140280F58`, `FUN_140311CB4` and `FUN_14038E760` were rejected as graph/lighting/resource paths.
- `FUN_1401BD890` consumes mode/flag bytes but does not read the runtime aspect field.
- `FUN_14010C076` is a constructor/default initializer, not a transition owner.
- `FUN_140186BE8` builds matrix-like data, but caller/data-flow evidence ties it to movable-point-light/shadow and `ViewSpace...` scene resources, not cinematic camera ownership.
- No runtime validation or implementation was performed; this is a bounded static negative result.

### Completed / remaining / deferred

- Completed: Batch 1 state-family diff and the approved single-candidate ownership validation.
- Remaining: none within the approved bounded scope.
- Deferred: any new cinematic research requires new structural evidence and a separate approved plan.
- Blocked: no validated cinematic projection-only or refresh intervention point was found.
- Not runtime-validated: all conclusions in this entry are static-analysis conclusions.

### Patch summary

Completed a bounded 2.0.3 Ghidra ownership review and rejected the remaining projection-like candidate as unrelated scene/shadow processing.

### Changelog summary

Closed the constrained-vs-native cinematic projection research task as blocked without changing the stable gameplay fix.

## 2026-08-27 — Reject transient/durable FOV split experiment

### Scope

- Approved experimental A/B test of transient cinematic Hor+ FOV versus durable camera-state FOV.
- Determine whether cinematic framing can remain correct when `state + 0x54` receives the original source FOV.
- Stable gameplay ASI and release behavior were out of scope.

### Changed

- Added a diagnostic-only split-state branch and dedicated build script under the existing experimental workflow.
- Built Variant A and Variant B diagnostic ASIs for manual comparison.
- Updated `backlog/CUTSCENE_TRANSIENT_DURABLE_FOV_SPLIT_TASK_PLAN.md` with the completed result.

### Git review

- Canonical repository: project root.
- Branch/HEAD: `main@179222d` (`chore: organize build artifacts and research workflow`).
- Read-only review performed; no Git state was mutated.
- Existing diagnostic source, object files and untracked research plans remain visible in the working tree.

### Validation and limits

- Both diagnostic variants compiled successfully.
- Variant B confirmed transient `126.869896` with durable `state + 0x54 = 90`.
- Gameplay logging after the cutscene recorded `primaryFOV=90`, not `126.869896`.
- Manual testing showed that Variant B visually resembled gameplay without `STALKER2GameplayAspectFix`: the large gameplay-visible FOV contamination was removed, but the required cinematic Hor+ framing was lost.
- This does not identify the exact final projection consumer or provide a release-ready solution.

### Completed / remaining / deferred

- Completed: transient/durable split mechanism tested and rejected as final architecture.
- Remaining: find a solution that preserves cinematic Hor+ without injecting the transformed value into gameplay-visible camera state.
- Deferred: further implementation and projection-owner research until a new concrete direction is approved.
- Not runtime-validated: no release build or compatibility claim was made.

### Patch summary

Tested and rejected a split-state FOV design: it prevents the `126.87` gameplay leak but loses the required cinematic Hor+ framing.

### Changelog summary

Closed the split-state experiment with causal evidence that the transformed FOV must pass through camera-state machinery for the observed cinematic framing.

## 2026-08-27 — Reject cutscene FOV durable state as causal blend owner

### Scope

- Approved bounded static/runtime research of the established cutscene FOV state path.
- Determine whether `state + 0x54` is consumed as an active post-cutscene blend or projection source.
- No source behavior, ASI output, workaround, delayed replay or broad scan changes.

### Changed

- Updated `backlog/CUTSCENE_FOV_STATE_RUNTIME_TRACE_TASK_PLAN.md` with the final ownership-bounded result.
- Recorded that `FUN_1431D2094` reads `state + 0x54` only for equality/update checking.
- Recorded that the runtime-derived virtual target at executable RVA `0x20939B8` does not read `+0x54`.

### Git review

- Canonical repository: project root.
- Branch/HEAD: `main@179222d` (`chore: organize build artifacts and research workflow`).
- Read-only review performed; no Git state was mutated.
- Existing source, object files and untracked research plans remain outside this documentation batch.

### Validation and limits

- Re-ran the bounded read-only Ghidra inspection against the 2.0.3 program in `Dump/STALKER2-Ghidra`.
- Confirmed the durable `+0x54` write and the existing marker-correlated runtime state handoff evidence.
- Confirmed no downstream blend/projection reader was established in the known state family.
- This result does not identify the final camera handoff owner and does not validate a new implementation.

### Completed / remaining / deferred

- Completed: `state + 0x54` durable-state path rejected as a validated causal blend owner.
- Remaining: determine a safer architecture for preventing transformed cinematic FOV from affecting the subsequent camera handoff.
- Deferred: further blend-owner search, implementation and compatibility claims.
- Not runtime-validated: no new implementation runtime test was performed in this batch.

### Patch summary

Closed the bounded cutscene FOV state investigation after confirming durable state handoff but finding no active downstream blend consumer.

### Changelog summary

Rejected the `state + 0x54` persistence path as an implementation basis; future work must use a separate, evidence-backed camera handoff design.


## 2026-08-26 — Trim vendored dependency material and add notices

### Scope

- Approved cleanup of vendored dependency contents and third-party notices.
- Keep the dependencies vendored; no submodule migration.
- Preserve current source/build behavior and release assets.

### Changed

- Retained `external/spdlog/include/` and `external/spdlog/LICENSE`.
- Removed unused spdlog examples, tests, benchmarks, CMake/CI files, scripts, logos and metadata outside the retained paths.
- Preserved `external/safetyhook/` and bundled Zydis unchanged.
- Added SafetyHook BSL-1.0, Zydis MIT and spdlog MIT provenance/license notes to `THIRD_PARTY_NOTICES.md`.
- Closed `backlog/complete/DEPENDENCIES_CLEANUP_TASK_PLAN.md`.

### Git review

- Canonical repository: project root.
- Branch/HEAD: `main@f8b6603` (`Add files via upload`).
- Read-only review performed; no Git state was mutated.
- `release-assets/` and C++ source were untouched.

### Validation and limits

- Representative `build-transition-trace.cmd` completed successfully from `build-artifacts/test-scripts/`.
- Confirmed retained spdlog headers were sufficient for compilation and output remained in the test artifact folders.
- No game launch, ASI injection or runtime validation was performed.

### Completed / remaining / deferred

- Completed: dependency trim and licensing/provenance documentation.
- Remaining: none for this cleanup batch.
- Deferred: optional future submodule migration remains a separate task.

### Patch summary

Reduced vendored spdlog to build-required headers and license text while documenting all retained third-party dependencies.

### Changelog summary

Removed unused dependency project material and added clear SafetyHook, Zydis and spdlog notices.

## 2026-08-26 — Update testing and research summary

### Scope

- Approved documentation-only update based on completed task plans and established runtime evidence.
- Correct stale fixed-RVA/source claims and document current stable, experimental and deferred research status.
- No source, build script, release asset or runtime behavior changes.

### Changed

- Rewrote `TESTING_AND_RESEARCH.md` to document the dynamic gameplay resolver, 5120×1440 runtime observations, failed letterbox validation and rejected gameplay-camera state branch.
- Preserved the distinction between confirmed evidence, rejected paths, unresolved weapon/viewmodel ownership and executable/session limits.
- Added and closed `backlog/complete/TESTING_AND_RESEARCH_TASK_PLAN.md`.

### Git review

- Canonical repository: project root.
- Branch/HEAD: `main@f8b6603` (`Add files via upload`).
- Only the approved documentation and task-log paths were changed in this batch; no Git state was mutated.
- `git diff --check` passed.

### Validation and limits

- Checked the updated summary against the completed plans in `backlog/complete/`.
- Confirmed obsolete `RVA 0x00AF3A17` and fixed-RVA framing were removed.
- No build, game launch, ASI injection or new runtime validation was performed.

### Completed / remaining / deferred

- Completed: testing/research summary aligned with current evidence.
- Remaining: none for this documentation batch.
- Deferred: separate research task for the weapon/viewmodel object or downstream projection owner.

### Patch summary

Replaced stale testing notes with an evidence-based summary of the dynamic gameplay resolver, runtime state-diff result and experimental letterbox status.

### Changelog summary

Updated testing and research documentation to reflect current validated behavior and clearly bounded unresolved work.

## 2026-08-26 — Organize test build scripts and diagnostic ASI outputs

### Scope

- Approved cleanup of root test build scripts and diagnostic `.asi` files.
- Keep only the main `build.cmd` in the project root.
- Preserve release assets and leave `TESTING_AND_RESEARCH.md` unchanged.

### Changed

- Moved six diagnostic `build-*.cmd` scripts to `build-artifacts/test-scripts/`.
- Moved root diagnostic ASI files to `build-artifacts/test-asi/`.
- Updated moved test scripts to resolve source/dependency paths from the project root and write `.obj` files to `build-artifacts/obj/` and `.asi` files to `build-artifacts/test-asi/`.
- Kept the existing global `*.asi` ignore rule; no redundant folder-specific rule was added.
- Closed `backlog/TEST_BUILD_ARTIFACTS_TASK_PLAN.md` with the verified outcome.

### Git review

- Canonical repository: project root.
- Branch/HEAD: `main@f8b6603` (`Add files via upload`).
- Read-only review confirmed only `build.cmd` and no `.asi` remain in the project root; `release-assets/` was untouched.
- `git diff --check` passed; no Git state was mutated.
- Pre-existing staged diagnostic sources, dependencies and related workspace changes remain outside this cleanup scope.

### Validation and limits

- `build-transition-trace.cmd` succeeded from its new location.
- Verified `.obj` output in `build-artifacts/obj/` and `.asi` output in `build-artifacts/test-asi/`.
- No game launch, ASI injection or runtime validation was performed.

### Completed / remaining / deferred

- Completed: test script and diagnostic ASI organization with path-preserving build correction.
- Remaining: none for this cleanup batch.
- Deferred: release packaging and stable `build.cmd` output policy remain separate tasks.

### Patch summary

Separated diagnostic build scripts and ASI outputs from the project root while preserving their build paths and the existing global ignore policy.

### Changelog summary

Kept the project root focused on the main build entry point and moved test artifacts into dedicated directories.

## 2026-08-26 — Organize task plans and root build artifacts

### Scope

- Approved cleanup of root-level `.obj` files and project task plans.
- Preserve all artifacts and plan contents; do not change source behavior, release assets or Git history.
- Keep `TESTING_AND_RESEARCH.md` unchanged for a later documentation task based on `backlog/complete`.

### Changed

- Moved eight root `.obj` files to `build-artifacts/obj/`.
- Removed the project-local `*.obj` rule from `.gitignore`.
- Moved four completed plans to `backlog/complete/`.
- Added English status lines and closure reasons to the completed plans.
- Added `backlog/BACKLOG_ORGANIZATION_TASK_PLAN.md` with status `Closed` and a closure reason.
- Updated direct references affected by plan moves.

### Git review

- Canonical repository: project root.
- Branch/HEAD: `main@f8b6603` (`Add files via upload`).
- Read-only review confirmed no root-level `.obj` or `*_TASK_PLAN.md` remains.
- `git diff --check` passed; no Git state was mutated.
- Pre-existing staged diagnostic sources, dependencies and related workspace changes remain outside this cleanup scope.

### Validation and limits

- Confirmed all eight `.obj` files exist in `build-artifacts/obj`.
- Confirmed all four completed plans exist in `backlog/complete` and the cleanup plan exists in `backlog`.
- Confirmed `.gitignore` no longer ignores `.obj`.
- No build or runtime validation was required or performed.

### Completed / remaining / deferred

- Completed: bounded artifact and task-plan organization.
- Remaining: none for this cleanup batch.
- Deferred: update `TESTING_AND_RESEARCH.md` from completed plans as a separate documentation task.

### Patch summary

Moved reusable object files and completed task plans into dedicated folders, normalized plan statuses and removed the obsolete `.obj` ignore rule.

### Changelog summary

Improved project navigation by separating build artifacts from source and maintaining a backlog with a completed-plan archive.

## 2026-08-26 — Reject gameplay-camera state as weapon FOV causal owner

### Scope

- Approved research-only ADS runtime state-diff and marker validation.
- Compare the bounded 12-field gameplay-camera context across cutscene exit, ADS and pause transitions.
- No stable gameplay logic, experimental letterbox logic, memory writes or game-function calls.

### Changed

- Updated `src/gameplay_transition_trace.cpp` with five manual markers: F8 cutscene exit, F9/F10 ADS enter/exit and F11/F12 pause open/close.
- Updated `backlog/complete/ADS_STATE_DIFF_TASK_PLAN.md` with the completed batch result and next phase gate.
- Built `STALKER2GameplayTransitionTrace.asi`; runtime log was written outside the repository in the game's Win64 directory.

### Git review

- Canonical repository: project root.
- Branch/HEAD: `main@f8b6603` (`Add files via upload`).
- Working tree contains pre-existing diagnostic sources, plans and vendored dependencies; this batch changed only the tracer source and its task plan.
- No Git state was mutated.

### Validation and limits

- Build succeeded and produced `STALKER2GameplayTransitionTrace.asi`.
- Runtime log loaded the tracer and recorded all five markers: sequences 1903, 2119, 2425, 2526 and 2557.
- Across the marker windows, the 12 sampled gameplay-camera values showed no distinct state pattern associated with ADS or pause correction; `outputFov == inputFov` throughout.
- Runtime evidence is limited to the tested executable/session and does not identify the weapon/viewmodel owner.

### Completed / remaining / deferred

- Completed: gameplay-camera state branch rejected as the causal owner by marker-correlated runtime evidence.
- Remaining: find the separate weapon/viewmodel object or downstream projection context.
- Deferred: ownership tracing and any implementation; no compatibility claim is made.
- Not runtime-validated: no new letterbox or stable gameplay implementation was tested in this batch.

### Patch summary

Added precise manual event markers to the bounded runtime sampler and used them to exclude the known gameplay-camera context from the weapon/viewmodel FOV causal path.

### Changelog summary

Closed the gameplay-camera state investigation with a reproducible negative runtime result; future research starts from a separate weapon/viewmodel or projection context.

## 2026-08-25 — Remove obsolete publishing documents

### Scope

- Approved cleanup of two obsolete project-local publishing documents.
- No source, runtime, release archive, `HANDOFF.md`, `AGENTS.md` or Git history changes.

### Changed

- Removed `PUBLISHING_CHECKLIST.md`.
- Removed `NEXUS_DESCRIPTION.md`.

### Git review

- Canonical repository: project root.
- Branch: `main`, still tracking `origin/main`.
- Cleanup appears as two expected deleted paths.
- The pre-existing `RELEASE_NOTES.md` documentation change remains separate and was not removed.

### Validation and limits

- Confirmed both exact target paths no longer exist.
- `git diff --check` passed; only a normal LF-to-CRLF warning was reported.
- No build, game launch, ASI load or runtime validation performed.

### Completed / remaining

- Completed: obsolete publishing and Nexus description files removed.
- Remaining: none for this cleanup batch.

### Patch summary

Removed two unused project-local publishing documents while preserving source and release assets.

### Changelog summary

Cleaned obsolete publishing metadata from the canonical project tree.

## 2026-08-25 — Align documentation contract with canonical v0.1.1 source

### Scope

- Documentation-only follow-up to Task 1 static/source verification.
- Align the stable contract and project/release documentation with canonical `main@dad8176`.
- Keep `HANDOFF.md` as historical context; do not alter source, resolver code, release assets or Git refs.

### Changed

- Updated `AGENTS.md` to describe the implemented `.text` signature resolver and its decode/safe-refusal contract.
- Corrected project release documentation from UE 5.5.6 to the supported UE 5.5.4 wording.
- Updated `PUBLISHING_CHECKLIST.md` for v0.1.1 and marked the retained v0.1.0 archive as historical.

### Git review

- Canonical repository: project root.
- Branch/commit before documentation changes: `main@dad8176`.
- Working tree was clean before this batch; source and Git refs were not changed.

### Validation and limits

- Confirmed resolver provenance in canonical `src/gameplay_aspect_fix.cpp`.
- Checked documentation references for the obsolete fixed-RVA/v0.1.0 contract and UE 5.5.6 claims.
- No build, ASI load, game launch or runtime validation performed.

### Completed

- Documentation contract now matches the current resolver source at the stated Git identity.
- Task 1 remains complete at static/source evidence level only.

### Remaining / deferred / blocked

- Runtime operation and compatibility across game builds remain unvalidated.
- The retained release archive remains historical v0.1.0 packaging.

### Patch summary

Aligned stable and release documentation with the canonical v0.1.1 dynamic resolver without changing implementation or runtime state.

### Changelog summary

Corrected resolver provenance, UE version wording and v0.1.1 publishing guidance.

## 2026-08-25 — Restore post-change Git review workflow

### Scope

- Restore the rule that every approved change is followed by a read-only Git review and an implementation-vs-plan comparison.
- Define a stable task-log format for future patch and changelog writing.

### Changed

- Updated `AGENTS.md` with the post-change Git review and task-log workflow.
- Added this root `TASKLOG.md` as the workspace task-log location.

### Git review

- Canonical repository: project root.
- Branch: `main`.
- Git state: existing user Markdown changes and handoff history remain uncommitted; no Git state was mutated for this task.
- Source, build outputs and release assets were not changed.

### Validation and limits

- Confirmed the new rules are present in `AGENTS.md`.
- This task did not build the ASI, launch the game or perform runtime hook validation.

### Completed

- Post-change `git status`/diff review is now mandatory before task closure.
- Future entries must distinguish completed, remaining, deferred, blocked and not-runtime-validated work.

### Remaining / deferred

- No source implementation work was requested in this task.
- The canonical source remains subject to the version and resolver limitations documented in `AGENTS.md` and `HANDOFF.md`.

### Patch summary

Restored a factual post-change Git audit and task-log workflow for the native C++/ASI workspace.

### Changelog summary

Added structured task logging for reliable patch and release-note preparation.

## 2026-08-30 — `FUN_1422FC35A` runtime correlation gate

### Scope

- Run one isolated, read-only 2.0.4 runtime correlation pass for the highest-
  ranked exploratory static candidate `FUN_1422FC35A`.
- No game-state writes, correction logic, stable gameplay changes or global
  experimental ASI changes.

### Changed

- Added the candidate-specific task plan and runtime evidence review under
  `02-Research/Ghidra/reports`.

### Git review

- Canonical repository contains pre-existing uncommitted research/source and
  build-artifact changes; they were preserved.
- This pass did not modify stable gameplay or global experimental ASI files.
- Recent HEAD at review: `5d3d157 Create FUNDING.yml`.

### Validation and limits

- User-supplied runtime log corresponds to the recorded 2.0.4 executable hash
  `2ecc5d19fe37f97e3f7f2467d652b299b5a47f010fa49fd803a49a4a6930a409`.
- The run produced `2782` hits across three candidate objects, all on the
  ENTER thread, but candidate scalar state did not track FOV convergence.
- No write-test was performed.

### Completed

- `FUN_1422FC35A` was tested under the approved runtime gate and rejected as
  the native cinematic FOV owner.

### Remaining / deferred

- Secondary static candidates remain held; no automatic promotion is allowed.
- Native FOV transition owner remains unresolved.

### Patch summary

Recorded the read-only runtime correlation result for the top static candidate
without changing stable or global fix behavior.

### Changelog summary

Runtime evidence rejects `FUN_1422FC35A` as the current-build cinematic FOV
owner; no production or correction code was changed.

## 2026-08-25 — Restore adapted workflow and model routing modules

### Scope

- Re-analyze the previously removed workflow documentation.
- Restore useful process rules under native C++/ASI/Ghidra realities.
- Preserve the removal of unrelated Messenger/web/Prisma guidance.

### Changed

- Added engineering discipline, task-plan authority, contextual module registry and `GPT-5.6 Luna`/`Terra`/`Sol` routing to `AGENTS.md`.
- Added adapted modules under `docs/`: code style, architecture, implementation, testing, documentation and model routing.
- Updated root `HANDOFF.md` to reference the adapted workflow modules.

### Git review

- Workspace-level documentation changed; canonical source files and Git history were not changed.
- Existing canonical repository user Markdown state remains uncommitted.

### Validation and limits

- Confirmed every registered module exists and is referenced by `AGENTS.md`.
- Confirmed model routing distinguishes bounded execution, architecture/coordination and safety/contradiction escalation.
- No build, game launch, injected ASI test or runtime hook validation was performed.

### Completed

- Useful workflow rules from the prior documentation are restored in project-specific form.
- Unrelated web-project rules remain excluded.

### Remaining / deferred

- No source implementation or resolver recovery was requested.

### Patch summary

Restored evidence-driven phase gates, contextual engineering modules and Sol/Terra/Luna model routing for the native reverse-engineering workspace.

### Changelog summary

Reintroduced project-specific engineering workflow and model escalation guidance without restoring unrelated web-stack documentation.

## 2026-08-25 — Clarify plan fields, contradictory evidence and task-log timing

### Scope

- Apply the three optional workflow clarifications from the documentation review.
- Keep model routing and the stable/research boundaries unchanged.

### Changed

- Added the canonical Required Task Plan Fields checklist to `AGENTS.md` and the implementation guidelines.
- Defined contradictory evidence as evidence that invalidates an accepted contract, ownership conclusion, executable identity or safety assumption.
- Separated per-batch Git review from final task-level `TASKLOG.md` entry timing.
- Added the same timing distinction to the documentation guidelines.

### Validation and limits

- Confirmed all three rules appear in the relevant modules.
- No source, build, release asset, runtime hook or Git state was changed.

### Completed

- Plans now have a stable checklist for scope, evidence, batches, validation, risks, gates and final Git review.
- Consistent evidence does not reopen a phase; contract-invalidating evidence does.
- Batch reviews can be recorded without fragmenting one task into multiple final task-log entries.

### Remaining / deferred

- No implementation or runtime validation was requested.

### Patch summary

Formalized task-plan fields, contradiction handling and batch-versus-task task-log timing.

### Changelog summary

Clarified agent workflow gates and task-log semantics for reliable patch preparation.
## 2026-08-31 — Reject `FUN_1476C41A6` as legacy live-FOV equivalent

### Scope

- Complete the planned read-only micro-review of current-build candidate `FUN_1476C41A6`.
- Do not build or run a runtime artifact; do not modify stable/global ASI code.

### Changed

- Added the single-function Ghidra inspection script under `02-Research/Ghidra/ghidra-scripts`.
- Added `legacy-live-fov1476c41a6-micro-review-204.md` with the instruction-level verdict.
- Marked Batch 7 complete in `LEGACY_LIVE_FOV_CONSUMPTION_RECOVERY_204_TASK_PLAN.md`.

### Validation and limits

- Reused the existing 2.0.4 Ghidra program read-only with `-noanalysis`.
- Verified executable SHA-256: `2ecc5d19fe37f97e3f7f2467d652b299b5a47f010fa49fd803a49a4a6930a409`.
- Confirmed the ranked scalar slice is sensitivity-settings/UI data, not cinematic FOV.
- No build, game launch, injected ASI test or runtime hook validation was performed.

### Completed

- `FUN_1476C41A6` rejected as a current-build equivalent of the legacy live cinematic FOV consumption boundary.
- No runtime promotion or write-test justified.

### Remaining / deferred

- Native current-build live cinematic FOV consumption boundary remains unresolved.
- Remaining ranked/family candidates require a separate decision; no automatic runtime promotion was made.

### Patch summary

Completed the final planned micro-review candidate and recorded its settings/UI scalar lineage.

### Changelog summary

Rejected `FUN_1476C41A6` as unrelated sensitivity-settings code; stable gameplay and experimental global ASI remained untouched.
## 2026-08-31 — Reconstruct legacy live-FOV callgraph boundary

### Scope

- Complete Batch 1 of the separate cross-version legacy callgraph/data-flow
  reconstruction plan.
- Use existing Ghidra programs read-only; no runtime artifact or production
  source changes.

### Changed

- Added `legacy-fov-callgraph-boundary-203.md` with the exact 2.0.3
  ENTER/EXIT scalar and callgraph neighbourhood.
- Added `InspectLegacyWorkingFovBoundary203.java` for reproducible read-only
  inspection.
- Marked Batch 1 complete in `LEGACY_FOV_CALLGRAPH_DATAFLOW_RECONSTRUCTION_204_TASK_PLAN.md`.

### Validation and limits

- Used the existing 2.0.3 Ghidra program with `-noanalysis -readOnly`.
- Verified executable SHA-256:
  `81961b7281c7cf528ce49c549ce086fcc684bd676f32faf042bc743d939e3c69`.
- Confirmed ENTER `MOVSS XMM0,[0x149EDE50C]` → `FUN_146B68976` and the EXIT
  counterpart `MOVSS XMM0,[RDI+0x38]` → the same callee.
- No current-build descendant was claimed; no build, game launch or runtime
  validation was performed.

### Completed

- Established the portable legacy fingerprint: shared lifecycle function,
  direction-specific scalar sources, shared XMM0 consumer, related-object
  resolver and shared transition helper.

### Remaining / deferred

- Search the existing 2.0.4 program for structural/data-flow descendants.
- No runtime promotion until a concrete current-build boundary is found.

### Patch summary

Recovered the legacy ENTER/EXIT live-FOV callgraph and register/data-flow
contract from the validated 2.0.3 executable.

### Changelog summary

Documented the portable legacy FOV-consumption fingerprint without reusing its
historical addresses as 2.0.4 hook targets.

## 2026-09-01 — Reject ADS primitive-setter correction path

### Scope

- Validate whether the unique 2.0.4 `FirstPersonPrimitiveType` (`+0x265`) setter
  is invoked during the native ADS correction of the post-cinematic weapon/viewmodel state.
- Keep the test read-only and isolated; do not change the stable ASI.

### Changed

- Reviewed the supplied `STALKER2WeaponViewmodelPrimitiveSetterTrace204.log`.
- Updated `WEAPON_VIEWMODEL_FOV_POST_CINEMATIC_CAUSAL_TRACE_TASK_PLAN.md` with the runtime result.
- Rate-limited diagnostic ADS marker logging for the next trace build.

### Validation and limits

- Primitive setter, ADS IN/OUT and camera-writer resolvers installed successfully.
- No `Primitive setter:` event occurred during the captured ADS marker window.
- The log contained no explicit cinematic ENTER/EXIT marker, so the result closes
  the setter-during-ADS hypothesis for the captured window but does not identify
  the downstream correction mechanism.
- No production ASI or game state was modified.

### Completed

- Rejected the hypothesis that the observed ADS correction necessarily invokes
  the resolved `+0x265` setter.

### Remaining / deferred

- Actual first-person/viewmodel projection or refresh consumer remains unresolved.
- No production fix is justified from this branch.

### Patch summary

Closed the bounded ADS primitive-setter branch on negative runtime evidence and
reduced diagnostic marker spam.

### Changelog summary

Confirmed that the captured ADS correction path did not call the current 2.0.4
`+0x265` setter; stable gameplay/cinematic code remains unchanged.

## 2026-09-01 — Close known ADS primitive/mesh differential paths

### Scope

- Analyze the corrected single-window ADS differential capture.
- Determine whether known primitive setter or mesh-assignment paths change during
  the post-cinematic weapon/viewmodel correction.

### Changed

- Updated `WEAPON_VIEWMODEL_FOV_POST_CINEMATIC_CAUSAL_TRACE_TASK_PLAN.md` with the clean capture result.

### Validation and limits

- Exactly one ADS differential window was captured.
- No `+0x265` setter event and no mesh-assignment event occurred inside the window.
- Camera `+0x234`, `+0x262` and aspect remained stable; only world FOV transitioned
  from approximately `90.6557` to `83.6122`.
- The trace does not identify the downstream consumer that visually corrects the
  weapon/viewmodel.

### Completed

- Closed the known primitive-setter and mesh-assignment hypotheses for the
  captured ADS correction path.

### Remaining / deferred

- Identify the first-person/viewmodel projection or consumer path that responds
  to ADS while the inspected scalar and primitive paths remain unchanged.
- Static bounded audit of `+0x234` reads is next; no production implementation is justified.

### Patch summary

Used a clean ADS differential capture to reject the known `+0x265`/mesh refresh
paths and isolate the remaining problem to a downstream consumer layer.

### Changelog summary

Confirmed that the observed ADS correction does not use the inspected primitive
setter or mesh-assignment path; stable gameplay/cinematic code remains unchanged.

## 2026-09-01 — Close broad camera FOV offset intersection audit

### Scope

- Search current 2.0.4 code for consumers intersecting camera `+0x230`,
  `+0x234` and `+0x262`.
- Promote only a small runtime-plausible first-person projection consumer.

### Changed

- Added `02-Research/Ghidra/ghidra-scripts/AuditCameraFirstPersonFovConsumers204.java`.
- Added `02-Research/Ghidra/reports/camera-first-person-fov-consumer-audit-204.md`.
- Updated `WEAPON_VIEWMODEL_FOV_POST_CINEMATIC_CAUSAL_TRACE_TASK_PLAN.md` with
  the bounded scan result.

### Validation and limits

- Read-only Ghidra scan completed against the current program.
- Found 516 functions with at least two target offsets and 9 functions with all
  three offsets.
- All 9 all-three functions were large generic contexts; no compact projection
  consumer was justified.
- No runtime artifact or production ASI change was made.

### Completed

- Closed the raw `+0x230/+0x234/+0x262` intersection as insufficient for safe
  consumer selection.

### Remaining / deferred

- The downstream first-person/viewmodel consumer remains unresolved.
- Further work must narrow from ADS data-flow or a stronger semantic anchor,
  without opening a broad renderer search.

### Patch summary

Completed a bounded static consumer triage and rejected raw offset intersection
as a sufficient hook-selection method.

### Changelog summary

Documented that camera FOV offset intersections remain too generic for a safe
runtime hook; stable gameplay/cinematic code remains unchanged.

## 2026-09-01 — Audit ADS local call/data flow

### Scope

- Inspect only the validated current 2.0.4 ADS IN/OUT neighborhoods.
- Classify direct calls, object dereferences, transition math and state writes.
- Do not build a runtime tracer or alter the stable ASI without a stronger
  ownership contract.

### Changed

- Added `02-Research/Ghidra/ghidra-scripts/AuditAdsLocalCallChain204.java`.
- Added `02-Research/Ghidra/ghidra-scripts/RankAdsDirectCallees204.java`.
- Added `02-Research/Ghidra/ghidra-scripts/DecompileRankedAdsCallees204.java`.
- Added `02-Research/Ghidra/ghidra-scripts/PrintProgramIdentity204.java`.
- Added `02-Research/Ghidra/reports/ads-local-callchain-audit-204.md`.
- Updated `WEAPON_VIEWMODEL_FOV_POST_CINEMATIC_CAUSAL_TRACE_TASK_PLAN.md` with
  the Batch 9 result.

### Validation and limits

- The read-only Ghidra scripts executed successfully, but the selected Ghidra
  image was stale relative to the current runtime 2.0.4 executable.
- Static ADS matches were `RVA 0x6A849C` and `0x6A8639`; current runtime logs
  resolve the corresponding anchors at different RVAs.
- Direct-callee ranking and decompilation are invalidated for current 2.0.4
  address selection.
- No runtime artifact, production hook or stable ASI change was made.

### Completed

- Detected and explained the RVA discrepancy as a Ghidra image-identity
  mismatch rather than a function-entry versus hook-instruction offset.
- Preserved the stale-image findings as historical guidance only.

### Remaining / deferred

- Reopen the ADS local ranking against a Ghidra image matching the current game
  SHA-256 before selecting any runtime candidate.
- No runtime follow-up or production implementation is justified from this
  static batch.

### Patch summary

Added a read-only ADS call-chain audit, then invalidated its candidate ranking
after reconciling the stale Ghidra image with the current runtime executable.

### Changelog summary

Confirmed that the initial ADS static ranking used a stale image; current-build
candidate selection remains deferred, and stable gameplay/cinematic code remains
unchanged.

## 2026-09-02 — Revalidate ADS static analysis on matching 2.0.4 image

### Scope

- Use the canonical `Dump/STALKER2-Ghidra` entry
  `Stalker2-Win64-Shipping.exe (2.0.4)`.
- Enforce executable identity before repeating the ADS local audit and ranking.
- Reconcile current ADS anchors and inspect only the bounded direct-callee set.
- Do not create a runtime tracer or modify production ASI logic.

### Changed

- Updated `02-Research/Ghidra/ghidra-scripts/RankAdsDirectCallees204.java` to
  use the current 2.0.4 ADS containing-function entry `RVA 0x6ABB9C`.
- Updated `02-Research/Ghidra/ghidra-scripts/DecompileRankedAdsCallees204.java`
  with the current-image top three bounded candidates.
- Extended `02-Research/Ghidra/reports/ads-local-callchain-audit-204.md` with
  the matching-image identity header, current ADS slice and current ranking.

### Validation and limits

- Ghidra `.text` block size: `130818560`, matching the current executable.
- Image base: `0x140000000`, matching the current executable.
- Current executable SHA-256:
  `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`.
- ADS IN resolved uniquely at `RVA 0x6ABE7E`; ADS OUT resolved uniquely at
  `RVA 0x6AC01B`.
- Current ADS containing function is `FUN_1406ABB9C`; the old
  `FUN_1406A81BA` result is stale-image evidence and remains invalid.
- Current-image ranking found 13 direct callees. The top three were reviewed;
  none has sufficient ownership and causal evidence for a hook.
- Primitive-setter reconciliation is not included in this batch.

### Completed

- Passed the matching-image identity gate for the canonical 2.0.4 Ghidra entry.
- Revalidated the ADS local call-chain anchor and bounded direct-callee ranking
  on the correct image.
- Preserved stale-image provenance instead of replacing the earlier result.

### Remaining / deferred

- Reconcile the current-image primitive setter against the runtime-known
  `RVA 0x5665FA6` anchor.
- Revalidate any surviving ADS-state ownership candidate before proposing a
  bounded runtime trace.
- No production implementation is justified yet.

### Patch summary

Reopened the ADS static investigation on the canonical matching Steam 2.0.4
Ghidra image and replaced the invalid stale-image candidate set with a current
bounded ranking.

### Changelog summary

Corrected the analysis provenance for the ADS research; no stable gameplay or
 cinematic behavior was changed.

## 2026-09-02 — Complete matching-image production and P1/P2 revalidation

### Scope

- Reconcile the canonical `Dump/STALKER2-Ghidra` project with the Steam 2.0.4
  executable before interpreting static evidence.
- Revalidate production-facing cinematic/gameplay contracts and bounded
  high-potential cinematic alternatives.
- Do not modify production C++/ASI/INI logic, create a runtime tracer, launch
  the game or expand into weapon/viewmodel research.

### Changed

- Updated `MATCHING_IMAGE_REVALIDATION_TASK_PLAN.md` through final review.
- Updated `02-Research/Ghidra/reports/matching-image-revalidation-2026-09-02.md`
  with fresh current-image identity, contract reconciliation and P1/P2
  results.
- Preserved stale-image provenance and corrected the lock note: the leftover
  lock was caused by an agent-owned headless process, not the user's GUI
  session.

### Validation and limits

- Canonical `2.0.4` Ghidra entry SHA-256:
  `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`.
- `.text` size: `130818560`; image base: `0x140000000`.
- ADS IN/OUT anchors matched current runtime RVAs `0x6ABE7E` and `0x6AC01B`.
- Current ENTER aspect setter uniquely resolved at `RVA 0x6B7CB05`; its
  local data-flow contains aspect/mode stores but no FOV/projection owner.
- The generic cinematic helper returned `ENTER_MATCHES=0`; this was reconciled
  as a stale broad helper pattern, not promoted as an anchor failure.
- The recorded camera-field intersection contains `516` generic functions; no
  causal projection consumer was promoted from that non-specific result.
- No build, runtime tracer or in-game validation was performed in this task.

### Completed

- Matching-image identity gate: PASS.
- Production safety contracts: PASS within the tested current-image/source
  contract; no production contract was changed.
- P1/P2 cinematic alternatives: no concrete native or simpler replacement
  advantage demonstrated; no branch promoted or reopened.
- Agent-owned Ghidra process and stale lock were verified absent after
  analysis.

### Remaining / deferred

- Any runtime candidate trace or production change requires a separate approved
  task with its own identity gate and validation scope.
- Weapon/viewmodel causal research remains outside this task.

### Patch summary

Completed the matching-image recovery pass after the stale-Ghidra-image
incident, confirmed the current production contracts and closed the bounded
P1/P2 revalidation without changing production behavior.

### Changelog summary

Restored static-evidence provenance for the current 2.0.4 image; no native
cinematic replacement mechanism was proven, and no production or weapon FOV
implementation was changed.

## 2026-09-02 — Cross-patch production resolver validation

### Scope

- Apply the unchanged `0.4.0` production resolver contracts to the canonical
  Steam `2.0.2`, `2.0.3` and `2.0.4` Ghidra entries.
- Validate identity, uniqueness, gameplay instruction decoding and cinematic
  aspect/ENTER/EXIT contracts.
- Do not generalize signatures, modify production code, build/inject an ASI or
  expand into weapon research.

### Changed

- Added the read-only helper
  `02-Research/Ghidra/ghidra-scripts/ValidateCurrentProductionResolversAcrossPrograms.java`.
- Added
  `02-Research/Ghidra/reports/cross-patch-production-resolver-validation-2026-09-02.md`.
- Added and completed
  `CROSS_PATCH_PRODUCTION_RESOLVER_VALIDATION_TASK_PLAN.md`.

### Validation and limits

- Identity `PASS` for all three images; `.text` sizes were `130803712`,
  `130806784` and `130818560` respectively, with distinct `.text` hashes.
- Gameplay, cinematic aspect, ENTER and EXIT patterns each matched uniquely on
  `2.0.2`, `2.0.3` and `2.0.4`.
- Gameplay decode passed as `MOVSS [RBX+0x30], XMM0` on all three images.
- Overall Gate A result: `WOULD INSTALL` for all three versions.
- This is static resolver portability evidence, not runtime injection or
  in-game validation on the older versions.
- No production source, ASI or runtime tracer was changed.

### Completed

- Confirmed current production resolver portability across the retained Steam
  `2.0.2`, `2.0.3` and `2.0.4` images.
- Confirmed that no Gate B signature generalization is required for this set.
- Verified the agent-owned Ghidra process exited and the canonical project has
  no remaining lock file.

### Remaining / deferred

- Future patches still require a fresh identity gate and validation.
- Runtime validation on older game versions remains unperformed.
- Weapon/viewmodel research remains a separate bounded task.

### Patch summary

Added a read-only cross-patch Gate A simulation and documented unique resolver
matches and decode contracts across Steam 2.0.2, 2.0.3 and 2.0.4.

### Changelog summary

Confirmed that the unchanged 0.4.0 resolver architecture is statically
portable across the three tested Steam images; no production behavior changed.

## 2026-09-02 — Revalidate weapon/viewmodel FOV ownership on canonical 2.0.4

### Scope

- Revalidate primitive setter/MRSD linkage and ADS ownership on the matching
  Steam 2.0.4 Ghidra image.
- Re-rank direct ADS callees and inspect bounded camera-field evidence.
- Do not create a runtime tracer, modify production source/ASI or expand the
  weapon branch beyond the approved static scope.

### Changed

- Added the read-only helper
  `02-Research/Ghidra/ghidra-scripts/AuditCurrentWeaponViewmodelOwnership204.java`.
- Added
  `02-Research/Ghidra/reports/weapon-viewmodel-fov-revalidation-2026-09-02.md`.
- Updated `WEAPON_VIEWMODEL_FOV_POST_CINEMATIC_CAUSAL_TRACE_TASK_PLAN.md` with
  current-image ownership and ADS results.

### Validation and limits

- Canonical 2.0.4 identity passed: executable SHA-256
  `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`,
  `.text` size `130818560`, image base `0x140000000`.
- Primitive setter `RVA 0x5665FA6` resolved to `FUN_145665FA6`; its current
  instruction flow compares/writes `+0x265` and jumps to `0x140072660`.
- The setter has one discovered xref from `FUN_1454B26FE`; weapon/viewmodel
  ownership of its `this` object remains unproven.
- ADS resolved to `FUN_1406ABB9C`; 13 direct callees were ranked.
- Strongest candidate `FUN_1424BE5AE` is a generic interpolation helper with
  128 callers. No direct callee established a causal weapon/viewmodel
  projection owner.
- No build, runtime tracer or in-game validation was performed.

### Completed

- Current primitive setter/MRSD linkage: PASS.
- Current ADS containing-function and direct-callee ranking: PASS/COMPLETE.
- Direct primitive/MRSD explanation for the captured ADS correction: CLOSED as
  non-causal on available evidence.

### Remaining / deferred

- Downstream first-person/viewmodel projection ownership remains unresolved.
- A future runtime gate requires a specific current-image ownership candidate.
- No production implementation is justified.

### Patch summary

Revalidated weapon/viewmodel ownership anchors on the correct 2.0.4 image and
closed the stale-image primitive/MRSD branch without creating a new tracer.

### Changelog summary

Confirmed current primitive setter/MRSD linkage and current ADS ownership, but
found no bounded native weapon/viewmodel projection candidate.

## 2026-09-02 — Reject AF4FA4 projection branch for post-cinematic weapon correction

### Scope

- Validate the `FUN_140AF4FA4` entry predicate using the existing read-only
  runtime gate.
- Record `source+0x25C`, its absolute value, the computed branch and source
  flags `+0x260/+0x261` across cinematic EXIT and ADS correction.
- Do not add production behavior or expand into generic renderer analysis.

### Changed

- Updated the existing PRE-only read-only runtime tracer to observe the entry
  predicate; removed the unused POST-hook path.
- Added the matching-image predicate audit and updated the downstream consumer
  report and task plan.

### Validation and limits

- Canonical Steam 2.0.4 identity passed.
- Runtime capture recorded 1914 PRE events across the cinematic/ADS sequence.
- `source+0x25C=0`, `abs(+0x25C)=0`, and `predicate=early` for all observed
  events; `+0x260/+0x261` remained `0x0/0x0`.
- `source+0x230` and output `+0x30/+0x38` still showed the ADS world-FOV
  transition.
- No projection/tanf/atanf path entry was observed; no production ASI was
  changed.

### Completed

- Rejected `FUN_140AF4FA4` as the causal owner for the captured post-cinematic
  weapon correction.
- Preserved the independent world-FOV propagation finding.

### Remaining / deferred

- The downstream first-person/viewmodel correction owner remains unresolved.
- No new runtime tracer is authorized until a new bounded static candidate is
  identified.

### Patch summary

Added PRE-only predicate observation and closed the AF4FA4 projection branch on
current 2.0.4 runtime evidence.

### Changelog summary

Confirmed that the observed ADS weapon correction does not enter the candidate
consumer's projection-derived path; no production behavior changed.

## 2026-09-02 — Stop local output+0x38 downstream-use audit

### Scope

- Inspect only the local caller tail after `FUN_140AF4022 + 0xAF42A7`.
- Find direct or obvious local consumers of the validated presentation
  output/`+0x38` with first-person/viewmodel semantics.
- Do not scan generic renderer/data-flow, create a runtime tracer or change
  production behavior.

### Changed

- Added the bounded helper
  `02-Research/Ghidra/ghidra-scripts/AuditOutput38DownstreamUse204.java`.
- Added
  `02-Research/Ghidra/reports/ads-output38-downstream-use-audit-204-2026-09-02.md`.
- Completed `ADS_OUTPUT38_DOWNSTREAM_USE_TASK_PLAN.md`.

### Validation and limits

- Canonical 2.0.4 identity passed.
- No direct `RBX+0x38` read or write was found after the consumer call.
- No direct callee was established as a first-person/viewmodel presentation
  owner.
- `RAX/RCX+0x38` accesses were not tied to the validated output object.
- No runtime tracer or production ASI change was made.

### Completed

- Bounded output+0x38 downstream-use theory stopped as `UNRESOLVED`.
- Generic renderer/data-flow expansion was explicitly not performed.

### Remaining / deferred

- The downstream weapon/viewmodel owner remains unresolved and is deferred.
- Dialogue zoom is the next independent research scope.

### Patch summary

Audited the local presentation-output tail and stopped without promoting an
unsupported downstream consumer.

### Changelog summary

No local current-2.0.4 first-person/viewmodel consumer was identified; no
production behavior changed.

---

## 2026-09-05 — Cinematic axis-constraint ownership Batch 1B

### Scope

- Perform the approved narrow source-to-binary correspondence pass for the
  UE 5.5 `GetProjectionData` / effective axis-policy handoff hypothesis.
- Do not change production source, build artifacts or runtime behavior.

### Changed paths

- `research/reports/CINEMATIC_ASPECT_RATIO_AXIS_CONSTRAINT_OWNERSHIP_BATCH1B.md`
- `backlog/active/CINEMATIC_ASPECT_RATIO_AXIS_CONSTRAINT_OWNERSHIP_TASK_PLAN.md`

### Validation and limits

- Current Steam 2.0.4 identity gate: `PASS`.
- Existing current-build evidence was checked for viewport/aspect, effective
  FOV, projection construction and axis-dependent semantics.
- Historical 2.0.3 projection logs were retained as provenance only.
- No safe current-build projection boundary or effective owner was established.
- No runtime test, tracer, source change, build or production modification
  was performed.

### Result

- Batch 1B: `COMPLETE — BLOCKED OUTCOME`; no promotable current-build
  boundary was found.
- UE 5.5 engine contract: confirmed as a research target, not as STALKER 2
  binary proof.
- Batch 2 `MaintainYFOV` runtime test remains blocked.
- Production Full Hor+ remains unchanged.

### Patch summary

Checked the approved `GetProjectionData` structural target without promoting
generic or historical projection helpers as current-build ownership.

### Changelog summary

No production behavior changed; the axis-constraint branch remains unresolved
and deferred pending new evidence.

---

## 2026-09-03 — Dialogue zoom production promotion

### Scope

- Promote the validated Adaptive and optical half-strength dialogue model into
  the production dialogue subsystem.
- Keep the existing resolver boundary, cinematic isolation, ADS specificity,
  gameplay and cinematic subsystems unchanged.
- Use `DialogueCycle=F10` as the production default; custom bindings remain
  supported.

### Changed

- Updated `src/experimental_cinematic_21_9_combined_fix_204.cpp`.
- Updated the unified test/build script and public configuration examples.
- Added `DIALOGUE_ZOOM_PRODUCTION_PROMOTION_TASK_PLAN.md`.
- Production policy cycle is now `Native → Adaptive → Reduced → Disabled → Native`.
- `Reduced` now uses the promoted optical half-strength model.
- Promoted the validated EXIT anchor/recovery behavior.
- Removed feasibility-only `OpticalReduced` and EXIT diagnostic instrumentation
  from the production build path.

### Validation and limits

- Build succeeded for the production candidate.
- `git diff --check` passed with only existing line-ending warnings.
- Canonical Steam 2.0.4 runtime identity passed:
  `gameSha256=2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`.
- Runtime log confirmed all four policies, custom hotkey cycling, persistence,
  sequential dialogue recovery and cinematic/gameplay hook installation.
- User visually confirmed smooth ENTER/EXIT behavior for all four policies.
- Runtime coverage is the tested Steam 2.0.4 scenarios; future patches and
  untested game states remain outside this result.

### Completed

- Dialogue production core promotion: PASS.
- Native, Adaptive, Reduced and Disabled: PASS.
- Sequential dialogue cycles and baseline recovery: PASS.
- Hotkey cycle and persistence: PASS.
- Existing gameplay/cinematic behavior: no regression observed.

### Remaining / deferred

- Final release archive/package review for v0.5.0 remains.
- Runtime compatibility on Steam 2.0.2/2.0.3 remains statically portable but
  not runtime-validated.

### Patch summary

Promoted the validated FOV-aware optical dialogue zoom model and smooth EXIT
recovery into the production dialogue path.

### Changelog summary

Added Adaptive dialogue zoom, changed Reduced to optical half-strength behavior,
and preserved the native dialogue lifecycle with smooth recovery.

---

## 2026-09-10 — D3D12 overlay ownership feasibility boundary

### Scope

- Investigate the approved bounded path from DXGI factory creation toward an
  active game swapchain for optional notification-overlay feasibility.
- Keep the stable Camera/FOV core and release assets untouched.
- Stop before Present, ResizeBuffers, command submission, render resources,
  ImGui or notifications.

### Changed paths

- `src/d3d12_notification_overlay_feasibility_204.cpp`
- `backlog/active/DXGI_COM_SAFE_SWAPCHAIN_INTERCEPTION_TASK_PLAN.md`
- `backlog/active/EXISTING_ACTIVE_SWAPCHAIN_OWNERSHIP_TASK_PLAN.md`
- `research/reports/D3D12_NOTIFICATION_OVERLAY_BATCH1A_BOOTSTRAP_RESULT.md`
- `research/reports/EXISTING_ACTIVE_SWAPCHAIN_OWNERSHIP_GATE_A_INVENTORY.md`
- `build-artifacts/test-asi/STALKER2NotificationOverlayFeasibility.asi`

### Validation and evidence

- Visual Studio 2022 x64 C++23 build: PASS.
- Final test ASI size: 800,256 bytes.
- Final test ASI SHA-256:
  `298F3455697F1D51E7648995CEA9EDD9EB108CD5B8275749733162DF3E98B8C3`.
- Earlier build with composition slot `17` was invalid and excluded from
  evidence; the final build uses `24`.
- Official DXGI interface review confirmed the relevant method family and
  D3D12 `pDevice` semantics; the ABI mapping used in the final build is
  
  `10=CreateSwapChain`, `15=CreateSwapChainForHwnd`,
  `16=CreateSwapChainForCoreWindow`, `24=CreateSwapChainForComposition`.
- User runtime log: `CreateDXGIFactory1` returned `IDXGIFactory4`, the queried
  `IDXGIFactory2` pointer matched the returned address, and the private
  25-entry clone installed without a crash.
- No `CreateSwapChain*` callback was observed in the corrected run.
- No swapchain, device or queue provenance was obtained.
- Git commands were intentionally not run at the user's request; no Git state
  change was performed or claimed.

### Completed

- Rejected the earlier unsafe direct SafetyHook COM-VMT approach after its
  access violation evidence.
- Implemented and built the bounded per-instance COM clone path.
- Added forensic factory IID, returned-interface, ABI and provenance logging.
- Corrected the composition method slot before accepting the final runtime
  result.
- Completed the corrected factory-path observation as a valid bounded negative:
  no creation callback was observed.
- Completed Gate A inventory for existing swapchain acquisition: no
  independent current-build, documented runtime or retained-COM acquisition
  anchor was found.

### Remaining / deferred / blocked

- `EXISTING_ACTIVE_SWAPCHAIN_OWNERSHIP`: `BLOCKED / DEFERRED` at Gate A.
- Swapchain ownership, device provenance and queue provenance were not reached.
- Present/Resize lifecycle, command submission, rendering and ImGui were not
  started.
- Overlay idea remains `DEFERRED`, not failed or rejected; resume requires a
  genuinely new ownership evidence class.
- Production Camera/FOV core and release behavior remain unchanged.

### Patch summary

Added a bounded COM-safe factory observation implementation, corrected the DXGI
ABI mapping, and documented the absence of an independently proven path to an
already-existing game swapchain.

### Changelog summary

No production behavior changed. D3D12 notification overlay research is deferred
because presentation ownership could not be established safely.

---

## 2026-09-10 — UE4SS reflected camera-call route closed

### Scope

- Audit the UE4SS reflected-call surface for `CameraComponent.GetCameraView`.
- Perform a follow-up read-only metadata/callability audit of
  `PlayerCameraManager.BlueprintUpdateCamera`.
- Keep runtime invocation, writes, suppression, production ASI changes and
  undocumented helper reverse engineering out of scope.

### Changed paths

- `research/deferred/GETCAMERAVIEW_CALLABILITY_PROBE_TASK_PLAN.md`

### Validation and evidence

- Current UE4SS metadata confirms `GetCameraView(float, FMinimalViewInfo&)`.
- Current UE4SS metadata confirms
  `BlueprintUpdateCamera(AActor*, FVector&, FRotator&, float&)`.
- `SafeObject.call(...)` was inspected and does not establish safe complex
  ref/out storage or readback semantics.
- `CallFunctionsByHandle(...)` exists only through a wrapper; no source-backed
  contract, packing rules, lifetime rules or ref/out example was found.
- No runtime invocation was attempted by design.
- Existing unrelated working-tree changes were preserved.

### Completed

- Closed the UE4SS reflected-call route as `CLOSED / BLOCKED BY LUA ABI`.
- Classified both camera candidates as metadata-positive but invocation-blocked.
- Corrected the archived plan to record the later
  `BlueprintUpdateCamera` read-only audit without misrepresenting it as part of
  the primary runtime probe.

### Remaining / deferred / blocked

- The camera-evaluation hypothesis remains open; only this Lua invocation path
  is exhausted.
- Resume requires a documented/source-backed call bridge, a simpler callable
  reflected method, or a new independent lifecycle anchor.
- No production camera behavior or release asset changed.

### Patch summary

Documented the bounded UE4SS callability audit and its concrete Lua ABI blocker;
no runtime probe or behavior modification was added.

### Changelog summary

No production changes. Reflected camera evaluation remains deferred pending a
safe, documented invocation contract.

---

## 2026-09-10 — ASI vtable `+0x638` static gate closed

### Scope

- Classify only the virtual dispatch immediately after the confirmed camera
  writer for the canonical Steam 2.0.4 image.
- Determine whether trusted target/type evidence establishes camera/view or
  projection reevaluation semantics.
- No runtime calls, writes, new hooks, broad scans or production changes.

### Changed paths

- `research/deferred/ASI_LAST_GATE_VTABLE_638_TASK_PLAN.md`

### Validation and evidence

- Existing bounded Ghidra report
  `02-Research/reports/CINEMATIC_AXIS_POLICY_OUTPUT_OWNERSHIP_BATCH1_1_VSLOT638.md`
  records identity `PASS` for SHA-256
  `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`.
- Image base `0x140000000` and the recorded `.text` layout match the canonical
  Steam 2.0.4 image.
- `FUN_1453BA300` preserves the source/output pair across
  `FUN_140AF4022` and the following virtual call at `[RDI + 0x638]`.
- No trusted concrete virtual target or owner/type association was recovered
  within the bounded pass.
- No runtime feasibility test was performed or authorized.

### Completed

- Completed the final ASI static gate as `COMPLETE / BOUNDED PARTIAL`.
- Confirmed same-object source/output handoff into the virtual dispatch.
- Closed the ASI cinematic route at this gate as `CLOSED / BLOCKED` for lack
  of proven reevaluation semantics.

### Remaining / deferred / blocked

- The broader camera-evaluation hypothesis is not disproven.
- Resume requires genuinely new target/type or lifecycle evidence; neighboring
  slot scans and further caller climbing remain out of scope.
- Production ASI and stable gameplay behavior remain unchanged.

### Patch summary

Recorded the existing bounded vtable-slot audit and its unresolved polymorphic
target; no runtime or production code was changed.

### Changelog summary

No production changes. The final ASI cinematic research gate is deferred because
`+0x638` could not be promoted to a proven camera/view reevaluation boundary.

---

## 2026-09-10 — UE4SS cinematic architecture feasibility closed

### Scope

- Audit the installed UE4SS observer, state-access and callable surfaces for a
  future cinematic subsystem.
- Keep the stable Camera/FOV ASI, release assets and runtime behavior untouched.

### Changed paths

- `research/reports/UE4SS_CINEMATIC_ARCHITECTURE_FEASIBILITY.md`
- `research/deferred/UE4SS_CINEMATIC_ARCHITECTURE_FEASIBILITY_TASK_PLAN.md`

### Validation and evidence

- Existing UE4SS observers confirm live Camera/CameraManager/PCM discovery,
  reflected state reads and lifecycle sampling.
- Reflected property mutation is available, but storage mutation alone causing
  the required downstream reevaluation was not observed.
- `GetCameraView` and `BlueprintUpdateCamera` metadata is present, but complex
  ref/out invocation remains unsupported by a confirmed Lua ABI contract.
- `CallFunctionsByHandle` is available only through an opaque wrapper without
  source-backed packing, lifetime or readback rules.
- No runtime writes, undocumented calls, replacement module or production
  migration was performed.
- Existing unrelated working-tree changes were preserved.

### Completed

- Completed capability matrix and architecture decision gate.
- Classified UE4SS observer/state architecture as `FEASIBLE`.
- Classified UE4SS cinematic native-refresh architecture as
  `BLOCKED / UNPROVEN`.
- Deferred production migration as `NOT JUSTIFIED`.

### Remaining / deferred / blocked

- Cinematic research remains deferred pending a genuinely new evidence class.
- Native reevaluation and downstream projection rebuild remain unresolved.
- Stable production ASI remains the active fallback and was not modified.

### Patch summary

Documented the UE4SS architecture boundary: strong observation/state access,
but no proven safe native cinematic reevaluation mechanism.

### Changelog summary

No production changes. UE4SS migration is deferred; only observer/state-layer
use is currently justified.

---

## 2026-09-10 — UE4SS STALKER2CameraTweaks parallel prototype

### Scope

- Create a separate UE4SS module named `STALKER2CameraTweaks`.
- Port the established gameplay, cinematic and dialogue policies where the
  available UE4SS Lua/property surface is safe and deterministic.
- Keep the production ASI and all native source behavior untouched.

### Changed paths

- `research/ue4ss/STALKER2CameraTweaks/Scripts/main.lua`
- `research/ue4ss/STALKER2CameraTweaks/STALKER2CameraTweaks.ini`
- `research/ue4ss/STALKER2CameraTweaks/README.md`
- `build-artifacts/test-ue4ss/STALKER2CameraTweaks.zip`
- `UE4SS_STALKER2CAMERATWEAKS_PORT_TASK_PLAN.md` (archived after review)

### Validation and evidence

- Verified dynamic CameraComponent/CameraManager/PCM discovery logic and
  object invalidation recovery paths by source inspection.
- Verified the ZIP contains only the intended UE4SS module files.
- `git diff --check` found no new whitespace errors in the implementation;
  one pre-existing trailing-whitespace warning remains in an unrelated line of
  `backlog/TASKLOG.md`.
- No Lua interpreter is installed in the workspace, so automatic Lua syntax
  validation was not available.
- No game launch or runtime validation was performed.

### Completed

- Added the separate UE4SS module with the required module name.
- Added configuration-compatible gameplay, cinematic and dialogue policy
  handling with bounded reflected writes and change-only state logging.
- Documented the unsupported native reevaluation/ref-out limits and ASI-off
  test isolation requirement.
- Built the test ZIP without production ASI files.

### Remaining / deferred / blocked

- In-game behavior, visual framing and post-exit restoration remain
  `NOT RUNTIME-VALIDATED`.
- Native downstream projection reevaluation after reflected writes remains
  `UNPROVEN`; no unsafe invocation was added.
- The stable ASI remains the production fallback and was not modified.

### Patch summary

Added a parallel UE4SS implementation that exposes the current camera policy
model through dynamic live-object discovery and bounded property operations.

### Changelog summary

New research/test artifact: `STALKER2CameraTweaks` UE4SS prototype. No change
to the stable ASI release path.

---

## 2026-09-10 — UE4SS gameplay native-like transition correction

### Scope

- Re-analyze the confirmed native gameplay A→B→C transition.
- Correct only the gameplay path of the UE4SS `STALKER2CameraTweaks`
  prototype.
- Keep cinematic and dialogue mutation disabled for this validation build.

### Changed paths

- `research/ue4ss/STALKER2CameraTweaks/Scripts/main.lua`
- `research/ue4ss/STALKER2CameraTweaks/STALKER2CameraTweaks.ini`
- `research/ue4ss/STALKER2CameraTweaks/README.md`
- `build-artifacts/test-ue4ss/STALKER2CameraTweaks-gameplay-test.zip`
- `UE4SS_GAMEPLAY_NATIVE_FIX_TASK_PLAN.md` (archived after review)

### Validation and evidence

- Reconfirmed from the supplied UE4SS log that the previous build only loaded
  with gameplay disabled and recorded state; it did not reproduce the native
  two-pass behavior.
- Replaced the one-step final-state write with a bounded sequence:
  constrained on → native 16:9 aspect → constrained off.
- Added explicit phase, retry-limit and completion logging.
- Static structural checks passed; no Lua interpreter is available for an
  automatic syntax check.
- Archive contents were inspected and contain only the intended module files.
- No in-game runtime validation was performed after this correction.

### Completed

- Implemented the gameplay-only native-like transition attempt.
- Disabled cinematic/dialogue mutation execution in this test build.
- Preserved dynamic object discovery and authored gameplay FOV.

### Remaining / deferred / blocked

- Visual gameplay framing and downstream projection rebuild remain
  `NOT RUNTIME-VALIDATED`.
- UE4SS setter side effects are not yet proven equivalent to the native
  reevaluation path.
- Cinematic and dialogue ports remain intentionally untested.
- Production ASI remains untouched.

### Patch summary

Corrected the UE4SS gameplay prototype to follow the evidence-backed native
two-pass transition instead of writing only the final reflected state.

### Changelog summary

Updated the UE4SS gameplay test artifact; no production ASI changes.

---

## 2026-09-10 — UE4SS module config path correction

### Scope

- Resolve the UE4SS prototype's adjacent INI from the script location rather
  than relying on UE4SS's current working directory.
- Keep gameplay algorithm, cinematic/dialogue behavior and production ASI
  untouched.

### Changed paths

- `research/ue4ss/STALKER2CameraTweaks/Scripts/main.lua`
- `build-artifacts/test-ue4ss/STALKER2CameraTweaks-gameplay-configpath-fix.zip`
- `research/deferred/UE4SS_CONFIG_PATH_FIX_TASK_PLAN.md`

### Validation and evidence

- Fresh installed log showed `Gameplay.Enabled=false` while the adjacent
  installed INI contained `Enabled=true`, proving the previous log value could
  not be trusted as evidence of the adjacent file.
- The module now logs `CONFIG_PATH` and resolves
  `Scripts/../STALKER2CameraTweaks.ini` first.
- Structural checks and archive-content inspection passed.
- Runtime validation after the path correction remains pending.

### Completed

- Removed the `.ini` path ambiguity without introducing `.conf` handling.
- Produced a new gameplay test ZIP with the corrected loader.

### Remaining / deferred / blocked

- Gameplay visual success remains unvalidated until a new launch log is
  supplied.
- No cinematic or dialogue testing was performed.
- Production ASI remains untouched.

### Patch summary

Fixed UE4SS configuration resolution so `STALKER2CameraTweaks` reads the INI
beside its own script and reports the exact selected path.

### Changelog summary

Configuration-path reliability fix for the UE4SS gameplay test prototype.

---

## 2026-09-10 — UE4SS module-specific logging

### Scope

- Add a dedicated `STALKER2CameraTweaks.log` beside the UE4SS module INI.
- Keep gameplay behavior unchanged and keep cinematic/dialogue unvalidated.

### Changed paths

- `research/ue4ss/STALKER2CameraTweaks/Scripts/main.lua`
- `research/ue4ss/STALKER2CameraTweaks/README.md`
- `build-artifacts/test-ue4ss/STALKER2CameraTweaks-gameplay-module-log.zip`
- `research/deferred/UE4SS_MODULE_LOG_TASK_PLAN.md`

### Validation and evidence

- Log path is derived from `Scripts/main.lua`, independent of UE4SS working
  directory.
- Module messages continue to appear in `UE4SS.log` and are also written to
  the dedicated module log.
- Static checks and ZIP content inspection passed.
- Runtime creation was not revalidated after this change.

### Completed

- Added dedicated module logging with a `LOG_PATH` startup record.
- Preserved gameplay logic and did not confirm cinematic/dialogue behavior.

### Remaining / deferred / blocked

- Runtime validation of the new log remains pending.
- Cinematic and dialogue fixes remain unconfirmed.
- Production ASI remains untouched.

### Patch summary

Added isolated logging for UE4SS `STALKER2CameraTweaks` evidence collection.

### Changelog summary

UE4SS test module now writes its own module-specific log.

---

## 2026-09-10 — UE4SS cinematic and dialogue policy port

### Scope

- Restore the cinematic aspect/FOV and dialogue policy branches in the
  separate `STALKER2CameraTweaks` UE4SS module.
- Keep the validated gameplay branch and production ASI unchanged.

### Changed paths

- `research/ue4ss/STALKER2CameraTweaks/Scripts/main.lua`
- `research/ue4ss/STALKER2CameraTweaks/README.md`
- `build-artifacts/test-ue4ss/STALKER2CameraTweaks-all-policies-test.zip`
- `research/deferred/UE4SS_CINEMATIC_DIALOGUE_PORT_TASK_PLAN.md`

### Validation and evidence

- Re-enabled cinematic lifecycle handling through the shared
  `SetCinematicMode` callback when available, with a reflected-hook fallback.
- Re-enabled configured cinematic aspect policies and the existing Full Hor+
  FOV policy for non-`Native` modes.
- Re-enabled the existing dialogue zoom detector and policy model.
- Confirmed no separate `DisplayAspectRatio` or `PreserveAuthoredFOV` contract
  remains in the module.
- Inspected the test archive; it contains only the module README, INI and
  `Scripts/main.lua`.
- Runtime cinematic and dialogue behavior was not validated.

### Completed

- Restored the two requested UE4SS policy branches without ref/out calls,
  native hooks, raw offsets or production ASI changes.
- Preserved the dedicated module log and dynamic object discovery.

### Remaining / deferred / blocked

- User must test cinematic and dialogue behavior in-game.
- Visual framing, letterbox removal and post-exit restoration remain
  unconfirmed.
- UE4SS native reevaluation remains unproven; writes are best-effort reflected
  property/setter operations.

### Patch summary

Restored the UE4SS module's cinematic lifecycle/aspect handling and dialogue
zoom policy while keeping gameplay logic and production ASI untouched.

### Changelog summary

UE4SS test module now includes the previously disabled cinematic and dialogue
policy branches for runtime evaluation.

---

## 2026-09-10 — UE4SS dialogue/cinematic conflict correction

### Scope

- Correct the newly restored cinematic/dialogue branches after the first test
  log showed FOV oscillation and missing cinematic activation.

### Changed paths

- `research/ue4ss/STALKER2CameraTweaks/Scripts/main.lua`
- `build-artifacts/test-ue4ss/STALKER2CameraTweaks-all-policies-test.zip`

### Validation and evidence

- The supplied log showed repeated dialogue writes of `110.791499` while the
  game interpolated the camera FOV, confirming a write loop.
- Dialogue application is now bounded to one write per detected transition;
  lifecycle changes reset dialogue state.
- Added a state fallback for cinematic paths that do not emit the reflected
  `SetCinematicMode` callback, restricted to a completed gameplay stage.
- Runtime retest remains pending.

### Completed

- Removed the known repeated-write conflict from the UE4SS dialogue branch.
- Added a non-invasive cinematic detection fallback.

### Remaining / deferred / blocked

- Cinematic visual framing and dialogue behavior still require a fresh in-game
  test.
- The fallback cannot prove native evaluation or guarantee all cinematic types.

### Patch summary

Bounded dialogue FOV writes to prevent oscillation and added fallback cinematic
state detection when the reflected lifecycle hook is silent.

### Changelog summary

UE4SS test module no longer continuously fights the game's dialogue FOV
interpolation.

---

## 2026-09-10 — UE4SS cinematic FOV baseline and write-loop correction

### Scope

- Correct the second runtime-test regression reported in the module log:
  exaggerated cinematic FOV and repeated cinematic/dialogue writes.

### Changed paths

- `research/ue4ss/STALKER2CameraTweaks/Scripts/main.lua`
- `research/ue4ss/STALKER2CameraTweaks/README.md`
- `build-artifacts/test-ue4ss/STALKER2CameraTweaks-all-policies-test.zip`

### Validation and evidence

- The supplied log showed cinematic FOV `148.582718`, caused by using the
  transient `FieldOfView=120` instead of the authored
  `FirstPersonFieldOfView=90` baseline.
- The log also showed repeated cinematic writes every poll while the game
  continued its own camera transition.
- Cinematic baseline now prefers `FirstPersonFieldOfView` and cinematic policy
  writes are bounded to once per entry.
- Runtime retest remains pending.

### Completed

- Removed the observed `120 → 148.58` cinematic over-expansion source.
- Removed continuous cinematic writes that could fight dialogue/camera
  interpolation.

### Remaining / deferred / blocked

- Fresh gameplay, dialogue and cinematic runtime evidence is still required.
- No claim is made yet about final visual framing or dialogue behavior.

### Patch summary

Use the authored first-person FOV as the cinematic source and apply cinematic
state once per lifecycle entry to avoid FOV inflation and oscillation.

### Changelog summary

UE4SS cinematic test path now avoids transient gameplay FOV inflation and
repeated per-poll camera writes.

---

## 2026-09-10 — UE4SS double-FOV and transition misclassification correction

### Scope

- Correct the supplied runtime regression showing gameplay FOV corruption and
  cinematic over-expansion.

### Changed paths

- `research/ue4ss/STALKER2CameraTweaks/Scripts/main.lua`
- `build-artifacts/test-ue4ss/STALKER2CameraTweaks-all-policies-test.zip`

### Validation and evidence

- The log showed `DIALOGUE_START baselineFOV=120` during the constrained
  gameplay aspect transition; this was a false dialogue detection.
- The log showed cinematic `90` being transformed to `148.582725`; the formula
  had applied the aspect conversion twice.
- Dialogue detection now ignores constrained camera lifecycle transitions.
- Cinematic FOV conversion now uses one aspect conversion and waits for the
  target aspect state before applying it once.
- Runtime retest remains pending.

### Completed

- Removed the observed false dialogue trigger during gameplay reevaluation.
- Removed the observed double aspect conversion in cinematic FOV.
- Preserved authored first-person FOV as the cinematic source.

### Remaining / deferred / blocked

- Fresh runtime evidence is required for gameplay, dialogue and cinematic
  visual behavior.

### Patch summary

Corrected the cinematic Hor+ calculation and prevented gameplay camera
transitions from being mistaken for dialogue zoom.

### Changelog summary

UE4SS test module no longer double-converts cinematic FOV or seeds dialogue
zoom from a constrained gameplay transition.

---

## 2026-09-10 — UE4SS authored cinematic FOV tracking

### Scope

- Preserve instant cinematic correction while handling later native authored
  FOV updates without a per-poll write loop.

### Changed paths

- `research/ue4ss/STALKER2CameraTweaks/Scripts/main.lua`
- `research/ue4ss/STALKER2CameraTweaks/README.md`
- `build-artifacts/test-ue4ss/STALKER2CameraTweaks-all-policies-test.zip`

### Validation and evidence

- The supplied log showed native camera logic overwriting the one-shot
  cinematic result back toward 90 degrees.
- Cinematic tracking now compares each observed FOV with the last transformed
  value and transforms only a new observed input.
- The instant ENTER correction is retained.
- Runtime retest remains pending.

### Completed

- Added self-write suppression for cinematic FOV tracking.
- Preserved the existing gameplay and dialogue isolation rules.

### Remaining / deferred / blocked

- Fresh runtime evidence is required to confirm cinematic shot changes,
  dialogue behavior and post-exit restoration.

### Patch summary

Retained instant cinematic correction and added bounded authored-FOV tracking
for later native camera updates.

### Changelog summary

UE4SS cinematic handling now follows new authored FOV inputs instead of
repeating the same transformed write every poll.

---

## 2026-09-10 — Unified ASI build wiring correction

### Scope

- Connect the current unified gameplay/cinematic/dialogue source to the
  canonical build command.

### Changed paths

- `build.cmd`
- `STALKER2CameraTweaks.asi`
- `ASI_UNIFIED_BUILD_TASK_PLAN.md`

### Validation and evidence

- `build.cmd` now compiles
  `src/experimental_cinematic_21_9_combined_fix_204.cpp`.
- Bounded local build completed successfully with VS 2022.
- Output name is now `STALKER2CameraTweaks.asi`, matching README.
- Runtime validation and game installation were not performed.

### Completed

- Fixed the build wiring that previously compiled only the superseded
  gameplay-only source.

### Remaining / deferred / blocked

- The new unified ASI still requires in-game validation on the current game
  executable.
- No runtime compatibility claim is made for the post-update game build.

### Patch summary

Canonical build now produces the documented unified gameplay/cinematic/dialogue ASI.

### Changelog summary

Unified ASI source is now included in the default build command.

---

## 2026-09-10 — Current-patch cinematic EXIT resolver update

### Scope

- Adapt the unified cinematic resolver to the current game executable after
  the previous EXIT signature reported zero matches.
- Keep gameplay and dialogue implementation unchanged.

### Changed paths

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- `STALKER2CameraTweaks.asi`
- `ASI_CINEMATIC_EXIT_UPDATE_TASK_PLAN.md`

### Validation and evidence

- Current executable SHA-256:
  `E7B481A97C02D80581FAB0BECE940214A88EBE30211088A00129845A039F9293`.
- Current ENTER topology resolves uniquely with one match.
- New indexed EXIT topology resolves uniquely with one match at the static
  scan level and preserves the validated consumer/vcall continuation shape.
- VS 2022 local build completed successfully.
- New ASI size: `1,095,680` bytes.
- Follow-up runtime log showed the pattern was found but rejected by an
  off-by-one decoded-instruction length/callsite check; corrected the indexed
  `MOVSS` length to 6 bytes and rebuilt the ASI.
- Corrected ASI SHA-256:
  `69021D8758069F7EFE098B3C562A41E326A6DB0BF4BA88B789FB38854217DFB2`.
- Runtime injection and in-game behavior after the update were not validated.
- The game-folder ASI was not installed or modified by this task.

### Completed

- Added the current indexed `movss` EXIT pattern and semantic Zydis validation.
- Preserved the previous EXIT pattern as a fallback for older compatible
  topology.
- Rebuilt the documented unified `STALKER2CameraTweaks.asi`.

### Remaining / deferred / blocked

- Fresh in-game validation is required on the current executable.
- No new compatibility guarantee is claimed until the ASI is injected and the
  runtime log confirms both cinematic hooks.

### Patch summary

Updated cinematic EXIT resolution for the post-update indexed camera sample
path while retaining safe uniqueness and decoded-instruction checks.

### Changelog summary

Cinematic FOV hook resolution now recognizes the current game patch's EXIT
instruction topology.
## 2026-09-11 — Release preparation v0.5.2

- Scope: prepared the unified ASI release archive for v0.5.2; no runtime logic
  changes were introduced. Non-goals were research modules, old artifact
  cleanup, commit/tag/publish and new gameplay validation.
- Paths changed: `src/experimental_cinematic_21_9_combined_fix_204.cpp`,
  release-facing Markdown files, `release-assets/STALKER2CameraTweaks.asi`,
  `release-assets/STALKER2CameraTweaks.ini`,
  `release-assets/README.md`, and the v0.5.2 release archive. Evidence is in
  `research/reports/RELEASE_PREPARATION_v0.5.2.md`.
- Git state: branch `main`; working tree contained pre-existing unrelated
  changes and research artifacts. No Git state-changing operation was run.
- Validation: build succeeded; production ASI SHA-256 is
  `F55B17768625549D96E033FD79340A79DCA27FD9697264C37F4E7A6D6BAEEFB4`;
  archive SHA-256 is
  `73DD26FCAA29732E951728FE5EF01417AEB539435AA377D89F3858AC9D89A433`;
  archive allowlist contains exactly five files. Supplied runtime evidence is
  for the preceding current-build binary, not a new post-build injection.
- Completed: v0.5.2 metadata, compatibility wording, production asset refresh
  and archive construction.
- Remaining: user review and any explicit publication/commit approval.
- Deferred: runtime regression of the newly rebuilt binary; older-patch runtime
  testing; all experimental camera research.
- Blocked: none for package construction.
- Not runtime-validated: the newly rebuilt v0.5.2 binary in this task.
- Patch summary: updated release metadata for Steam 2.0.5 and preserved the
  existing unified resolver implementation.
- Changelog summary: v0.5.2 package update for the current Steam build, with
  static resolver portability documented separately for Steam 2.0.2–2.0.4.

## 2026-09-12 — Post-EXIT PCM/ViewTarget/CameraCache topology audit 2.0.5

- Scope: one bounded, read-only Ghidra audit around the confirmed cinematic
  handoff and camera writer. Non-goals were runtime probes, source changes,
  production behavior, broad scans and guessed calls.
- Paths changed: archived task plan at
  `research/deferred/POST_EXIT_PCM_VIEWTARGET_CAMERACACHE_TOPOLOGY_AUDIT_205_TASK_PLAN.md`.
  Research script, runner, evidence and report are under the workspace-level
  `02-Research/` tree, outside the canonical Git repository.
- Git state: branch `main`; pre-existing dirty changes and research artifacts
  were preserved; no Git state-changing operation was run.
- Validation: current 2.0.5 identity passed in Ghidra with SHA-256
  `E7B481A97C02D80581FAB0BECE940214A88EBE30211088A00129845A039F9293`, image
  base `0x140000000` and `.text` size `0x7CCD000`. The tracked headless run
  completed and no canonical Ghidra lock remained.
- Completed: audited `FUN_14318DCD4`, `FUN_14366F9AA`, `FUN_1453A7C88` and
  `FUN_140A9EF7C`, including one-level direct callers and the requested PCM/cache
  offset neighborhood.
- Remaining: PCM/ViewTarget/cache blend ownership and the physical post-EXIT
  interpolation producer remain unresolved.
- Deferred: further ASI static expansion and runtime probing until a new
  concrete topology anchor is available.
- Blocked: no new PCM topology anchor was recovered within the approved scope.
- Not runtime-validated: no behavior or production binary was changed or tested.
- Patch summary: added a version-gated read-only topology audit and durable
  evidence report; archived the completed plan as deferred.
- Changelog summary: none; production behavior and release assets unchanged.

## 2026-09-12 — Post-cinematic gameplay replay defer research build

- Scope: compile-time research branch that defers the existing gameplay aspect
  replay until three stable samples from the same source after confirmed native
  cinematic recovery. Non-goals were production behavior, cinematic/dialogue
  logic, FOV writes/clamps and direct EXIT timers.
- Paths changed: `src/experimental_cinematic_21_9_combined_fix_204.cpp`,
  `build-artifacts/research/build-post-cinematic-gameplay-replay-defer-test.cmd`,
  and the separate research artifact
  `build-artifacts/research/STALKER2CameraTweaks-PostCinematicGameplayReplayDeferTest205.asi`.
- Git state: branch `main`; unrelated dirty changes and existing research
  artifacts were preserved; no Git state-changing operation was run.
- Validation: VS 2022 build succeeded with
  `POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST` and a 120 ms post-stability
  delay. Research ASI size is 1,107,968 bytes; SHA-256 is
  `CBCF0F0387E71587923AE049ABC1AA73C789181FEBA9716B01FE823C7D750F9E`.
  No production binary was overwritten or installed.
- Completed: same-source gate, three consecutive stable FOV samples,
  coordinator/cinematic/dialogue/FOV/aspect invalidation, optional post-stability
  delay and defer telemetry markers.
- Remaining: clean in-game validation of recovery timing, replay timing and
  visual framing.
- Deferred: promotion to production and any change to the released ASI.
- Blocked: none for the research build.
- Not runtime-validated: behavior of the research ASI in-game.
- Patch summary: added an isolated state-based post-cinematic replay defer test
  while preserving the existing replay implementation as the only apply path.
- Changelog summary: none; research-only artifact, not a release change.

## 2026-09-14 — v1.0.0 Batch 2A research-surface isolation

- Scope: physically separate the 63 non-production research/legacy `.cpp`
  files from the production source surface without changing runtime
  semantics.
- Paths changed: moved non-production `.cpp` files from `src/` into
  `research/probes/` and `research/traces/`; the production monolith remains
  temporarily in `src/` for the next domain-split batch.
- Git state: branch `main`; unrelated object-file changes, documentation
  changes and user research artifacts were preserved; no Git state-changing
  operation was run.
- Validation: `build.cmd` completed successfully and produced
  `STALKER2CameraTweaks.asi`; no in-game runtime validation was performed.
- Completed: `src/` now contains only the current production `.cpp` plus
  headers; research isolation did not alter the current compile command.
- Remaining: split the production monolith into the planned domain modules;
  separate production and research build targets.
- Deferred: lifecycle safety and runtime semantic findings remain Batch 3
  work and were not changed here.
- Blocked: none for the bounded isolation batch.
- Not runtime-validated: in-game behavior after the structural move.
- Patch summary: isolated research source files while preserving the current
  production build path.
- Changelog summary: none; intermediate refactor batch, not a release change.

## 2026-09-14 — v1.0.0 Batch 2B.2 config subsystem closure

- Scope: complete config I/O and template ownership while preserving existing
  load, managed-template and persistence semantics.
- Paths changed: added `src/config/config_template.hpp` and
  `src/config/config_template.cpp`; completed `src/config/config_repository.*`;
  updated `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully after template extraction,
  legacy-body removal and default config-path extraction; ownership search no
  longer finds INI parsing, template manipulation or persistence implementation
  in `runtime.cpp`.
- Completed: `config/` now owns model/policies, config loading/creation,
  persistence, default config path and managed template synchronization.
- Remaining: continue Batch 2 with hooks and domain decomposition.
- Deferred: persistence failure classification/fix remains Batch 3 work; no
  fallback behavior was changed.
- Blocked: none for config subsystem closure.
- Not runtime-validated: in-game behavior after the config extraction.
- Patch summary: completed the first production subsystem and removed its
  duplicate legacy implementations from the runtime module.
- Changelog summary: none; intermediate refactor batch, not a release change.

## 2026-09-14 — v1.0.0 Batch 2B.2d config load extraction

- Scope: move configuration file creation and value loading behind the config
  repository while preserving the existing template-sync callback and parsing
  semantics.
- Paths changed: updated `src/config/config_repository.hpp` and
  `src/config/config_repository.cpp`; updated `src/plugin/runtime.cpp` and
  `build.cmd` remained compatible with the added module.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: first build caught and fixed a missing `FeatureConfig` header
  dependency; the repeated `build.cmd` completed successfully and produced
  `STALKER2CameraTweaks.asi`.
- Completed: config file creation and value loading now use
  `config::LoadFeatureConfig`; managed template synchronization remains
  injected as a callback so its current behavior is preserved.
- Remaining: extract `SynchronizeManagedConfigTemplate` into
  `config_template.*`, remove the duplicate legacy load/persistence bodies
  from `runtime.cpp`, then close the config subsystem.
- Deferred: persistence failure classification/fix remains Batch 3 work.
- Blocked: none for this bounded extraction step.
- Not runtime-validated: behavior after config I/O migration.
- Patch summary: moved config load semantics behind the repository boundary
  while retaining the existing template synchronization path.
- Changelog summary: none; intermediate refactor batch, not a release change.

## 2026-09-14 — v1.0.0 Batch 2B.2c config persistence extraction

- Scope: move the existing `PersistConfigValue` I/O behavior behind a config
  repository API without changing its atomic-replace/direct-write fallback.
- Paths changed: added `src/config/config_repository.hpp` and
  `src/config/config_repository.cpp`; updated `src/plugin/runtime.cpp` and
  `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully and compiled the new config
  repository module; no in-game runtime validation was performed.
- Completed: hotkey persistence now calls `config::PersistConfigValue` with a
  logger callback; existing fallback semantics were preserved intentionally.
- Remaining: move `LoadFeatureConfig` and managed template synchronization,
  then remove the duplicate legacy implementations from `runtime.cpp`.
- Deferred: persistence failure classification/fix remains Batch 3 work.
- Blocked: none for this bounded extraction step.
- Not runtime-validated: behavior after config I/O migration.
- Patch summary: introduced the config repository persistence boundary without
  applying the pending safety fix.
- Changelog summary: none; intermediate refactor batch, not a release change.

## 2026-09-14 — v1.0.0 Batch 2B.2 config contract extraction

- Scope: establish config type and pure policy ownership without changing
  parsing, persistence or runtime transition semantics.
- Paths changed: added `src/config/feature_config.hpp` and
  `src/config/feature_config.cpp`; updated `src/plugin/runtime.cpp` and
  `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully and compiled
  `feature_config.cpp`; no in-game runtime validation was performed.
- Completed: policy enums, `FeatureConfig`, trimming, bool/hotkey/policy
  parsing, policy naming and policy cycling now have a config ownership
  boundary.
- Remaining: extract config repository/template synchronization and
  persistence from `runtime.cpp`.
- Deferred: lifecycle safety and runtime semantic changes remain Batch 3 work.
- Blocked: none for the bounded config-contract extraction.
- Not runtime-validated: behavior after the config split.
- Patch summary: moved pure config semantics behind an explicit header/source
  boundary while preserving the existing build and runtime path.
- Changelog summary: none; intermediate refactor batch, not a release change.

## 2026-09-14 — v1.0.0 Batch 2B.1 plugin entrypoint split

- Scope: separate DLL entrypoint ownership from the production runtime without
  changing initialization, hook cleanup or runtime semantics.
- Paths changed: moved the production TU to `src/plugin/runtime.cpp`; added
  `src/plugin/runtime.hpp` and `src/plugin/dll_entry.cpp`; updated `build.cmd`
  to compile both production modules.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully and produced
  `STALKER2CameraTweaks.asi`; no in-game runtime validation was performed.
- Completed: `DllMain` now owns only process attach/detach dispatch; runtime
  initialization and existing cleanup remain behind the plugin runtime facade.
- Remaining: split config, hooks, gameplay, cinematics, dialogue and Win32
  ownership out of `src/plugin/runtime.cpp`.
- Deferred: loader-lock safety, thread shutdown and transactional rollback
  semantics remain Batch 3 work.
- Blocked: none for the bounded entrypoint split.
- Not runtime-validated: behavior after the structural split.
- Patch summary: introduced the first production ownership boundary and kept
  the current build contract intact.
- Changelog summary: none; intermediate refactor batch, not a release change.

## 2026-09-14 — Weapon Viewmodel FOV research closure

- Scope: close the reference WVF investigation after causal validation of
  `MPC_FOV.TanFOV` and archive the standalone reflection-bridge blocker.
- Paths changed: `TESTING_AND_RESEARCH.md`; WVF plans moved from
  `backlog/active` into `research/completed` or `research/deferred` according
  to their final status.
- Git state: branch `main`; unrelated dirty object files and user research
  artifacts were preserved; no Git state-changing operation was run.
- Validation: read-only plan/status review and final path review; no new game
  launch, build or production runtime validation.
- Completed: reference profile → FOV math → `MPC_FOV.TanFOV` → visible
  viewmodel framing was retained as confirmed causal evidence; runtime WVF
  bootstrap and profile/application path were archived.
- Remaining: none within the current Weapon Viewmodel FOV research scope.
- Deferred: standalone UE reflection/invocation bridge, post-cinematic
  `TanFOV` repair, and Custom Weapon FOV.
- Blocked: production implementation by the absence of a validated,
  patch-resilient native UE reflection bridge.
- Not runtime-validated: any standalone ASI implementation of the deferred
  repair path; no such implementation was made.
- Patch summary: documented the final production boundary and archived the
  completed/deferred WVF research plans.
- Changelog summary: none; production source and release artifacts unchanged.

## 2026-09-14 — WVF Batch 3.1 provider-backed spawn identity

- Scope: offline CUE4Parse import-resolution diagnostic for the targeted
  `WVF.uasset` package. No game launch, UE4SS runtime change, production ASI/
  source change or release work.
- Paths changed: `research/tools/CUE4ParseWVF/Program.cs`,
  `backlog/active/WVF_APPLICATION_PATH_BATCH3_SPAWN_PROFILE_TASK_PLAN.md`.
- Git state: branch `main`; unrelated dirty object files and untracked user
  research material were preserved; no Git state-changing operation was run.
- Validation: the research dumper built successfully with 0 errors; the exact
  `N.utoc` was registered with `DefaultFileProvider`, and the provider-backed
  package load resolved both previously unresolved spawn imports.
- Completed: `FPackageIndex -1` resolves to
  `/S2Dev_Library/S2Dev_Event_Watcher.S2Dev_Event_Watcher_C`; `-3` resolves to
  `/Weapon_Viewmodel_FOV/WVF_Actor.WVF_Actor_C`.
- Remaining: prove which downstream state/function owns visible weapon-FOV
  framing; Batch 3.2 remains partial.
- Deferred: runtime validation and any ASI implementation.
- Blocked: none for bounded spawn identity resolution.
- Not runtime-validated: all conclusions in this batch are static/tooling
  evidence only.
- Patch summary: registered the research IoStore container with the provider
  before package analysis and selected the provider-backed package for import
  resolution, retaining the direct-reader fallback.
- Changelog summary: none; research-only tooling and evidence.

## 2026-09-14 — WVF Batch 3.2 MPC scalar target recovery

- Scope: offline static continuation from the provider-backed `WVF_Actor_C`
  Kismet package. No game launch, UE4SS runtime change, production ASI/source
  change or release work.
- Paths changed: `research/tools/CUE4ParseWVF/Program.cs` and
  `backlog/active/WVF_APPLICATION_PATH_BATCH3_SPAWN_PROFILE_TASK_PLAN.md`.
- Git state: branch `main`; unrelated dirty object files and untracked user
  research material were preserved; no Git state-changing operation was run.
- Validation: the provider-backed CUE4Parse run completed successfully after
  registering the exact `N.utoc`; `WVF_Actor_C` Kismet expressions resolved
  eight scalar-write branches.
- Completed: all eight bounded `SetScalarParameterValue` calls target
  `/Game/_Stalker_2/Materials/MPC/MPC_FOV.MPC_FOV` with parameter `TanFOV`;
  each value is a local float produced after the profile/viewport/FOV math
  branch.
- Remaining: identify the downstream reader of `MPC_FOV.TanFOV` and prove its
  relationship to visible weapon/viewmodel projection.
- Deferred: runtime causal validation and ASI implementation.
- Blocked: none for the bounded scalar-target recovery.
- Not runtime-validated: MPC scalar ownership of visible framing remains
  unproven in-game.
- Patch summary: retained the provider-backed package path and recorded the
  exact MPC, parameter, branch statement indices and value-input indices.
- Changelog summary: none; research-only tooling and evidence.

## 2026-09-14 — WVF Batch 4.1 MPC downstream-reference boundary

- Scope: bounded offline inspection of cooked package/index metadata for
  downstream consumers of `MPC_FOV.TanFOV`. No game launch, UE4SS runtime
  change, production ASI/source change or release work.
- Paths changed: `research/tools/CUE4ParseWVF/Program.cs`,
  `backlog/active/WVF_MPC_TANFOV_DOWNSTREAM_CONSUMER_BATCH4_TASK_PLAN.md`.
- Git state: branch `main`; unrelated dirty object files and untracked user
  research material were preserved; no Git state-changing operation was run.
- Validation: research dumper build/run succeeded; the provider index resolved
  `Stalker2/Content/_Stalker_2/Materials/MPC/MPC_FOV.uasset`.
- Completed: confirmed the collection asset is indexed and established that the
  available provider exposes forward reference scanning but no bounded reverse
  reference/dependency index for locating all cooked material consumers.
- Remaining: identify a safe, bounded source of reverse `MPC_FOV` consumers;
  first-person/viewmodel classification has not started.
- Deferred: broad cooked-material crawling, runtime validation and ASI
  implementation.
- Blocked: downstream consumer recovery is blocked by missing reverse-reference
  metadata in the current bounded CUE4Parse/provider path.
- Not runtime-validated: no claim about shader-side ownership or visible
  framing was made.
- Patch summary: added an indexed-collection and provider-capability probe,
  then stopped at the documented reverse-reference boundary.
- Changelog summary: none; research-only tooling and evidence.

## 2026-09-14 — WVF Batch 5.1 bounded static feasibility

- Scope: read-only check for an addressable asset-registry, dependency table or
  reverse-reference source for `MPC_FOV.TanFOV`. No game launch, UE4SS runtime
  change, production ASI/source change or release work.
- Paths changed: `backlog/active/WVF_MPC_TANFOV_CAUSAL_VALIDATION_BATCH5_TASK_PLAN.md`.
- Git state: branch `main`; unrelated dirty object files and untracked user
  research material were preserved; no Git state-changing operation was run.
- Validation: no `AssetRegistry.bin` or equivalent bounded registry file was
  found under the installed game directory; the provider capability remains
  forward-only (`ScanForPackageRefs`) with no reverse-reference index.
- Completed: Batch 5.1 static-feasibility gate and exact tooling limitation.
- Remaining: decide whether to authorize the one targeted `TanFOV` causal
  intervention described in Batch 5.2.
- Deferred: runtime intervention, broad material crawl and ASI implementation.
- Blocked: bounded static consumer recovery is unavailable with current
  metadata/tooling.
- Not runtime-validated: no causal ownership conclusion was made.
- Patch summary: recorded the 5.1 stop condition and preserved the single
  targeted-intervention route for a later bounded decision.
- Changelog summary: none; research-only planning/evidence.

## 2026-09-14 — WVF Batch 5.2 TanFOV intervention probe prepared

- Scope: prepare one disposable UE4SS runtime probe for the confirmed
  `MPC_FOV.TanFOV` causal intervention. No game launch, probe installation,
  production ASI/source change or release work.
- Paths changed: `research/ue4ss/MPC_TanFOVCausalProbe/Scripts/main.lua` and
  `backlog/active/WVF_MPC_TANFOV_CAUSAL_VALIDATION_BATCH5_TASK_PLAN.md`.
- Git state: branch `main`; unrelated dirty object files and untracked user
  research material were preserved; no Git state-changing operation was run.
- Validation: textual API review against the existing working
  `KismetMaterialLibrary` MPC pattern completed. No Lua interpreter is
  available locally, so syntax and in-game behavior are not validated.
- Completed: F9 one-shot probe resolves the exact MPC and `TanFOV`, reads the
  baseline, writes `baseline × 1.25`, reads back, and records weapon/
  AnimScript identity before and after. No polling and no restore.
- Remaining: install the disposable research probe and perform the single
  authorized post-EXIT intervention, if the user chooses to run it.
- Deferred: causal classification, runtime owner decision and ASI
  implementation.
- Blocked: none in probe preparation; runtime validation is pending.
- Not runtime-validated: all mutation/readback/framing outcomes.
- Patch summary: added a bounded F9 TanFOV intervention probe with explicit
  readback and lifecycle-stability logging.
- Changelog summary: none; research-only probe.

## 2026-09-14 — WVF Batch 5.2 TanFOV causal intervention

- Scope: one F9 intervention using the disposable `MPC_TanFOVCausalProbe`
  with `Weapon Viewmodel FOV - 0` active. No production ASI/source change or
  release work.
- Paths changed: `backlog/active/WVF_MPC_TANFOV_CAUSAL_VALIDATION_BATCH5_TASK_PLAN.md`
  and the user-provided runtime log was inspected read-only.
- Git state: branch `main`; unrelated dirty object files and untracked user
  research material were preserved; no Git state-changing operation was run.
- Validation: log readback confirmed `TanFOV` changed from
  `1.678197145462` to `2.0977463722229` after a requested `2.0977464318275`;
  `MutationState=CONFIRMED`. Weapon mesh and `AnimBP_pm_fp_C` identity were
  unchanged before/after. User observed a distinct framing change immediately
  after F9.
- Completed: active `MPC_FOV.TanFOV` intervention causally changes visible
  weapon framing under this controlled run.
- Remaining: identify the downstream shader/material consumer and determine a
  clean independent implementation seam.
- Deferred: production ASI implementation and any broad material crawl.
- Blocked: none for the bounded causal gate.
- Not runtime-validated: no independent downstream consumer identity was
  recovered; the later menu-triggered framing change is classified separately
  as an additional refresh/reapply event.
- Patch summary: confirmed the one authorized TanFOV intervention and stable
  weapon/AnimScript identity across the write.
- Changelog summary: none; research-only causal evidence.

## 2026-09-14 — WVF production seam Seam 1 MPC access feasibility

- Scope: offline architecture review of independent `MPC_FOV.TanFOV` access
  for a future production seam. No stable source edit, game launch, runtime
  hook or release work.
- Paths changed: `backlog/active/WVF_TANFOV_PRODUCTION_SEAM_RESEARCH_TASK_PLAN.md`.
- Git state: branch `main`; unrelated dirty object files and untracked user
  research material were preserved; no Git state-changing operation was run.
- Validation: existing UE4SS probe evidence confirms exact MPC read/write with
  readback; existing ASI reflection audit confirms no safe `UObject`/
  `UFunction`/`StaticFindObject`/`ProcessEvent` bridge in stable C++.
- Completed: UE4SS access feasibility confirmed; production-compatible native
  access classified `PARTIAL/BLOCKED`.
- Remaining: determine whether a separately authorized native correspondence
  step or a UE4SS-assisted architecture can provide a production-safe seam.
- Deferred: repair timing/value recovery, stable ASI changes and Custom Weapon
  FOV.
- Blocked: direct stable-ASI MPC access without a new reflection/native bridge.
- Not runtime-validated: no new runtime test was run in this seam batch.
- Patch summary: recorded the split between confirmed UE4SS access and blocked
  stable-ASI reflection access.
- Changelog summary: none; research-only architecture result.

## 2026-09-14 — Native UE reflection bridge Bridge 1 inventory

- Scope: read-only architecture inventory of standalone ASI/research C++ for a
  grounded UE reflection/native invocation entry point. No Ghidra launch, game
  launch, runtime write, production source change or release work.
- Paths changed: `backlog/active/NATIVE_UE_REFLECTION_BRIDGE_FEASIBILITY_TASK_PLAN.md`.
- Git state: branch `main`; unrelated dirty object files and untracked user
  research material were preserved; no Git state-changing operation was run.
- Validation: bounded source/research search found SafetyHook/Zydis and
  game-specific native traces, but no existing UObject/UClass/UFunction bridge,
  object registry correspondence, ProcessEvent path or validated call-handle
  ABI. This matches the prior reflection audit.
- Completed: Bridge 1 architecture inventory.
- Remaining: none within this branch unless new grounded reflection evidence
  appears.
- Deferred: Bridge 2 native correspondence, invocation feasibility, repair
  timing/value and production implementation.
- Blocked: standalone ASI reflection access is blocked without a new grounded
  native correspondence anchor.
- Not runtime-validated: no runtime work was authorized or performed.
- Patch summary: applied the early stop condition and classified the bridge
  branch as `DEFERRED`.
- Changelog summary: none; research-only architecture result.

## 2026-09-12 — Combined atomic cinematic/gameplay handoff candidate

- Scope: separate build combining the runtime-validated gameplay atomic replay
  and cinematic RecoveryStart atomic handoff branches.
- Non-goals: full staged replay after cinematic EXIT, new writes, timers,
  dialogue/cinematic formula changes, production replacement and publishing.
- Paths changed: `build-artifacts/research/build-combined-atomic-cinematic-gameplay-handoff-candidate.cmd`,
  `backlog/COMBINED_ATOMIC_CINEMATIC_GAMEPLAY_HANDOFF_CANDIDATE_TASK_PLAN.md`,
  and separate artifact
  `build-artifacts/research/STALKER2CameraTweaks-CombinedAtomicCinematicGameplayHandoffCandidate205.asi`.
- Git state: branch `main`; unrelated dirty work was preserved; no Git
  state-changing operation was run.
- Validation: VS 2022 build succeeded. Candidate ASI SHA-256 is
  `788D98DCEFC6B268D83858CD5D92BFDABD64B5C07577EC33877013F56A7A361A`.
  Production binary was not overwritten or installed.
- Completed: combined candidate build using the two already tested atomic
  compile-time paths.
- Remaining: one combined in-game regression run.
- Deferred: production promotion and release-asset changes.
- Blocked: none for the build.
- Not runtime-validated: combined candidate behavior.
- Patch summary: prepared a single candidate for integrated gameplay and
  post-cinematic atomic handoff validation.
- Changelog summary: none; research-only candidate, not a release change.

## 2026-09-12 — Atomic gameplay mutation deduplication refactor

- Scope: behavior-preserving refactor of the combined candidate. Gameplay and
  cinematic RecoveryStart triggers now share `ApplyGameplayAspectFixAtomic`.
- Non-goals: trigger/state-machine changes, staged-state removal, cinematic or
  dialogue behavior changes, production replacement and release changes.
- Paths changed: `src/experimental_cinematic_21_9_combined_fix_204.cpp`,
  `backlog/ATOMIC_GAMEPLAY_MUTATION_DEDUP_REFACTOR_TASK_PLAN.md`, and the
  rebuilt combined candidate under `build-artifacts/research`.
- Git state: branch `main`; unrelated dirty work was preserved; no Git
  state-changing operation was run.
- Validation: VS 2022 combined candidate build succeeded. `AppliedConstrainPass`
  remains referenced by the legacy staged path; it was not removed. Production
  binary was not overwritten or installed.
- Completed: duplicated atomic mutation logic consolidated; independent gates
  remain in place.
- Completed: runtime regression check passed; gameplay and RecoveryStart
  atomic applies each occurred once and no legacy staged replay markers were
  observed.
- Deferred: production promotion and release-asset changes.
- Blocked: none for the build.
- Runtime validation: PASS for the tested gameplay and cinematic transition.
- Patch summary: centralized the already validated atomic aspect/flags write.
- Changelog summary: none; research-only refactor, not a release change.

## 2026-09-12 — Cinematic FOV transition bypass Pass 1 trace

- Scope: observation-only trace built on the validated combined atomic
  candidate. The known FOV consumer now records CinematicActive and
  CinematicExiting phases with incoming FOV, state fields and caller data.
- Non-goals: writes, suppression, clamping, transition bypass, duration/alpha
  changes, new hooks and production/release changes.
- Paths changed: `src/experimental_cinematic_21_9_combined_fix_204.cpp`,
  `build-artifacts/research/build-cinematic-fov-transition-bypass-pass1.cmd`,
  `backlog/CINEMATIC_FOV_TRANSITION_BYPASS_PASS1_TASK_PLAN.md`, and separate
  trace artifact
  `build-artifacts/research/STALKER2CameraTweaks-CinematicFovTransitionBypassPass1-Trace205.asi`.
- Git state: branch `main`; unrelated dirty work was preserved; no Git
  state-changing operation was run.
- Validation: VS 2022 build succeeded. Trace ASI SHA-256 is
  `60A2CBC20F3518809585EC382A0341FE95FE5001326B771F7DDFC5BB9BAA9647`.
  Production binary was not overwritten or installed.
- Completed: bounded ENTER/EXIT consumer instrumentation.
- Remaining: one runtime trace run and transition classification.
- Deferred: any FOV transition bypass implementation and production promotion.
- Blocked: none for the trace build.
- Not runtime-validated: trace behavior in-game.
- Patch summary: added observation-only cinematic phase/state telemetry.
- Changelog summary: none; research-only trace, not a release change.

## 2026-09-12 — Gameplay fix atomicity research build

- Scope: compile-time gameplay-only research branch replacing the staged
  aspect/flags replay with one final `1.77778/0x4` write.
- Non-goals: cinematic and dialogue changes, production artifact changes, new
  hooks, timers, FOV changes and release packaging.
- Paths changed: `src/experimental_cinematic_21_9_combined_fix_204.cpp`,
  `build-artifacts/research/build-gameplay-fix-atomicity-test.cmd`,
  `backlog/GAMEPLAY_FIX_ATOMICITY_TEST_TASK_PLAN.md`, and separate artifact
  `build-artifacts/research/STALKER2CameraTweaks-GameplayFixAtomicityTest205.asi`.
- Git state: branch `main`; unrelated dirty work was preserved; no Git
  state-changing operation was run.
- Validation: VS 2022 build succeeded. Research ASI SHA-256 is
  `BC4F2CBAEFA2F3FE8FB7B8D4B48DB92A6E0DB46567CC9ADFF7113726578D5751`.
  Production binary was not overwritten or installed.
- Completed: guarded one-shot atomic gameplay replay and dedicated telemetry
  marker; staged replay remains the default without the research flag.
- Remaining: clean in-game validation of gameplay framing and transitions.
- Deferred: production promotion and release-asset changes.
- Blocked: none for the research build.
- Not runtime-validated: in-game behavior of this artifact.
- Patch summary: added a gameplay-only atomic final-state replay candidate.
- Changelog summary: none; research-only artifact, not a release change.

## 2026-09-12 — Post-cinematic atomic gameplay replay at recovery start

- Scope: compile-time research branch that waits for the first confirmed
  downward FOV sample during final `CinematicExiting`, then applies the final
  gameplay aspect/flags state once as `1.77778/0x4`.
- Non-goals: production changes, cinematic ENTER changes, dialogue changes,
  timers, repeated clamps, new hooks and hard-coded gameplay FOV.
- Paths changed: `src/experimental_cinematic_21_9_combined_fix_204.cpp`,
  `build-artifacts/research/build-post-cinematic-gameplay-atomic-exit-handoff-test.cmd`,
  and separate artifact
  `build-artifacts/research/STALKER2CameraTweaks-PostCinematicGameplayAtomicExitHandoffTest205.asi`.
- Git state: branch `main`; unrelated dirty work was preserved; no Git
  state-changing operation was run.
- Validation: VS 2022 build succeeded. Research ASI SHA-256 is
  `0CF023173DCE9591677DE5996B35534BDBCD137872E5ED74519BFDA66C54E2B8`.
  Production binary was not overwritten or installed.
- Completed: first-downward-sample gate, same-source guard, dialogue/coordinator
  cancellation and `phase=RecoveryStart` telemetry.
- Remaining: one clean in-game comparison against the recovery-complete atomic
  artifact.
- Deferred: production promotion and release-asset changes.
- Blocked: none for the research build.
- Not runtime-validated: in-game behavior of this artifact.
- Patch summary: moved the one-shot atomic gameplay projection apply from the
  end of native recovery to the first confirmed native FOV descent.
- Changelog summary: none; research-only artifact, not a release change.

## 2026-09-12 — Post-cinematic gameplay replay at recovery research build

- Scope: separate compile-time research variant that applies the final gameplay
  aspect/flags state in the same writer invocation that confirms native FOV
  recovery. Non-goals were pre-recovery writes, timers, new hooks, FOV changes
  and production behavior.
- Paths changed: `src/experimental_cinematic_21_9_combined_fix_204.cpp`,
  `build-artifacts/research/build-post-cinematic-gameplay-replay-at-recovery-test.cmd`,
  and separate artifact
  `build-artifacts/research/STALKER2CameraTweaks-PostCinematicGameplayReplayAtRecoveryTest205.asi`.
- Git state: branch `main`; unrelated dirty work was preserved; no Git
  state-changing operation was run.
- Validation: VS 2022 build succeeded with the recovery-time research flags.
  Research ASI size is 1,108,992 bytes; SHA-256 is
  `1B03CEBA06E3D13632F90BA8D475AB78DD1FF678A3BE592AF0784F5A23BD731A`.
  Production binary was not overwritten or installed.
- Completed: recovery-complete atomic apply path and explicit phase telemetry.
- Remaining: one clean in-game comparison against the deferred atomic build.
- Deferred: production promotion and release-asset changes.
- Blocked: none for the research build.
- Not runtime-validated: in-game behavior of the recovery-time artifact.
- Patch summary: moved the research atomic apply to the earliest boundary where
  native FOV recovery is already confirmed.
- Changelog summary: none; research-only artifact, not a release change.

## 2026-09-14 — WVF Kismet entrypoint recovery Batch 2

- Scope: offline CUE4Parse inspection of the reference `WVF` and
  `WVF_Actor` IoStore packages using the runtime-generated `.usmap`. No game
  launch, UE4SS runtime change, production ASI/source change or release work.
- Paths changed: `research/tools/CUE4ParseWVF/Program.cs` and
  `backlog/active/WVF_APPLICATION_PATH_BATCH2_KISMET_TASK_PLAN.md`.
- Git state: branch `main`; unrelated dirty object files and untracked user
  research material were preserved; no Git state-changing operation was run.
- Validation: CUE4Parse tool build succeeded with 0 errors; direct reads of
  `WVF.uasset` (5839 bytes) and `WVF_Actor.uasset` (25866 bytes) exited 0.
  No runtime visual validation was performed in this batch.
- Completed: mapped runtime EP `1423`, `15` and `3903` to serialized Kismet
  statements; confirmed `OnWorldBeginPlay → ExecuteUbergraph_WVF(1423)`,
  two spawn/finish chains, `ReceiveBeginPlay → ExecuteUbergraph_WVF_Actor(3903)`,
  and the actor material-parameter path. Confirmed `.wvf` profile parsing in
  `UserConstructionScript`.
- Remaining: resolve the two spawn class import references and prove the
  visible weapon-FOV owner/function.
- Deferred: any ASI implementation or runtime hook based on these static edges.
- Blocked: none for the bounded Kismet objective.
- Patch summary: added bounded Kismet expression and package-reference
  inspection to the research dumper.
- Changelog summary: none; research-only tooling and evidence.

## 2026-09-12 — Post-cinematic gameplay replay atomicity research build

- Scope: compile-time research branch layered on the deferred handoff gate. It
  applies final aspect/flags `1.77778/0x4` in one writer invocation after three
  stable same-source samples and the existing 120 ms research delay. Non-goals
  were production changes, new hooks, FOV changes and renderer intervention.
- Paths changed: `src/experimental_cinematic_21_9_combined_fix_204.cpp`,
  `build-artifacts/research/build-post-cinematic-gameplay-replay-atomicity-test.cmd`,
  and separate artifact
  `build-artifacts/research/STALKER2CameraTweaks-PostCinematicGameplayReplayAtomicityTest205.asi`.
- Git state: branch `main`; unrelated dirty work was preserved; no Git
  state-changing operation was run.
- Validation: VS 2022 build succeeded with the defer and atomicity defines.
  Research ASI size is 1,108,992 bytes; SHA-256 is
  `BD506C52CE44919063C06EBC1FFF6458211BC24F2AE6813771C2C99E796DA7CA`.
  Production binary was not overwritten or installed.
- Completed: stable-sample counter now stops at `3/3`; atomic research path,
  final-state write and telemetry marker were added behind compile-time flags.
- Remaining: one clean in-game test is required to compare visual framing and
  confirm whether the intermediate projection jump disappears.
- Deferred: promotion to production and any release-asset change.
- Blocked: none for the research build.
- Not runtime-validated: in-game behavior of the atomicity artifact.
- Patch summary: added a one-invocation final gameplay aspect/flags test while
  preserving the existing deferred and production replay paths.
- Changelog summary: none; research-only artifact, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.3a hook signature definitions

- Scope: extract production signature definitions from `plugin::Runtime`
  without changing resolver, validation or hook lifecycle semantics.
- Paths changed: added `src/hooks/signatures/signature_definitions.hpp` and
  updated `src/plugin/runtime.cpp` to consume the centralized definitions.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  old local signature definitions were removed from `runtime.cpp`.
- Completed: camera-writer, cinematic, dialogue and vcall signature data now
  has a dedicated hooks/signatures owner.
- Remaining: extract scanner, instruction validation and hook ownership in
  later Batch 2B.3 sub-batches.
- Deferred: resolver ambiguity handling, rollback, transactionality and
  shutdown semantics remain Batch 3 work.
- Blocked: none for signature-definition extraction.
- Not runtime-validated: in-game hook resolution and behavior after this
  structural extraction.
- Patch summary: centralized signature definitions while preserving the
  existing runtime call sites and validation flow.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.3b scanner and instruction validator extraction

- Scope: extract candidate discovery and Zydis instruction-validation
  primitives without changing resolver decisions or hook lifecycle semantics.
- Paths changed: added `src/hooks/signature_scanner.*` and
  `src/hooks/instruction_validator.*`; updated `src/plugin/runtime.cpp` and
  `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root.
  The intermediate duplicate-symbol issue caused by including the legacy
  header implementation in a second translation unit was removed without
  changing the helper implementation or runtime behavior.
- Completed: scanner forwarding, executable-memory validation, Zydis decode,
  rel32 call validation, byte-shape checks and rel32 target resolution now
  have dedicated hooks owners.
- Remaining: extract installed-hook ownership in `hook_set.*`.
- Deferred: resolver transactionality, rollback, failure pass-through and
  shutdown semantics remain Batch 3 work.
- Blocked: none for scanner/validator extraction.
- Not runtime-validated: in-game signature resolution and behavior after this
  structural extraction.
- Patch summary: separated discovery and instruction-shape validation from
  the runtime coordinator while preserving existing resolver control flow.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.3c hook ownership extraction

- Scope: move installed `SafetyHookMid` object ownership into `hooks::HookSet`
  without changing installation order, partial-failure paths, reset order or
  shutdown behavior.
- Paths changed: added `src/hooks/hook_set.*`; updated
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  existing runtime references remain aliases to the same hook objects and
  existing reset calls remain in their original order.
- Completed: gameplay, cinematic, dialogue and optional research-trace hook
  objects now have a dedicated `hooks::HookSet` owner.
- Remaining: complete Batch 2 domain decomposition and later review the
  coordinator boundaries.
- Deferred: transactional installation, rollback, shutdown synchronization
  and destruction semantics remain Batch 3 work.
- Blocked: none for hook ownership extraction.
- Not runtime-validated: in-game hook resolution and behavior after this
  structural extraction.
- Patch summary: completed the hooks ownership boundary while preserving the
  existing coordinator control flow.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.4a gameplay state extraction

- Scope: move gameplay transition-state types and their names into the
  gameplay domain without changing the state machine or camera-hook logic.
- Paths changed: added `src/gameplay/gameplay_state.*`; updated
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  `CoordinatorState`, `ReplayState` and their string representations now
  resolve through the gameplay module.
- Completed: gameplay transition state has an explicit domain owner.
- Remaining: extract gameplay camera resolver/behavior and continue the
  cinematic and dialogue domain splits.
- Deferred: gameplay lifecycle and atomicity safety changes remain Batch 3
  work.
- Blocked: none for gameplay state extraction.
- Not runtime-validated: in-game transition behavior after this structural
  extraction.
- Patch summary: established the first gameplay domain boundary while
  preserving existing coordinator and replay semantics.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.4b gameplay camera extraction

- Scope: move gameplay camera-writer discovery and structural validation into
  the gameplay domain while preserving the existing resolver contract and
  transition behavior.
- Paths changed: added `src/gameplay/gameplay_camera.*`; updated
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root.
  The runtime coordinator still owns only the result assignment and existing
  telemetry; gameplay camera matching remains unique-match and fail-closed.
- Completed: `.text` discovery, camera-writer signature matching and MOVSS
  operand validation now have an explicit gameplay-camera owner.
- Remaining: extract gameplay camera transition/application behavior and then
  continue cinematic and dialogue domain extraction.
- Deferred: atomicity, replay, lifecycle and failure-semantics changes remain
  Batch 3 work.
- Blocked: none for camera resolver extraction.
- Not runtime-validated: in-game resolver resolution and gameplay behavior
  after this structural extraction.
- Patch summary: moved the validated camera-writer resolver out of the
  runtime coordinator without changing its acceptance or refusal conditions.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.4c gameplay camera application primitive

- Scope: move the validated aspect/flags memory-write primitive into the
  gameplay camera domain without changing atomic state transitions, logging,
  replay behavior or post-cinematic handoff control flow.
- Paths changed: updated `src/gameplay/gameplay_camera.*`,
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root.
  The runtime coordinator still owns atomic state application and telemetry;
  the gameplay module owns the validated write primitive and camera resolver.
- Completed: gameplay camera resolver and aspect/flags write primitive now
  have an explicit gameplay owner.
- Remaining: the atomic transition/replay coordinator remains coupled to
  lifecycle state and needs a separate bounded extraction before gameplay can
  close.
- Deferred: atomicity, replay, lifecycle and failure-semantics changes remain
  Batch 3 work.
- Blocked: none for this bounded application extraction.
- Not runtime-validated: in-game transition behavior after this structural
  extraction.
- Patch summary: extended gameplay camera ownership to the low-level write
  primitive while preserving coordinator semantics.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.5a cinematic FOV math extraction

- Scope: move the pure cinematic Hor+ projection calculation into the
  cinematics domain without changing ENTER/EXIT hook or coordinator behavior.
- Paths changed: added `src/cinematics/cinematic_fov.*`; updated
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  the runtime call site now delegates to `cinematics::HorPlus` with the same
  inputs and native-aspect reference.
- Completed: pure cinematic FOV transformation has an explicit domain owner.
- Remaining: cinematic resolver, aspect-store behavior and ENTER/EXIT hook
  coordination remain for later cinematic sub-batches.
- Deferred: lifecycle, handoff and failure-semantics changes remain Batch 3
  work.
- Blocked: none for pure cinematic math extraction.
- Not runtime-validated: in-game cinematic behavior after this structural
  extraction.
- Patch summary: isolated the validated Hor+ calculation while preserving
  cinematic hook control flow.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.5b cinematic aspect ownership

- Scope: move cinematic aspect policy branching into the cinematics domain
  while preserving viewport probing, aspect-store behavior and cross-domain
  ENTER/EXIT coordination.
- Paths changed: added `src/cinematics/cinematic_aspect.*`; updated
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  runtime delegates policy selection to `cinematics::ResolveAspect` and keeps
  the existing Auto resolver callback and aspect constants.
- Completed: Native/Auto/16:9/21:9/32:9 policy selection now has an explicit
  cinematic owner.
- Remaining: cinematic aspect-store resolver/application and ENTER/EXIT hook
  coordination remain for later sub-batches.
- Deferred: lifecycle, handoff and failure-semantics changes remain Batch 3
  work.
- Blocked: none for cinematic aspect policy extraction.
- Not runtime-validated: in-game cinematic behavior after this structural
  extraction.
- Patch summary: separated cinematic policy resolution from the runtime
  coordinator without changing the selected policy or Auto callback path.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.5c cinematic aspect-store resolver

- Scope: move cinematic aspect-store candidate discovery and structural
  validation into the cinematics domain without changing hook installation,
  original-byte handling or aspect-store application behavior.
- Paths changed: updated `src/cinematics/cinematic_aspect.*`,
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  runtime retains the same unique-match logging, validated store assignment
  and subsequent hook installation path.
- Completed: cinematic aspect-store resolution has an explicit domain owner.
- Remaining: aspect-store hook application and ENTER/EXIT coordination remain
  cross-domain runtime responsibilities.
- Deferred: hook transactionality, rollback, lifecycle and failure-semantics
  changes remain Batch 3 work.
- Blocked: none for aspect-store resolver extraction.
- Not runtime-validated: in-game aspect-store resolution and cinematic behavior
  after this structural extraction.
- Patch summary: separated cinematic store discovery/validation from runtime
  orchestration while preserving the existing acceptance conditions.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.5d cinematic aspect application extraction

- Scope: move cinematic aspect-store application into the cinematics domain
  while preserving hook-context continuation, hook ownership, original-byte
  handling and lifecycle semantics.
- Paths changed: updated `src/cinematics/cinematic_aspect.*`,
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  runtime still owns `SafetyHookContext.rip` continuation, logging and hook
  lifecycle while the cinematic module performs the validated memory write.
- Completed: policy, aspect-store resolver and aspect-store application now
  have explicit cinematic-domain owners.
- Remaining: ENTER/EXIT hook execution and gameplay recovery handoff remain
  cross-domain runtime responsibilities.
- Deferred: hook transactionality, rollback, lifecycle and failure-semantics
  changes remain Batch 3 work.
- Blocked: none for this bounded application extraction.
- Not runtime-validated: in-game aspect-store behavior after this structural
  extraction.
- Patch summary: separated cinematic aspect application from hook-context and
  lifecycle coordination without changing the write/refusal conditions.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.5e cinematic ENTER FOV behavior extraction

- Scope: move validated cinematic ENTER FOV input validation and Hor+ transform
  behavior into `cinematic_fov.*` without changing lifecycle state writes,
  trace arming or cross-domain recovery coordination.
- Paths changed: updated `src/cinematics/cinematic_fov.*`,
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  runtime still owns ENTER/EXIT state, logging and gameplay coordination while
  cinematic FOV validation/transformation is delegated to the domain module.
- Completed: ENTER authored-FOV validation and Hor+ transformation now have an
  explicit cinematic owner.
- Remaining: EXIT state, trace lifecycle and gameplay recovery handoff remain
  cross-domain runtime responsibilities.
- Deferred: lifecycle, handoff and failure-semantics changes remain Batch 3
  work.
- Blocked: none for this bounded ENTER behavior extraction.
- Not runtime-validated: in-game cinematic ENTER/EXIT behavior after this
  structural extraction.
- Patch summary: isolated cinematic ENTER FOV behavior while preserving the
  existing hook context mutation conditions.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.6a dialogue FOV policy extraction

- Scope: move dialogue projection transforms and Adaptive/Reduced target
  calculations into the dialogue domain without changing dialogue phase state,
  boundary-hook behavior or cinematic/gameplay coordination.
- Paths changed: added `src/dialogue/dialogue_fov.*`; updated
  `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  runtime still owns phase tracking, policy gating, boundary hook context and
  lifecycle logging while dialogue math is delegated to the domain module.
- Completed: projection-sample transforms and Adaptive/Reduced target policy
  calculations now have an explicit dialogue owner.
- Remaining: dialogue state, boundary resolver/install and cross-domain
  lifecycle integration remain for later sub-batches.
- Deferred: lifecycle, hook transactionality and failure-semantics changes
  remain Batch 3 work.
- Blocked: none for dialogue FOV policy extraction.
- Not runtime-validated: in-game dialogue behavior after this structural
  extraction.
- Patch summary: isolated dialogue FOV policy/math while preserving the
  existing phase machine and hook control flow.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.6b dialogue state ownership extraction

- Scope: move dialogue phase ownership and phase-name representation out of
  configuration and into the dialogue domain without changing phase
  transitions, locking or boundary-hook control flow.
- Paths changed: added `src/dialogue/dialogue_state.*`; updated
  `src/config/feature_config.hpp`, `src/plugin/runtime.cpp` and `build.cmd`.
- Git state: branch `main`; unrelated dirty object files, documentation and
  research artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  runtime state variables and existing phase references now use the dialogue
  domain phase type and name function.
- Completed: `DialoguePhase` is no longer a configuration-model type; phase
  state and its representation have an explicit dialogue owner.
- Remaining: dialogue boundary resolver/install and cross-domain lifecycle
  integration remain for later sub-batches.
- Deferred: lifecycle, hook transactionality and failure-semantics changes
  remain Batch 3 work.
- Blocked: none for dialogue state ownership extraction.
- Not runtime-validated: in-game dialogue phase behavior after this structural
  extraction.
- Patch summary: corrected dialogue phase ownership while preserving the
  existing state machine and callback flow.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.7a Win32 memory and SHA-256 extraction

- Scope: move reusable Win32 memory-safety and executable SHA-256 mechanisms
  into `platform/win32` while leaving hotkey, viewport-monitor and thread
  lifecycle ownership in `plugin::Runtime`.
- Paths changed: added `src/platform/win32/memory.*` and
  `src/platform/win32/sha256.*`; updated `src/plugin/runtime.cpp` and
  `build.cmd`.
- Git state: branch `main`; unrelated dirty research, documentation and
  generated artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  runtime wrappers preserve the existing call sites and semantics.
- Completed: `IsWritable`, safe memory reads and `ComputeSha256` now have
  explicit Win32 infrastructure owners.
- Remaining: window/viewport helpers remain for a separate bounded extraction;
  Runtime still owns monitor/hotkey loops and lifecycle coordination.
- Deferred: thread shutdown, loop lifecycle, hook failure semantics and other
  safety-sensitive changes remain Batch 3 work.
- Blocked: none for this bounded extraction.
- Not runtime-validated: in-game behavior after this structural extraction.
- Patch summary: isolated Win32 memory and hashing mechanisms behind thin
  Runtime wrappers without changing lifecycle or feature control flow.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2B.7b Win32 window and viewport extraction

- Scope: move current-process window discovery and client/display aspect
  acquisition into `platform/win32` while leaving monitor-loop scheduling,
  hotkey handling and thread lifecycle in `plugin::Runtime`.
- Paths changed: added `src/platform/win32/window.*` and
  `src/platform/win32/viewport.*`; updated `src/plugin/runtime.cpp` and
  `build.cmd`.
- Git state: branch `main`; unrelated dirty research, documentation and
  generated artifacts were preserved; no Git state-changing operation was run.
- Validation: `build.cmd` completed successfully from the repository root;
  the existing Auto-aspect fallback and diagnostic monitor control flow remain
  in Runtime while Win32 acquisition is delegated to the platform layer.
- Completed: current-process window lookup and client/display aspect reading
  now have explicit Win32 infrastructure owners.
- Remaining: a short Runtime remainder review is still needed before deciding
  whether `platform/win32` is structurally complete.
- Deferred: monitor/hotkey loop lifecycle, shutdown and other safety-sensitive
  semantics remain Batch 3 work.
- Blocked: none for this bounded extraction.
- Not runtime-validated: in-game aspect switching and Auto policy after this
  structural extraction.
- Patch summary: isolated Win32 window/viewport mechanisms without changing
  when Runtime samples them or how it responds to changes.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2 platform/win32 closure review

- Scope: perform the bounded post-extraction inventory and close the Win32
  adapter work without moving lifecycle-sensitive loops or diagnostic control
  flow.
- Review result: no clean Win32 primitive with an obviously wrong owner remains
  in `src/plugin/runtime.cpp`.
- Remaining direct Win32 calls are justified by plugin integration or
  diagnostics: module identity, hotkey polling, display/window observation,
  sleep scheduling and thread creation/cleanup.
- Validation: the preceding `build.cmd` run passed; the inventory was
  read-only and `diff --check` remained clean.
- Completed: `platform/win32` is structurally complete for Batch 2.
- Remaining: decide whether the current `plugin::Runtime` is sufficient as the
  cross-domain coordinator; no forced coordinator extraction is implied.
- Deferred: thread lifecycle, shutdown, diagnostic-loop semantics and related
  safety findings remain Batch 3 work.
- Blocked: none.
- Not runtime-validated: the refactored plugin remains pending full in-game
  regression validation.
- Patch summary: closed the platform ownership boundary based on evidence
  rather than moving lifecycle code for directory symmetry.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 2 final Runtime ownership review

- Scope: review the remaining Runtime responsibilities after domain and Win32
  extraction; decide whether a separate cross-domain coordinator has a
  sufficient independent state/API boundary.
- Decision: `KEEP`. `plugin::Runtime` remains the composition root and
  cross-domain coordinator for this phase.
- Evidence: the remaining coordinator state is interleaved with
  `SafetyHookContext`, installed-hook ownership, original-byte restoration,
  module/config/logger lifetime, thread startup and shutdown. Extracting it
  would require passing the same Runtime machinery through callbacks rather
  than creating an independent abstraction.
- Validation: read-only symbol/state inventory; no source behavior was changed
  by this review. Previous Batch 2 build passed; full runtime regression remains
  deferred.
- Completed: no additional coordinator layer is justified by current
  ownership boundaries; Batch 2 structural scope is complete.
- Remaining: Batch 3 safety/control-flow review and Batch 4 in-game regression.
- Deferred: transactional hook changes, thread shutdown semantics, failed-write
  continuation and other audit findings remain outside Batch 2.
- Blocked: none.
- Not runtime-validated: the post-refactor plugin still requires the planned
  in-game regression matrix.
- Patch summary: documented the evidence-based `KEEP` decision instead of
  introducing an artificial coordinator solely to reduce Runtime LOC.
- Changelog summary: none; intermediate refactor batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 3.1 thread and shutdown lifecycle classification

- Scope: inspect thread creation, loop termination, handle ownership,
  `DllMain` detach behavior and `Shutdown` control flow without changing
  runtime semantics.
- Evidence: `HotkeyLoop` and `ResolutionMonitorLoop` have no stop signal;
  their thread handles are closed immediately after creation; `DllMain`
  invokes `plugin::Shutdown()` during `DLL_PROCESS_DETACH`, where hook reset and
  aspect restoration occur.
- Classification: confirmed lifecycle-safety candidates requiring a bounded
  control-flow decision; not yet classified as an implementation fix until
  shutdown/unload behavior and safe-failure requirements are reviewed.
- Paths reviewed: `src/plugin/dll_entry.cpp`, `src/plugin/runtime.cpp`, and
  `src/plugin/runtime.hpp`.
- Validation: read-only source/control-flow inspection; no build or runtime
  behavior was changed by this audit step.
- Completed: thread/shutdown candidates are isolated from the remaining Batch 3
  audit instead of being mixed with hook rollback or config persistence.
- Remaining: define safe stop/join/ownership semantics and loader-lock-safe
  cleanup before implementation.
- Deferred: hook rollback, failed-write continuation and persistence failure
  paths remain separate Batch 3 batches.
- Blocked: no technical blocker; implementation awaits the bounded lifecycle
  design and validation scope.
- Not runtime-validated: this is classification only.
- Patch summary: recorded lifecycle evidence without applying speculative
  thread or shutdown fixes.
- Changelog summary: none; audit step, not a release change.

## 2026-09-14 — v1.0.0 Batch 3.1 thread and shutdown design

- Scope: define ownership, stop protocol, join boundary and detach semantics
  for the lifecycle candidates identified by the preceding audit.
- Paths changed: added
  `research/reports/V1_0_0_BATCH3_THREAD_SHUTDOWN_DESIGN.md`.
- Design result: controlled shutdown and detach-safe notification are separate
  operations; Runtime owns worker state and handles; joins are forbidden from
  `DllMain` and from worker threads; process-termination detach skips complex
  teardown.
- Validation: design review only; no source, build or runtime behavior was
  changed.
- Completed: implementation contract for the next lifecycle batch.
- Remaining: implement the contract in a bounded source batch and validate
  startup, stop, join and detach behavior.
- Deferred: hook rollback, failed-write continuation and persistence failure
  paths remain separate Batch 3 batches.
- Blocked: normal controlled unload still lacks an external non-loader-lock
  owner; this is an implementation boundary, not silently resolved here.
- Not runtime-validated: design only.
- Patch summary: separated controlled shutdown from minimal detach-safe
  behavior before any lifecycle code changes.
- Changelog summary: none; design step, not a release change.
## 2026-09-14 — v1.0.0 Batch 3.1 thread lifecycle implementation

- Scope: implement the approved worker ownership, interruptible stop,
  partial-start cleanup, controlled join path and minimal detach notification.
- Paths changed: updated `src/plugin/runtime.cpp`,
  `src/plugin/runtime.hpp` and `src/plugin/dll_entry.cpp`.
- Implementation: Runtime now owns worker handles and a manual-reset stop
  event; worker loops use interruptible waits; startup failures stop and join
  already-created workers; controlled `Shutdown()` joins workers before hook
  reset/aspect restore; self-join is refused.
- Detach behavior: `DllMain` calls only `NotifyProcessDetach`; process
  termination skips teardown, while normal detach only signals the stop event.
- Validation: `build.cmd` completed successfully; static call-path review
  confirmed detach does not call `Shutdown`, join, hook reset or aspect restore.
  `diff --check` passed.
- Completed: controlled shutdown semantics and worker handle ownership are
  implemented.
- Remaining: targeted runtime validation and a separate decision on safe
  normal-unload ownership; normal unload is not claimed as fully supported.
- Deferred: hook rollback transactionality, failed-write continuation,
  persistence fallback and feature behavior remain separate Batch 3 batches.
- Blocked: no build blocker; runtime validation remains outstanding.
- Not runtime-validated: worker stop timing, controlled unload and process
  detach behavior were not exercised in-game.
- Patch summary: replaced detached infinite workers with Runtime-owned,
  interruptible workers and separated controlled shutdown from DllMain detach.
- Changelog summary: none; safety implementation batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 3.1 validation gate result

- Status: `IMPLEMENTED / VALIDATION PARTIAL`; Batch 3.1 is not marked fully
  complete yet.
- Validation performed: source inventory confirmed there is no `tests/` tree or
  existing lifecycle harness; the implementation build passed and the static
  detach call-path review passed.
- Not validated: controlled stop wake-up, idempotency, partial-start cleanup,
  self-join refusal and normal worker startup under a dedicated harness or
  targeted runtime owner.
- Decision: do not create an artificial game hotkey or broad runtime probe just
  to claim the gate; either add a bounded lifecycle harness in a later test
  batch or perform an explicitly scoped external runtime validation.
- Remaining: targeted lifecycle validation; normal unload remains unsupported
  as a claimed feature.
## 2026-09-14 — v1.0.0 Batch 3.1 lifecycle harness validation

- Scope: validate the real `WorkerLifecycle` production implementation without
  loading the DLL, hooks or game process.
- Paths changed: added `tests/lifecycle/worker_lifecycle_harness.cpp`; added
  `src/plugin/worker_lifecycle.*` as the production lifecycle owner; updated
  `src/plugin/runtime.cpp`, `src/plugin/runtime.hpp` and `build.cmd`.
- Harness result: normal startup, controlled stop, idempotent stop, partial
  startup failure cleanup, self-join refusal and stop-before-start all passed.
- Validation: harness compiled against the real production
  `worker_lifecycle.cpp` and exited successfully; main `build.cmd` also passed;
  static detach review and `diff --check` passed.
- Completed: Batch 3.1 lifecycle implementation and targeted harness gate.
- Remaining: full in-game regression remains Batch 4; normal DLL unload is not
  claimed as a supported feature without an external non-loader-lock owner.
- Deferred: hook rollback, failed-write continuation, persistence fallback and
  other safety clusters remain separate Batch 3 work.
- Blocked: none for this batch.
- Not runtime-validated: no game-process load/unload or feature regression was
  performed; the harness validates lifecycle mechanics only.
- Patch summary: extracted the tested worker lifecycle primitive and validated
  its production behavior across H1-H6 scenarios.
- Changelog summary: none; safety implementation batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 3.1 loader-lock post-validation correction

- Scope: review static destruction after harness integration and remove any
  implicit blocking cleanup path from the global lifecycle owner.
- Finding: an automatic `WorkerLifecycle` destructor join would have recreated
  the loader-lock teardown risk during DLL destruction.
- Change: `WorkerLifecycle` now has a trivial destructor; worker cleanup remains
  explicit through controlled `Shutdown()` only.
- Validation: `build.cmd` passed and the H1-H6 lifecycle harness passed again;
  static review confirms `DllMain` still reaches only the signal-only detach
  path.
- Completed: Batch 3.1 implementation and targeted harness validation are
  complete with the loader-lock correction applied.
- Remaining: normal DLL unload remains explicitly unsupported as a claimed
  feature; full in-game regression remains Batch 4.
- Deferred: other Batch 3 safety clusters remain untouched.
- Blocked: none.
- Not runtime-validated: no game-process unload/reload was performed.
- Patch summary: removed implicit destructor join so controlled cleanup cannot
  be reintroduced through static object destruction.
- Changelog summary: none; safety implementation correction, not a release
  change.
## 2026-09-14 — v1.0.0 Batch 3 testing invariant

- Stable test principle: tests validate observable behavioral contracts and
  safe outcomes, not private members, exact HANDLE layout, synchronization
  primitives, call sequence or source/module organization.
- The architecture may change while the feature contracts remain stable;
  implementation details are replaceable unless they are themselves an
  explicit safety contract.
- Batch 3.1 lifecycle harness is retained as evidence because it exercises the
  production lifecycle behavior, not because its current class structure is a
  permanent API.
- Scope: principle applies to later gameplay, cinematic, dialogue, config and
  resolver tests.
- Validation: documentation-only decision; no source or runtime behavior
  changed.
## 2026-09-14 — v1.0.0 Batch 3.2 initialization and hook rollback audit

- Scope: classify initialization ordering, hook creation failures and partial
  rollback without changing installation semantics.
- Evidence: the `std::exception` initialization path stops workers and resets
  dialogue/cinematic hooks plus aspect state; the catch-all path stops workers
  but does not repeat the hook/aspect cleanup.
- Classification: confirmed partial-initialization rollback candidate; the
  catch-all path can leave previously-created hook state after an unknown
  initialization failure. This remains an audit finding, not yet a fix.
- Additional behavior observed: dialogue-boundary failure is intentionally
  downgraded to native policy; optional research trace failures are logged and
  do not abort production initialization.
- Paths reviewed: `src/plugin/runtime.cpp`, `src/hooks/hook_set.*` and
  `src/plugin/dll_entry.cpp`.
- Validation: read-only control-flow inspection; no source or behavior changed
  by this audit step.
- Completed: initialization failure branches and rollback asymmetry are
  isolated from the already-closed worker lifecycle batch.
- Remaining: design the smallest rollback correction and behavioral harness
  contract before changing code.
- Deferred: cinematic failed-write continuation and config persistence failure
  paths remain separate audit clusters.
- Blocked: none.
- Not runtime-validated: audit classification only.
- Patch summary: recorded the rollback asymmetry without accepting every audit
  warning as an automatic fix.
- Changelog summary: none; audit step, not a release change.
## 2026-09-14 — v1.0.0 Batch 3.2 initialization failure inventory

- Scope: inventory every bounded failure path reachable during `Initialize`,
  including acquired state, current handling and feature-local versus
  runtime-fatal classification.
- Paths changed: added
  `research/reports/V1_0_0_BATCH3_2_INITIALIZATION_FAILURE_INVENTORY.md`.
- Result: configuration, dialogue-boundary, optional trace and worker-control
  failures have recoverable/feature-local candidates; cinematic, gameplay and
  optional-worker failures currently use the outer fatal-style path even where
  independent continuation may be safe; unknown ownership after a non-standard
  exception remains the strongest fatal candidate.
- Validation: read-only control-flow/source inventory; no catch block, hook
  ordering or feature behavior changed.
- Completed: actual acquisition order and failure-path classifications are
  documented before rollback design.
- Remaining: define separate feature-local and runtime-fatal behavioral
  contracts, then decide whether the catch asymmetry is a defect.
- Deferred: implementation, cinematic failed-write path and config persistence
  remain untouched.
- Blocked: none.
- Not runtime-validated: inventory only.
- Patch summary: replaced the assumption of all-or-nothing rollback with an
  evidence-based failure inventory that preserves graceful-degradation as a
  first-class candidate.
- Changelog summary: none; audit step, not a release change.
## 2026-09-14 — v1.0.0 Batch 3.2 dialogue dependency clarification

- Evidence update: Gameplay is not established as a hard Dialogue
  initialization prerequisite. It is a soft presentation/quality dependency:
  an incorrect underlying gameplay FOV can make dialogue framing feel worse,
  while disabled dialogue scaling may hide that impact.
- Contract impact: `Gameplay=OFF` remains a valid configuration for Dialogue;
  the dependency graph must distinguish technical initialization dependencies
  from visual-quality dependencies.
- Validation: documentation/control-flow model update only; no production
  behavior changed.
- Remaining: finalize configuration-dependent contracts before any rollback or
  catch-block implementation.
## 2026-09-14 — v1.0.0 Batch 3.2 final feature-dependency classification

- Final model: Gameplay, Cinematics and Dialogue are technically independent
  user-facing features. Their absence can affect ultrawide presentation quality
  but does not create a hard initialization dependency for the other features.
- Contract: `DISABLED` and `FAILED` remain distinct states; either one normally
  disables or falls back only the affected feature. Runtime-fatal status is
  reserved for shared infrastructure or uncertain ownership state.
- Classification result: config, dialogue and trace failures already follow
  local/native fallback; gameplay hook, cinematic hook and optional worker
  startup currently use fatal-style outer handling and are concrete behavior
  candidates for the next design/implementation step.
- Validation: documentation and control-flow classification only; no
  production behavior changed.
- Completed: Batch 3.2 dependency model and failure classification.
- Remaining: define the smallest local-fallback implementation for the
  concrete candidates before changing catch paths or initialization flow.
- Deferred: cinematic failed-write continuation and config persistence remain
  separate Batch 3 clusters.
- Blocked: none.
- Not runtime-validated: classification only.
- Patch summary: removed the conditional-core assumption and formalized
  independent feature graceful degradation.
- Changelog summary: none; audit step, not a release change.
## 2026-09-14 — v1.0.0 Batch 3.2 local failure rollback design

- Scope: define the mechanics for feature-local failure handling and the
  shared-fatal boundary without modifying production behavior.
- Paths changed: added
  `research/reports/V1_0_0_BATCH3_2_LOCAL_FAILURE_ROLLBACK_DESIGN.md`.
- Design result: feature failures roll back only their own resources, mark the
  feature `FAILED`/unavailable, log a precise reason and allow independent
  initialization to continue; shared-fatal handling stops initialization and
  cleans shared ownership in a known fail-closed state.
- Invariants: `DISABLED` remains distinct from `FAILED`; local rollback cannot
  touch another feature; outer catch is only a last-resort shared-fatal boundary.
- Validation: design review only; no catch block, hook ordering or feature
  behavior changed.
- Completed: implementation contract for the next local-failure batch.
- Remaining: implement bounded feature-local initialization scopes and validate
  observable graceful-degradation outcomes.
- Deferred: cinematic failed-write continuation and config persistence remain
  separate safety clusters.
- Blocked: none.
- Not runtime-validated: design only.
- Patch summary: converted the dependency model into explicit local versus
  shared-fatal control-flow contracts.
- Changelog summary: none; design step, not a release change.
## 2026-09-14 — v1.0.0 Batch 3.2 local failure implementation and validation

- Scope: implement bounded feature-local initialization handling and retain a
  shared-fatal boundary without changing resolver semantics, hook ordering,
  policies or configuration defaults.
- Paths changed: added `src/plugin/feature_status.*` and
  `tests/feature_status/feature_status_harness.cpp`; updated
  `src/plugin/runtime.cpp`, `src/plugin/runtime.hpp` and `build.cmd`.
- Implementation: Gameplay, cinematic aspect/FOV and Dialogue setup failures
  now reset only their owned hooks/state and continue; hotkey and diagnostic
  worker failures remain local; feature status is logged as `DISABLED`,
  `AVAILABLE` or `FAILED`; failed Gameplay no longer arms post-cinematic
  gameplay handoff state.
- Shared-fatal path: unexpected failures in shared initialization use one
  cleanup path for workers, hooks and aspect state; standard and unknown
  exceptions now produce the same cleanup result.
- Validation: production `build.cmd` passed; lifecycle harness passed; feature
  status/graceful-degradation harness passed; `diff --check` passed.
- Harness results: disabled distinction, independent feature failure and
  `Gameplay=OFF` with Cinematics/Dialogue available all passed.
- Completed: Batch 3.2 implementation and bounded behavioral validation.
- Remaining: in-game regression of local failure behavior remains Batch 4;
  normal DLL unload remains unclaimed.
- Deferred: cinematic failed-write continuation and config persistence remain
  separate Batch 3 clusters.
- Blocked: none.
- Not runtime-validated: no injected game hook failure was exercised; harnesses
  validate the production status/lifecycle contracts without loading the game.
- Patch summary: replaced feature-failure escalation through the global catch
  with ownership-bounded fallback and explicit observable feature statuses.
- Changelog summary: none; safety implementation batch, not a release change.
## 2026-09-14 — v1.0.0 Batch 3.3 cinematic failed-write classification

- Scope: classify the cinematic aspect-store failed-write and `ctx.rip`
  continuation path without changing hook behavior.
- Paths changed: added
  `research/reports/V1_0_0_BATCH3_3_FAILED_WRITE_CLASSIFICATION.md`.
- Result: the current path is classified as intentional fail-closed behavior;
  invalid/unwritable targets skip the custom and native store and resume after
  the original ten-byte MOV encoding. No evidence proves native pass-through is
  safe or required.
- Classification: `INTENTIONAL BEHAVIOR`, not `CONFIRMED DEFECT`.
- Validation: bounded static control-flow and memory-validation review; no
  source, hook continuation or feature behavior changed.
- Completed: failed-write audit candidate classified and removed from the
  automatic-fix queue.
- Remaining: config persistence failure path remains the next unclassified
  Batch 3 candidate.
- Deferred: no cinematic write change is justified without contradictory runtime
  evidence.
- Blocked: none.
- Not runtime-validated: classification only.
- Patch summary: preserved the guarded fail-closed continuation and documented
  why the old pass-through finding is not a confirmed defect.
- Changelog summary: none; audit step, not a release change.

## 2026-09-15 — v1.0.0 remove Gameplay-disabled camera observer

- Scope: implement approved Design D only; no observer rewrite, throttling,
  XMM0 aspect derivation or unrelated feature changes.
- Paths changed: `src/plugin/runtime.cpp`; updated the observer design audit
  and final safety verification report; added the bounded implementation plan
  `backlog/active/V1_0_0_REMOVE_GAMEPLAY_DISABLED_OBSERVER_TASK_PLAN.md`.
- Implementation: the shared camera-writer hook is installed only when
  `Gameplay.Enabled=true`; the previous read-only observer is bypassed when
  Gameplay is disabled, including the `Cinematics=Auto` configuration.
- Preserved: cinematic aspect-store and FOV hooks, Auto viewport resolution,
  Gameplay-enabled correction/handoff and Dialogue behavior.
- Validation: production ASI build passed; WorkerLifecycle harness passed;
  FeatureStatus harness passed; ConfigPersistence harness passed;
  `git diff --check` passed; static install-condition review passed; one
  targeted user-run passed.
- Completed: bounded source implementation and targeted no-regression check.
- Remaining: full Batch 4 production regression and final release gate.
- Deferred: `helper.hpp` architecture/code-quality review.
- Blocked: none.
- Not runtime-validated: no causal proof that the former observer caused the
  earlier transient stutter.
- Patch summary: removed an unnecessary continuous camera-writer observer
  from the Gameplay-disabled path.
- Changelog summary: reduced unnecessary hot-path observation without changing
  enabled Gameplay or cinematic policy behavior.

## 2026-09-15 — v1.0.0 final architecture hardening A1–A4

- Scope: implement the approved final architecture hardening worklist without
  adding features, reopening deferred research or changing validated gameplay,
  cinematic, dialogue or resolver behavior.
- Paths changed: `src/plugin/runtime.cpp`, `src/hooks/signature_scanner.cpp`,
  `build.cmd`, `.gitignore`, `README.md`, `docs/ARCHITECTURE.md`,
  `docs/SAFETY_INVARIANTS.md`, `docs/UPDATING_GAME_VERSION.md` and this task
  record.
- A1 completed: runtime mutable state now has an explicit `RuntimeState`
  owner while preserving existing callback aliases and control flow. The ASI
  lifetime contract is documented as process-lifetime usage; normal dynamic
  unload remains unclaimed.
- A2 completed: `build.cmd` discovers the supported Visual Studio installation
  through `vswhere`, selects the documented MSVC 17.14+ toolchain and uses
  `/std:c++latest` for the required C++23-era facilities. Stable production
  behavior is no longer enabled by the test-named atomicity defines; remaining
  test macros are confined to the diagnostic/deferred research path.
- A3 completed: ignore rules now cover generated object, executable and tool
  build output. The approved generated artifacts were physically removed and
  untracked from the root, `build-artifacts/obj` and
  `research/tools/CUE4ParseWVF/bin|obj`. Preserved release archives were
  reviewed and left intact.
- A4 completed: scanner pattern/section helpers were isolated into
  `signature_scanner.cpp` without changing scan semantics; `runtime.cpp` no
  longer includes `helper.hpp`. Resolver update workflow, safety invariants and
  architecture/lifetime contracts are documented.
- Validation: production `build.cmd` passed; WorkerLifecycle, FeatureStatus
  and ConfigPersistence harnesses passed; `git diff --check` passed. No game
  launch was performed, so final in-game regression remains outstanding.
- Completed: A1 runtime ownership, A2 build/test separation, bounded A3 ignore
  policy and A4 documentation/helper isolation.
- Remaining: A5 architecture-freeze review and final Batch 4 regression.
- Deferred: normal DLL unload support, transactional HookSet redesign, new
  reflection/viewmodel-FOV work and all other deferred research branches.
- Blocked: none.
- Not runtime-validated: post-hardening in-game behavior has not been rerun.
- Patch summary: made runtime ownership explicit, made the build portable on
  the supported MSVC toolchain, removed stable production reliance on
  test-named switches, isolated the scanner from the legacy helper and added
  release-facing architecture/safety/update documentation.
- Changelog summary: final architecture hardening; no new user-facing feature.

## 2026-09-15 — v1.0.0 final quality hardening: ReadMemory + evidence accuracy

- Scope: implement only the two changes justified by the final Architecture
  Quality Pass: explicit readable-page validation in `ReadMemory()` and
  accurate FeatureStatus harness wording.
- Paths changed: `src/platform/win32/memory.cpp`,
  `tests/platform/memory_harness.cpp` and
  `research/reports/V1_0_0_BATCH3_FINAL_CLOSURE_REVIEW.md`.
- Implementation: `ReadMemory()` now refuses committed pages unless their base
  protection class explicitly supports reading. Existing range, commitment,
  guard and noaccess rejection semantics remain intact; write behavior and all
  callers are unchanged.
- Evidence correction: the Batch 3 closure now identifies FeatureStatus as a
  status-vocabulary harness. It no longer claims direct execution of feature
  initialization, hook rollback or runtime graceful-degradation orchestration.
- Validation: production `build.cmd` passed; the new platform-memory harness
  passed readable, execute-only, noaccess and guard-page cases; WorkerLifecycle,
  FeatureStatus and ConfigPersistence harnesses passed; `git diff --check`
  passed.
- Completed: both quality-pass changes justified for v1.0.
- Remaining: architecture freeze review, then final Batch 4 production
  regression.
- Deferred: all other quality-pass items retain their documented disposition;
  no HookSet, WorkerLifecycle, build-system, resolver or helper refactor was
  introduced.
- Blocked: none.
- Not runtime-validated: no game launch was performed; final in-game
  regression remains pending.
- Patch summary: SafeRead now rejects non-readable committed pages, and
  FeatureStatus evidence claims match the harness's actual scope.
- Changelog summary: internal safety and evidence-accuracy hardening; no
  user-facing behavior change.

## 2026-09-15 — v1.0.0 build object output location

- Scope: move generated production object output from the repository root to
  `build-artifacts/obj` without changing source, runtime behavior or release
  assets.
- Paths changed: `build.cmd`; removed the exact inventoried root-level
  generated `.obj` files.
- Implementation: `build.cmd` now creates `build-artifacts/obj` when needed
  and passes `/Fo` to `cl` so production compilation writes objects there.
- Validation: production `build.cmd` passed; zero root-level `.obj` files
  remained after cleanup; 22 production object files were emitted under
  `build-artifacts/obj`; `.gitignore` excludes both root `.obj` and the build
  artifact directory; `git diff --check` passed.
- Completed: root-level production object pollution removed and future
  production builds use the dedicated ignored output directory.
- Remaining: architecture-freeze review and final Batch 4 regression.
- Deferred: no test-build orchestration or build-system redesign was added.
- Blocked: none.
- Not runtime-validated: no game launch was performed; this batch changes only
  compiler output location.
- Patch summary: redirect compiler object output into `build-artifacts/obj`.
- Changelog summary: cleaner local build layout; no user-facing behavior
  change.
## 2026-09-15 — v1.0.0 final independent review findings resolution

- Scope: resolve bounded A–K findings from the final independent review before
  architecture freeze; no game launch, new feature, reverse engineering or
  broad architecture rewrite.
- Paths changed: `src/plugin/runtime.cpp`,
  `src/plugin/worker_lifecycle.cpp`, `src/plugin/worker_lifecycle.hpp`,
  `src/gameplay/gameplay_camera.cpp`, `src/hooks/signature_scanner.cpp`,
  `src/hooks/signature_scanner.hpp`, `build.cmd`, `test.cmd`,
  `tests/lifecycle/worker_lifecycle_harness.cpp`, project architecture and
  safety docs, supported-build manifest, current hardening reports and this
  record. Three generated harness executables were removed from Git and the
  working tree.
- Implementation: documented the actual translation-unit-private
  `RuntimeState` owner and structural SHA identity contract; made the runtime
  owner deliberate process-resident storage; formalized the single-owner
  worker contract and published handles before resume; added the gameplay
  pointer-addition guard; added `/W4`, unified `test.cmd`, trusted-input
  preconditions and the supported-build manifest.
- Validation: `build.cmd` PASS; `test.cmd` PASS; individual WorkerLifecycle,
  FeatureStatus, ConfigPersistence and PlatformMemory harnesses PASS;
  `git diff --check` PASS; tracked generated `.exe/.obj` count 0; root `.obj`
  count 0. No game launch was performed.
- Completed: A–K findings resolved by the dispositions in
  `research/reports/V1_0_0_FINAL_INDEPENDENT_REVIEW_RESOLUTION.md`.
- Remaining: bounded closure review, then architecture-freeze decision and
  separate Steam 2.0.5 in-game regression.
- Deferred: hot unload, CMake/DI, transactional HookSet, typed resolver
  framework, broad RAII or new feature/research work.
- Blocked: none identified in static/build/harness validation.
- Not runtime-validated: all post-hardening game behavior remains pending the
  user-run Batch 4 regression.
- Patch summary: reconciled ownership/lifetime contracts, improved worker
  publication safety, added the final test/build/repository infrastructure and
  removed misleading generated artifacts.
- Changelog summary: final pre-freeze engineering hardening; no user-facing
  feature change.

## 2026-09-15 — v0.6.0 configuration description update

- Scope: improve the user-facing descriptions in the generated and retained
  `STALKER2CameraTweaks.ini` without changing configuration keys, defaults or
  runtime behavior.
- Paths changed: `src/config/config_repository.cpp`,
  `src/config/config_template.cpp`,
  `tests/config/config_persistence_harness.cpp`, and the local
  `release-assets/STALKER2CameraTweaks.ini` release asset.
- Implementation: made boolean choices explicit; documented Native versus
  forced cinematic framing; added 90°/110° examples for every dialogue policy;
  clarified hotkey behavior and defaults. The harness assertion was updated to
  match the new managed description.
- Validation: production `build.cmd` PASS; WorkerLifecycle, FeatureStatus,
  ConfigPersistence and PlatformMemory harnesses PASS; `git diff --check`
  PASS. No game launch was performed.
- Completed: INI description update.
- Remaining: none within this bounded documentation/template task.
- Deferred: no version bump, README/release metadata update or runtime feature
  change.
- Blocked: none.
- Not runtime-validated: no game launch; this task changes descriptions only.
- Patch summary: clearer user-facing configuration guidance with unchanged
  keys and defaults.
- Changelog summary: improved INI documentation; no user-facing behavior
  change.

## 2026-09-15 — v1.0.0 managed INI template repair

- Scope: make managed configuration synchronization restore missing sections,
  keys and canonical comments while preserving user values and unknown content.
- Paths changed: `src/config/config_template.cpp`,
  `tests/config/config_persistence_harness.cpp`, and this record.
- Implementation: added bounded section/key repair, legacy managed-comment
  recognition, preservation of existing values including invalid values, and
  idempotent synchronization. Existing non-destructive staging/replacement
  semantics remain in place.
- Validation: `build.cmd` PASS; WorkerLifecycle, FeatureStatus,
  ConfigPersistence and PlatformMemory harnesses PASS; template repair and
  idempotence PASS; `git diff --check` PASS. No game launch was performed.
- Completed: missing structure repair, canonical comment restoration, value
  preservation, unknown-content preservation and second-run no-op behavior.
- Remaining: none within this bounded config-template task.
- Deferred: invalid-value rewriting remains intentionally outside scope; the
  existing runtime fallback policy is unchanged.
- Blocked: none.
- Not runtime-validated: no game launch; this is configuration-file behavior.
- Patch summary: configuration synchronization now repairs its managed
  structure without acting as a user-value editor.
- Changelog summary: more robust automatic INI recovery; no feature-policy
  change.

## 2026-09-15 — v1.0.0 architecture freeze declared

- Scope: close the pre-freeze architecture and configuration work after the
  independent A–K review and final managed-INI repair.
- Paths changed: `backlog/TASKLOG.md` only; no production source or runtime
  behavior changes.
- Decision: Feature set, production architecture and configuration contract are
  frozen. Further source changes require a concrete regression or release-
  blocking defect.
- Evidence: A–K independent review disposition approved architecture freeze;
  build, unified tests, focused harnesses, persistence tests, template repair
  and idempotence all passed; `git diff --check` passed.
- Completed: pre-freeze architecture and config hardening.
- Remaining: Batch 4 Steam 2.0.5 in-game regression.
- Deferred: release metadata/documentation finalization until Batch 4 passes.
- Blocked: none in static/build/harness validation.
- Not runtime-validated: no game launch has been performed by the agent.
- Patch summary: declared the frozen v1.0.0 engineering boundary.
- Changelog summary: architecture freeze; no user-facing behavior change.

## 2026-09-15 — remove game-version update tutorial

- Scope: remove the step-by-step future-patch tutorial and retain only the
  compatibility evidence requirement in the supported-build manifest.
- Paths changed: deleted `docs/UPDATING_GAME_VERSION.md` and updated
  `docs/SUPPORTED_BUILD_MANIFEST.md`.
- Implementation: replaced the tutorial link with a concise requirement to
  re-establish executable identity, resolver/instruction/register/offset
  evidence and runtime regression before claiming support.
- Validation: repository search found no remaining current documentation link;
  `git diff --check` passed. No build or game launch was performed.
- Completed: documentation cleanup.
- Remaining: licensing policy decision remains separate; no project license was
  changed in this batch.
- Deferred: no new maintenance tutorial or porting workflow was added.
- Blocked: SafetyHook license provenance requires upstream/license confirmation
  before any repository-wide licensing decision.
- Not runtime-validated: documentation-only change.
- Patch summary: publish the compatibility standard without publishing a
  porting recipe.
- Changelog summary: reduced future-version documentation surface; no runtime
  behavior change.

## 2026-09-15 — v1.0.0 legacy Lyall source cleanup

- Scope: remove only unreachable legacy `src/helper.hpp` and `src/stdafx.h`
  after the completed STALKER2Tweak provenance audit.
- Paths changed: deleted `src/helper.hpp` and `src/stdafx.h`; added the
  bounded task plan and this factual record. No active production source was
  modified.
- Evidence: neither file was compiled by `build.cmd`, included by a production
  translation unit, referenced by `test.cmd`, or referenced by an active
  production/build path after deletion.
- Validation: production `build.cmd` PASS; WorkerLifecycle, FeatureStatus,
  ConfigPersistence and PlatformMemory harnesses PASS; `git diff --check`
  PASS.
- Completed: removed unreachable historical helper/scaffolding from the
  current tree. The completed provenance audit now identifies no
  Lyall/STALKER2Tweak-derived implementation in the built production path.
- Remaining: licensing structure and historical-attribution wording remain
  separate, documentation-only decisions.
- Deferred: no licensing policy, attribution, or historical Git record was
  changed.
- Blocked: none.
- Not runtime-validated: no game launch was performed because behavior was not
  changed.
- Patch summary: removed unused legacy source files.
- Changelog summary: internal production-tree cleanup; no user-facing change.

## 2026-09-15 — Batch 4 cinematic FOV resolver diagnostics

- Scope: add diagnostic logging only to identify the exact cinematic FOV
  resolver validation stage failing in the current build.
- Paths changed: `src/plugin/runtime.cpp`; diagnostic task plan archived under
  `research/completed/`. No resolver predicates, signatures, formulas, hook
  ordering, statuses or runtime behavior were changed.
- Validation: production `build.cmd` PASS; known C4201 warnings remain in
  vendored Zydis; `git diff --check` PASS. No tests or game launch performed.
- Completed: ENTER/EXIT match counts, indexed fallback, decode/operand and
  structural validation, callsite bytes, call targets and target relationship
  are now logged on the next runtime run.
- Remaining: user-controlled runtime run is required to identify the actual
  failed stage.
- Deferred: no restoration patch or cinematic failure-boundary change was
  made.
- Blocked: root cause remains unresolved until the diagnostic log is captured.
- Not runtime-validated: diagnostic output has not yet been observed in-game.
- Patch summary: instrumented the existing cinematic FOV resolver without
  changing selection or validation semantics.
- Changelog summary: added temporary diagnostic evidence for Batch 4
  regression investigation; no feature behavior change intended.

## 2026-09-15 — Batch 4 cinematic FOV resolver execution breadcrumbs

- Scope: add unconditional execution breadcrumbs and exception markers to
  localize the last reached stage of the cinematic FOV resolver.
- Paths changed: `src/plugin/runtime.cpp`; breadcrumb task plan archived under
  `research/completed/`. Existing resolver selection, validation predicates,
  fallback behavior, hook setup and feature semantics were left unchanged by
  this diagnostic step.
- Validation: production `build.cmd` PASS; known C4201 warnings remain in
  vendored Zydis; `git diff --check` PASS. No tests or game launch performed.
- Completed: added entry, scan, validation, call-target and success markers,
  plus standard and unknown exception markers.
- Remaining: user-controlled runtime run is required to capture the last
  emitted breadcrumb and identify the resolver failure stage.
- Deferred: no resolver fix, failure-boundary change or cinematic behavior
  change was made.
- Blocked: root cause remains unresolved until the diagnostic log is captured.
- Not runtime-validated: breadcrumb output has not yet been observed in-game.
- Patch summary: instrumented resolver execution boundaries only.
- Changelog summary: added temporary diagnostic breadcrumbs for Batch 4
  regression investigation; no feature behavior change intended.

## 2026-09-16 — Cinematics FOV initialization diagnostics

- Scope: localize the current cinematic FOV initialization failure with
  boundary markers only.
- Paths changed: `src/plugin/runtime.cpp`; task plan archived under
  `research/completed/`. No cinematic policy, semantic failure-domain,
  resolver, hook-ordering or status-model repair was made.
- Validation: production `build.cmd` PASS; known C4201 warnings remain in
  vendored Zydis; `git diff --check` PASS. No tests or game launch performed.
- Completed: added `CFOV_INIT` markers for boundary entry, resolver result,
  ENTER/EXIT hook creation, successful availability and both exception paths.
- Remaining: user-controlled runtime run is required to identify whether the
  failure occurs in resolution, hook creation or exception handling.
- Deferred: transactional Cinematics boundary repair remains pending until the
  technical failure cause is established.
- Blocked: root cause remains unresolved pending runtime evidence.
- Not runtime-validated: diagnostic output has not been observed in-game.
- Patch summary: instrumented the FOV initialization boundary only.
- Changelog summary: added temporary diagnostics for Batch 4 regression
  investigation; no intended feature behavior change.

## 2026-09-16 — Scanner bad_alloc diagnostics

- Scope: localize the `CinematicEnter` scanner `std::bad_alloc` without
  changing scanner behavior.
- Paths changed: `src/hooks/signature_scanner.cpp`; task plan archived under
  `research/completed/`. No parser, pattern, scan-loop or match-limit repair
  was made.
- Validation: production `build.cmd` PASS; known C4201 warnings remain in
  vendored Zydis; `git diff --check` PASS. No tests or game launch performed.
- Completed: added parser completion/progress markers, executable-section
  bounds, periodic scan counters, match milestones and local bad_alloc stage
  markers with rethrow.
- Remaining: scanner repair and Cinematics transactional failure-domain repair
  remain separate follow-up work.
- Deferred: no scanner repair or Cinematics semantic repair was implemented in
  this diagnostic batch.
- Blocked: none for this diagnostic objective; the repair has not yet been
  authorized or implemented.
- Not runtime-validated: no production behavior validation was performed
  beyond the user-controlled diagnostic run.
- Patch summary: localized `bad_alloc` to parser cursor non-progress during
  the first `CinematicEnter` token.
- Changelog summary: scanner diagnostics established the parser failure path;
  no intended feature behavior change.

## 2026-09-16 — Scanner parser bad_alloc repair

- Scope: repair only the parser cursor handling identified by the scanner
  diagnostic and remove temporary FOV/scanner diagnostics.
- Paths changed: `src/hooks/signature_scanner.cpp`,
  `src/hooks/signature_scanner.hpp`,
  `tests/platform/signature_scanner_harness.cpp`, `test.cmd` and
  `src/plugin/runtime.cpp`; diagnostic task plan archived under
  `research/completed/`.
- Validation: scanner/parser harness PASS; WorkerLifecycle, FeatureStatus,
  ConfigPersistence and PlatformMemory harnesses PASS; production
  `build.cmd` PASS; known C4201 warnings remain in vendored Zydis;
  `git diff --check` PASS. No game launch performed.
- Completed: replaced the invalid parser cursor mutation with a mutable
  `char*` cursor and preserved wildcard/scanner semantics; all current
  production string signatures parse and scan without parser non-progress.
- Remaining: runtime validation of the repaired ASI is required.
- Deferred: the unified Cinematics AspectRatio failure-domain repair remains a
  separate open regression.
- Blocked: none for the bounded scanner repair.
- Not runtime-validated: the repaired ASI has not yet been tested in-game.
- Patch summary: fixed parser forward progress and added bounded production
  signature scanner coverage; removed temporary diagnostics.
- Changelog summary: fixed cinematic FOV resolver startup failure caused by
  parser cursor non-progress; no intended cinematic policy change.

## 2026-09-16 — FindAll stage diagnostics

- Scope: localize the second current `hooks::FindAll` failure after the
  parser forward-progress repair, without changing scanner behavior.
- Paths changed: `src/hooks/signature_scanner.cpp` and
  `src/plugin/runtime.cpp`; task plan archived under
  `research/completed/`.
- Validation: production `build.cmd` PASS; known C4201 warnings remain in
  vendored Zydis; `git diff --check` PASS. Tests and game launch were not
  performed.
- Completed: added bounded diagnostics for pattern parsing completion,
  executable-section enumeration, scan progress, result accumulation,
  return, and exception stages; FOV initialization now records standard or
  unknown exception details before preserving the existing failure behavior.
- Runtime evidence: `CinematicEnter` reaches `FINDALL 00` but does not reach
  `FINDALL 01`; it fails with `bad allocation` at `PatternBytes`. Earlier
  patterns complete scanning with one match and normal return.
- Confirmed second failure stage: `PatternBytes`, before section enumeration.
  The parser still has no explicit end-pointer/progress validation when
  `strtoul` performs no conversion, but the exact triggering character or
  input representation is not established by this log alone.
- Remaining: identify the exact parser input/progress failure and implement
  the minimal repair; no repair was included in this diagnostic batch.
- Deferred: scanner repair, bounds changes, resolver changes and the unified
  Cinematics failure-domain repair remain out of scope.
- Blocked: the exact parser-level cause remains unresolved; production repair
  is intentionally deferred to the next bounded batch.
- Runtime-validated: the second failure stage was observed in the
  user-controlled game run; repair behavior remains unvalidated.
- Patch summary: instrumented `FindAll` and the FOV exception boundary only;
  scanner control flow and cinematic semantics remain unchanged.
- Changelog summary: added temporary scanner-stage diagnostics for Batch 4
  regression investigation; no intended feature behavior change.

## 2026-09-16 — PatternBytes token diagnostics

- Scope: localize the `PatternBytes(CinematicEnter)` failure after the
  second runtime run confirmed that the exception occurs before section
  enumeration.
- Paths changed: `src/hooks/signature_scanner.cpp`; task plan archived under
  `research/completed/`.
- Validation: production `build.cmd` PASS; known C4201 warnings remain in
  vendored Zydis; `git diff --check` PASS. No tests or game launch performed.
- Completed: added bounded runtime-input logging, token offsets, parser
  cursor-before/after diagnostics, wildcard/value reporting, no-progress
  detection, size milestones and allocation-stage markers for the
  `CinematicEnter` parser path.
- Runtime evidence: the actual `CinematicEnter` input is the expected
  92-character pattern. Parsing succeeds through token 3, then reaches
  offset 11 (`0x20`, a space before the first `??` wildcard). `strtoul`
  performs no conversion because the next non-whitespace character is `?`,
  leaves the cursor unchanged, and the parser repeatedly appends `0x00`.
  `bytes.push_back` fails at token `3491081613` with `bytes_size=3491081613`.
- Confirmed root cause: parser does not advance past separators before
  wildcard handling, so a space immediately before `??` produces an
  unbounded no-progress loop.
- Remaining: implement and validate the minimal separator/cursor repair; no
  repair was included in this diagnostic batch.
- Deferred: parser repair, scanner changes, resolver changes and Cinematics
  failure-domain repair remain out of scope.
- Blocked: production repair is intentionally deferred to the next bounded
  implementation batch.
- Runtime-validated: the exact second `PatternBytes` failure and no-progress
  mechanism were observed in the user-controlled game run; repair behavior
  remains unvalidated.
- Patch summary: instrumented parser input and cursor progress only; parser
  control flow and production semantics remain unchanged.
- Changelog summary: added bounded token diagnostics for the remaining
  cinematic FOV scanner regression; no intended feature behavior change.

## 2026-09-16 — PatternBytes separator repair

- Scope: repair only the confirmed separator-to-wildcard parser
  no-progress defect and remove temporary scanner/FOV diagnostics.
- Paths changed: `src/hooks/signature_scanner.cpp`,
  `tests/platform/signature_scanner_harness.cpp`, `src/plugin/runtime.cpp`,
  `test.cmd`; task plan archived under `research/completed/`.
- Validation: scanner/parser harness PASS; WorkerLifecycle, FeatureStatus,
  ConfigPersistence and PlatformMemory harnesses PASS through `test.cmd`;
  production `build.cmd` PASS; known C4201 warnings remain in vendored
  Zydis; `git diff --check` PASS. Game launch was not performed.
- Completed: separators are skipped before token parsing; wildcard and hex
  semantics remain intact; malformed non-convertible input terminates without
  unbounded allocation; all current production string signatures parse and
  scan through the harness; temporary `PATTERN_DIAG`/`FINDALL` diagnostics
  and FOV exception diagnostics were removed.
- Remaining: runtime validation of the repaired ASI is required.
- Deferred: unified Cinematics Aspect/FOV failure-domain repair remains a
  separate open regression and was not changed.
- Blocked: none for the bounded parser repair.
- Not runtime-validated: the repaired production build has not yet been run
  in-game.
- Patch summary: fixed separator-to-wildcard forward progress and added
  bounded mixed-token/malformed-input coverage without changing scanner or
  cinematic semantics.
- Changelog summary: repaired the parser regression that prevented cinematic
  FOV signature resolution; runtime cinematic validation remains pending.

## 2026-09-16 — Cinematics transactional failure domain

- Scope: repair only the confirmed split failure domain for the single
  user-facing `[Cinematics].AspectRatio` policy.
- Paths changed: `src/cinematics/cinematic_initialization.hpp`,
  `src/cinematics/cinematic_initialization.cpp`, `src/plugin/runtime.cpp`,
  `tests/cinematics/cinematic_initialization_harness.cpp`, `build.cmd`,
  `test.cmd`; task plan archived under `research/completed/`.
- Validation: Cinematics transaction harness PASS; WorkerLifecycle,
  FeatureStatus, ConfigPersistence, PlatformMemory and signature scanner
  harnesses PASS through `test.cmd`; production `build.cmd` PASS; known
  C4201 warnings remain in vendored Zydis; `git diff --check` PASS. Game
  launch was not performed.
- Completed: aspect and FOV components now initialize through one transactional
  boundary; any component failure rolls back both component owners and exposes
  one `Cinematics` status; Native bypass remains separate; component source
  files remain separate.
- Remaining: runtime validation of transactional failure behavior is required.
- Deferred: Finding 2 (`Gameplay=false` and `CinematicExiting`) was not changed.
- Blocked: none for the bounded implementation and harness validation.
- Not runtime-validated: the repaired ASI has not yet been tested in-game.
- Patch summary: unified Cinematics ownership, rollback and observable status
  without changing successful aspect/FOV algorithms or hook signatures.
- Changelog summary: prevented partial cinematic initialization from leaving
  aspect or FOV hooks active after a component failure.

## 2026-09-16 — Coordinator recovery without Gameplay hook

- Scope: repair only Finding 2, completing the cinematic coordinator when the
  Gameplay writer hook is unavailable.
- Paths changed: `src/gameplay/gameplay_state.hpp`,
  `src/gameplay/gameplay_state.cpp`, `src/plugin/runtime.cpp`,
  `tests/cinematics/coordinator_recovery_harness.cpp`, `test.cmd`; task plan
  archived under `research/completed/`.
- Validation: coordinator recovery harness PASS; all existing harnesses,
  including the Cinematics transaction harness, PASS through `test.cmd`;
  production `build.cmd` PASS; known C4201 warnings remain in vendored
  Zydis; `git diff --check` PASS. Game launch was not performed.
- Completed: `TraceCinematicExit()` now returns the coordinator directly to
  `Gameplay` and resets dialogue runtime state when `g_gameplayAvailable` is
  false; the available Gameplay path remains `CinematicExiting` with the
  existing atomic handoff.
- Remaining: combined runtime validation of Findings 1 and 2 is required.
- Deferred: Findings 3–6 remain untouched.
- Blocked: none for bounded implementation and harness validation.
- Not runtime-validated: the repaired ASI has not yet been tested in-game.
- Patch summary: removed the permanent dialogue gate caused by waiting for an
  unavailable Gameplay callback, without adding an observer, timer or hook.
- Changelog summary: independent Cinematics and Dialogue recovery now remains
  available when Gameplay correction is disabled or fails to initialize.

## 2026-09-16 — Early callback crash diagnostic

- Scope: diagnostic-only localization of the `Gameplay=false` startup crash;
  no repair or callback behavior change.
- Paths changed: `src/plugin/runtime.cpp`; task plan archived under
  `research/completed/`.
- Validation: production `build.cmd` PASS; known C4201 warnings remain in
  vendored Zydis; `git diff --check` PASS. Game launch was not performed.
- Completed: bounded entry/return breadcrumbs added to the cinematic aspect
  store and dialogue boundary callbacks, including existing early-return
  branches.
- Remaining: one runtime run is required to determine whether either callback
  is active at the crash site.
- Deferred: Findings 1–6 remain unchanged; no crash-producing write path was
  identified by this diagnostic patch.
- Blocked: crash root cause remains unestablished pending runtime breadcrumbs.
- Not runtime-validated: diagnostic ASI has not been tested in-game.
- Patch summary: added minimal callback entry/return markers without guards,
  suppression, exception handling, or hook changes.
- Changelog summary: diagnostic build prepared to distinguish an early
  mod-owned callback crash from a crash outside the mod callback path.
## 2026-09-16 — Gameplay mode dispatch and HorPlus production integration

- Scope: implement two mutually exclusive gameplay correction modes only:
  `AspectRecalculation` (default) and `HorPlus`.
- Paths changed: `src/config/feature_config.hpp/.cpp`,
  `src/config/config_repository.cpp`, `src/config/config_template.cpp`,
  `src/gameplay/horplus_gameplay.hpp/.cpp`, `src/plugin/runtime.cpp`,
  `build.cmd`, `test.cmd`, `tests/config/gameplay_mode_harness.cpp`, and
  `tests/gameplay/horplus_gameplay_harness.cpp`.
- Validation: production `build.cmd` PASS; all harnesses through `test.cmd`
  PASS, including Gameplay mode and HorPlus harnesses; `git diff --check`
  PASS. Known C4201 warnings remain in vendored Zydis. Game launch was not
  performed.
- Completed: added fail-closed mode parsing and default/template persistence;
  retained one validated gameplay writer hook; dispatched the existing
  AspectRecalculation body or the native-input HorPlus transformation; kept
  16:9 as native bypass and accepted 21:9/32:9 with flags 0x4/0x5 without
  changing the multiplier.
- Remaining: runtime validation of both production modes, including their
  interaction with cinematic ENTER/EXIT, is required.
- Deferred: cinematic ownership/handoff redesign and all other findings were
  intentionally outside this patch.
- Blocked: none for static/build/harness validation.
- Not runtime-validated: the newly built ASI has not been run in-game.
- Patch summary: added selectable `AspectRecalculation` and `HorPlus`
  gameplay modes while preserving the validated default path and single-hook
  ownership.
- Changelog summary: users can select native aspect recalculation or direct
  aspect-aware HorPlus FOV correction; cinematic behavior is unchanged.
## 2026-09-16 — HorPlus cinematic EXIT handoff integration

- Scope: repair only the confirmed HorPlus EXIT state transition; no
  Cinematics subsystem rewrite.
- Paths changed: `src/gameplay/gameplay_state.hpp/.cpp`,
  `src/plugin/runtime.cpp`, `tests/cinematics/coordinator_recovery_harness.cpp`,
  `test.cmd`; task plan archived under `research/completed/`.
- Validation: all harnesses through `test.cmd` PASS; production `build.cmd`
  PASS; known C4201 warnings remain in vendored Zydis; `git diff --check`
  PASS. Game launch was not performed.
- Completed: `HorPlus` cinematic EXIT now returns the coordinator directly to
  `Gameplay` and does not arm the AspectRecalculation handoff; the validated
  `AspectRecalculation` EXIT path remains selected for its mode.
- Remaining: runtime validation of HorPlus cinematic ENTER/EXIT and gameplay
  continuation is required.
- Deferred: broader cinematic ownership research remains outside this patch.
- Blocked: none for static/build/harness validation.
- Not runtime-validated: the repaired ASI has not been tested in-game.
- Patch summary: added a mode-aware EXIT dispatch so HorPlus can resume its
  gameplay writer transformation without entering the old replay boundary.
- Changelog summary: HorPlus no longer leaves the shared coordinator waiting
  for an AspectRecalculation handoff that its mode does not use.
## 2026-09-16 — HorPlus cinematic-active FOV integration

- Scope: allow the existing HorPlus gameplay writer transformation during
  `CinematicActive` so cinematic and gameplay camera paths receive the same
  validated aspect-aware correction.
- Paths changed: `src/plugin/runtime.cpp`; diagnostic task plan archived under
  `research/completed/`.
- Validation: runtime log confirmed `CinematicActive` writer samples with
  `XMM0 90 -> 126.87`, `eligible=YES`, `applied=YES`; cinematic FOV was
  visually correct, EXIT was seamless, and ADS remained correct. Production
  and diagnostic builds passed before runtime. Game behavior was validated by
  the user.
- Completed: HorPlus no longer bypasses its transformation during active
  cinematics; AspectRecalculation and cinematic implementation remained
  unchanged.
- Remaining: broader user-FOV/aspect production matrix remains separate from
  this confirmed integration fix.
- Deferred: no additional cinematic research was opened.
- Blocked: none for this bounded repair.
- Patch summary: fixed the HorPlus cinematic-active writer bypass that caused
  incorrect FOV during cinematics while preserving seamless EXIT and ADS.
- Changelog summary: HorPlus now maintains correct ultrawide FOV during
  cinematics, gameplay transition and ADS.
## 2026-09-16 — AspectRecalculation dynamic effective-aspect gate

- Scope: generalize the constrained ultrawide detection gate and matching
  diagnostics; preserve normalization, replay choreography and forced
  cinematic targets.
- Paths changed: `src/plugin/runtime.cpp`,
  `tests/gameplay/horplus_gameplay_harness.cpp`; task plan archived under
  `research/completed/`.
- Validation: production `build.cmd` PASS; full `test.cmd` PASS including
  custom aspects `2.37037`, `2.38889`, `2.4`, `3.2` and `3.55556`;
  `git diff --check` PASS. Known C4201 warnings remain in vendored Zydis.
  Game launch was not performed.
- Completed: the `AspectRecalculation` constrained normalization gate now uses
  actual ultrawide aspect plus `flags == 0x5` instead of only `kCinemaAspect`.
  Matching diagnostic gates use the same semantic predicate.
- Remaining: runtime validation across custom resolutions is required.
- Deferred: forced cinematic target constants and rejected delayed-aspect probe
  were intentionally left unchanged.
- Blocked: none for static/build/harness validation.
- Not runtime-validated: the updated AspectRecalculation build has not been
  tested in-game.
- Patch summary: removed the production dependency on one canonical 21:9
  resolution while preserving the validated normalization operation.
- Changelog summary: AspectRecalculation now recognizes constrained ultrawide
  runtime states by their actual aspect rather than a fixed resolution.
## 2026-09-16 — HorPlus dynamic effective-aspect eligibility

- Scope: remove exact `21:9`/`32:9` eligibility gates from HorPlus; preserve
  frozen AspectRecalculation and all other camera features.
- Paths changed: `src/gameplay/horplus_gameplay.cpp`,
  `src/plugin/runtime.cpp`, `tests/gameplay/horplus_gameplay_harness.cpp`;
  task plan archived under `research/completed/`.
- Validation: HorPlus and full harness suite PASS; production `build.cmd`
  PASS; `git diff --check` PASS. Known C4201 warnings remain in vendored
  Zydis. Game launch was not performed.
- Completed: HorPlus now accepts any finite effective aspect wider than native
  16:9, including `3440x1440` (`2.38889`), with flags `0x4`/`0x5` remaining
  validation-only and producing the same multiplier for the same aspect.
- Remaining: runtime validation after the 21:9 resolution change is required.
- Deferred: no new HorPlus mathematics, hook, or cinematic integration was
  introduced.
- Blocked: none for static/build/harness validation.
- Not runtime-validated: the updated build has not been tested in-game.
- Patch summary: generalized HorPlus aspect eligibility from canonical aspect
  constants to the actual runtime effective aspect.
- Changelog summary: HorPlus supports real ultrawide display aspect values
  instead of relying on exact 21:9/32:9 ratios.
## 2026-09-16 — Shared aspect-policy predicate coverage

- Scope: extract the shared ultrawide and constrained-state predicates and add
  boundary, sweep and named custom-aspect coverage; no runtime launch.
- Paths changed: `src/gameplay/aspect_policy.hpp/.cpp`,
  `src/gameplay/horplus_gameplay.cpp`, `src/plugin/runtime.cpp`,
  `tests/gameplay/aspect_policy_harness.cpp`, `test.cmd`, `build.cmd`.
- Validation: production `build.cmd` PASS; full `test.cmd` PASS, including
  the shared aspect-policy harness; `git diff --check` PASS. Game launch was
  not performed.
- Completed: production and harness now use the same strict
  `nativeAspect + 0.001f` predicate. HorPlus retains `0x4`/`0x5` eligibility;
  AspectRecalculation constrained normalization retains `0x5`-only semantics.
  Boundary, 1.00–4.00 sweep, custom and non-finite cases pass.
- Remaining: runtime validation of the generalized predicates is required.
- Deferred: no changes to normalization operations, replay choreography,
  forced cinematic targets or FOV mathematics.
- Blocked: none for static/build/harness validation.
- Not runtime-validated: the updated build has not been tested in-game.
- Patch summary: production and automated tests share one canonical aspect
  policy implementation, removing duplicated eligibility semantics.
- Changelog summary: arbitrary ultrawide runtime aspects are covered by shared
  boundary and sweep tests without exact-resolution assumptions.
## 2026-09-17 — Gameplay mode cycle hotkey

- Scope: add a configurable hotkey for switching the two existing gameplay
  modes and document the INI setting.
- Paths changed: `src/config/feature_config.hpp/.cpp`,
  `src/config/config_repository.cpp`, `src/config/config_template.cpp`,
  `src/plugin/runtime.cpp`, `tests/config/gameplay_mode_harness.cpp`,
  `README.md`.
- Validation: production `build.cmd` PASS; full `test.cmd` PASS;
  `git diff --check` PASS. Game launch was not performed.
- Completed: `GameplayCycle=F11` is parsed and persisted through the existing
  hotkey infrastructure, cycles `AspectRecalculation -> HorPlus ->
  AspectRecalculation` on key-down edges, and updates the existing runtime
  mode selector. Hotkeys remain disabled unless `[Hotkeys] Enabled=true`.
- Remaining: in-game hotkey/runtime behavior is not validated.
- Deferred: no changes to gameplay algorithms, cinematic/dialogue behavior or
  hook ownership.
- Blocked: none for static/build/harness validation.
- Not runtime-validated: the hotkey-enabled build has not been tested in-game.
- Patch summary: added F11 runtime mode selection using the existing dispatch
  and config persistence paths.
- Changelog summary: users can switch between AspectRecalculation and HorPlus
  without restarting when runtime hotkeys are enabled.
## 2026-09-17 — Dialogue discriminator diagnostic build

- Scope: add diagnostic-only callback-context telemetry for the confirmed
  Dialogue false-positive; no production classifier or runtime behavior change.
- Paths changed in this batch: `src/plugin/runtime.cpp` and the diagnostic
  task plan; existing diagnostic build scripts were reused unchanged.
- Validation: diagnostic build `build-dialogue-diagnostic.cmd` PASS;
  `git diff --check` PASS. Game launch was not performed.
- Completed: removed unvalidated stack-return interpretation and added
  receiver/vtable readability, executable-vtable check, safe `[RSI+0x2C]`,
  integer/XMM register context, raw FOV, coordinator, policy and phase. The
  normal build keeps the diagnostic block disabled; the diagnostic output is a
  separate ASI.
- Remaining: one runtime run is required to compare ordinary gameplay FOV
  transitions, cinematic EXIT and real dialogue callback contexts.
- Deferred: no dialogue discriminator or production repair was selected.
- Blocked: safe production repair remains blocked until a positive
  dialogue-ownership signal is established.
- Not runtime-validated: diagnostic ASI behavior and caller differentiation.
- Patch summary: prepared bounded read-only telemetry at the existing dialogue
  boundary without adding hooks, caller guesses or production state changes.
- Changelog summary: diagnostic build can now compare receiver/vtable and
  source-field context across generic FOV descent and real dialogue.

## 2026-09-17 — `APC::IsInStaticDialog()` standalone access feasibility

- Scope: read-only current-image audit of the UE4SS-confirmed static-dialogue
  state query; no production Dialogue repair or runtime build.
- Paths changed: task plan/report in `research/`; read-only Ghidra helper,
  launcher and reproducible evidence under `02-Research/Ghidra/` and
  `02-Research/evidence/`.
- Validation: current Steam 2.0.5 identity PASS (SHA-256
  `E7B481A97C02D80581FAB0BECE940214A88EBE30211088A00129845A039F9293`, image
  base `0x140000000`, `.text` `0x7CCD000`). The read-only helper found one
  `.rdata` name record and no code/metadata follow-up XREF. No build or game
  launch was performed.
- Completed: established that UE4SS observes `false → true → false` for the
  tested static-dialogue path and that the current standalone ASI has no
  validated field/call/reflection route to consume it.
- Deferred: direct-state integration pending a separately established native
  access seam.
- Blocked: no safe APC field, native call, transition hook or ProcessEvent path
  exists in current evidence.
- Not runtime-validated: standalone ASI access and coverage beyond the tested
  static-dialogue path.
- Patch summary: added read-only, identity-gated evidence for the direct-state
  feasibility decision; production behavior is unchanged.
- Changelog summary: none; this is research only.

## 2026-09-17 — Dialogue ground-truth correlation diagnostic preparation

- Scope: prepare one synchronized UE4SS oracle plus the existing Dialogue
  boundary telemetry for correlation; no production classifier repair.
- Paths changed: `research/ue4ss/DialogueGroundTruthCorrelation/Scripts/main.lua`
  and the active diagnostic task plan. The existing diagnostic C++ telemetry
  was reused unchanged.
- Validation: `build-dialogue-diagnostic.cmd` PASS; diagnostic ASI produced as
  `STALKER2CameraTweaks_DialogueDiagnostic.asi`; `git diff --check` PASS. Build
  emitted only the known vendored Zydis C4201 warnings. Game was not launched.
- Completed: UE4SS logger polls `IsInStaticDialog()` every 50 ms and emits only
  state edges with sequence numbers; it fails closed on invalid player or query
  errors. The ASI diagnostic already records `[RSI+0x2C]`, XMM/source context,
  coordinator, phase and policy independently of classifier decisions.
- Remaining: one user-run synchronized runtime trace covering ordinary FOV
  descent, cinematic EXIT and a real static dialogue.
- Deferred: direct standalone `IsInStaticDialog()` access and any production
  Dialogue repair.
- Blocked: no blocker for the diagnostic run; production discriminator remains
  unestablished until the oracle correlation exists.
- Not runtime-validated: UE4SS/ASI timestamp correlation and discriminator
  usefulness.
- Patch summary: added a read-only ground-truth edge logger and produced the
  bounded diagnostic ASI without changing runtime behavior.
- Changelog summary: research-only diagnostic tooling; no release behavior
  change.

## 2026-09-17 — Dialogue ground-truth correlation runtime

- Scope: correlate UE4SS `APC::IsInStaticDialog()` edges with the existing
  standalone DialogueBoundary diagnostics; no production repair.
- Paths changed: runtime evidence report under `research/reports/`; no
  production source or config changes.
- Validation: supplied `UE4SS.log` and `STALKER2CameraTweaks.log` were matched
  by timestamps. Six `false/true` static-dialogue intervals were observed;
  `[RSI+0x2C]=70` aligned with sampled true intervals and `90` with the
  post-cinematic candidate/recovery context. No build was needed for analysis;
  no new game launch was performed by Codex.
- Completed: confirmed a post-cinematic Dialogue Candidate while the oracle
  was false; promoted `[RSI+0x2C]` to a strong practical discriminator
  candidate; confirmed F10 policy mutation during an oracle-true lifecycle.
- Remaining: establish whether the candidate survives broader non-dialogue
  FOV transitions and all Dialogue types before production classifier changes.
- Deferred: direct standalone access to `IsInStaticDialog()` remains deferred.
- Blocked: positive production Dialogue ownership is not yet established
  universally; no classifier repair selected from this run alone.
- Not runtime-validated: any production repair and any coverage beyond the
  tested static-dialogue path.
- Patch summary: recorded synchronized oracle/boundary evidence and separated
  confirmed findings from candidate correlations.
- Changelog summary: research evidence only; no release behavior change.

## 2026-09-17 — Dialogue discovery F12 diagnostic build

- Scope: bounded ten-second change-only discovery telemetry around the existing
  DialogueBoundary FOV-blend hook; production behavior unchanged.
- Paths changed: `src/plugin/runtime.cpp`, `build.cmd`,
  `build-dialogue-discovery.cmd`, diagnostic ASI and research report.
- Validation: separate diagnostic ASI built successfully; SHA-256 recorded in
  `research/reports/DIALOGUE_DISCOVERY_TRACE_BUILD_2026-09-17.md`; F12 is
  diagnostic-only and inactive by default; `git diff --check` passed. Game was
  not launched.
- Completed: added safe bounded `RSI+0x00..0x80` raw-DWORD snapshots, known FOV
  and register context, change-only logging, context rebasing and automatic
  ten-second expiry. Related-object calls and caller guessing were omitted.
- Remaining: user runtime trace and correlation with UE4SS ground truth.
- Deferred: production Dialogue classifier repair and standalone
  `IsInStaticDialog()` access.
- Blocked: no positive Dialogue discriminator established by this build.
- Not runtime-validated: all in-game diagnostic behavior.
- Patch summary: created isolated F12 discovery instrumentation and built
  `STALKER2CameraTweaks_DialogueDiscovery.asi`.
- Changelog summary: diagnostic research artifact only; no production release
  behavior change.

## 2026-09-17 — DialogueBoundary RSI+0x2C static data-flow audit

- Scope: identity-gated read-only audit of the validated DialogueBoundary
  context object and `[RSI+0x2C]` provenance; no production repair.
- Paths changed: Ghidra research scripts/runners and headless evidence under
  `02-Research/`; durable report under `research/reports/`.
- Validation: SHA-256, `.text` size, image base and unique DialogueBoundary
  signature passed for Steam 2.0.5. Older project images were rejected by the
  identity gate. No game launch, build or runtime validation was performed.
- Completed: established RSI as the native FOV-blend context object; established
  `[RSI+0x2C]` as a float target/end value used by native interpolation; found
  one plausible field-copy writer candidate but not its same-object runtime
  chain.
- Remaining: no production Dialogue discriminator or repair selected.
- Deferred: object identity/writer correlation against the UE4SS
  `IsInStaticDialog()` oracle; standalone direct access remains deferred.
- Blocked: `[RSI+0x2C]` alone is insufficient evidence for Dialogue ownership.
- Not runtime-validated: all items by task scope.
- Patch summary: added identity-gated Ghidra inventory/decompilation helpers
  and recorded the bounded negative result.
- Changelog summary: research-only static analysis; no release behavior change.

## 2026-09-18 — HorPlus telemetry truthfulness repair

- Scope: bounded D3 telemetry-only repair; Dialogue, Cinematics,
  AspectRecalculation, ZOOM, ADS and mode switching were explicitly excluded.
- Paths changed: `src/gameplay/horplus_gameplay.hpp`,
  `src/gameplay/horplus_gameplay.cpp`, `src/plugin/runtime.cpp`,
  `tests/gameplay/horplus_gameplay_harness.cpp`, and the completed task plan
  under `research/completed/`.
- Validation: full `test.cmd` suite passed; diagnostic candidate build
  `STALKER2CameraTweaks_ZoomHorPlusDiagnostic.asi` passed with the existing
  Zydis C4201 warnings; `git diff --check` passed. No game launch or runtime
  validation was performed.
- Completed: introduced one shared `EvaluateHorPlus()` result boundary used by
  production and the compatibility wrapper; `ApplyHorPlusGameplay()` now
  returns the actual decision/result; telemetry consumes that result after the
  production application instead of predicting it from owner classification.
  Cinematic samples no longer establish gameplay cache validity.
- Remaining: one combined runtime validation after the queued repairs are
  complete.
- Deferred: Tasks 2–10; no automatic transition to the next task.
- Blocked: none within this bounded batch.
- Not runtime-validated: live telemetry truthfulness in-game and stable ASI
  behavior on the target installation.
- Patch summary: aligned diagnostic HorPlus decision/result reporting with the
  actual writer transform boundary without changing Dialogue/Cinematic or
  AspectRecalculation behavior.
- Changelog summary: diagnostic telemetry now reports the production HorPlus
  result rather than an independent owner-based prediction; stable release
  behavior remains unchanged.

## 2026-09-18 — HorPlus neutral zoom-transition integration

- Scope: bounded Gameplay HorPlus integration design and neutral diagnostic
  terminology for the validated native zoom transition pair; Dialogue,
  Cinematics and AspectRecalculation were explicitly excluded.
- Paths changed: `src/plugin/runtime.cpp`,
  `src/hooks/signatures/signature_definitions.hpp`, `test.cmd`,
  `tests/gameplay/zoom_transition_harness.cpp`,
  `build-zoom-transition-diagnostic.cmd`, and the related plan/report.
- Validation: all local harnesses passed, including
  `Zoom transition HorPlus harness: PASS`; diagnostic candidate build passed
  with the existing Zydis C4201 warnings; `git diff --check` passed. No game
  launch or runtime validation was performed for this batch.
- Completed: confirmed the validated gameplay writer is the sole safe HorPlus
  FOV application point; renamed the diagnostic signature labels and log
  events to `ZOOM_IN`/`ZOOM_OUT`; added sample-by-sample single-transform and
  no-feedback harness coverage; built
  `STALKER2CameraTweaks_ZoomTransitionDiagnostic.asi`.
- Remaining: optional runtime validation of the renamed diagnostic labels;
  any future scheduling optimization requires a separate ordering/lifetime
  proof.
- Deferred: Dialogue and Cinematic changes, direct Wideboy-to-FOV transform,
  and writer-hit optimization.
- Blocked: none within this bounded batch.
- Not runtime-validated: in-game hook loading, zoom callback ordering relative
  to the gameplay writer, and user-visible HorPlus behavior after this rename.
- Patch summary: neutralized the old ADS naming without changing the
  gameplay transform owner or foreign camera subsystems.
- Changelog summary: diagnostic terminology and regression harness only;
  stable release behavior remains unchanged.

## 2026-09-18 — Diagnostic Dialogue hot-path logging cleanup

- Scope: remove repeated per-callback `EARLY_CB DIALOGUE` log lines from the
  merged diagnostic build; preserve Dialogue behavior and meaningful
  lifecycle/FOV telemetry.
- Paths changed: `src/plugin/runtime.cpp`,
  `research/completed/DIAGNOSTIC_HOTPATH_LOGGING_CLEANUP_TASK_PLAN.md` and
  the merged diagnostic build output.
- Validation: complete harness suite passed; merged Zoom/HorPlus diagnostic
  candidate build passed with the existing Zydis C4201 warnings; source search
  confirmed no `EARLY_CB DIALOGUE` logging remains; `git diff --check` passed.
  No game launch was performed after this cleanup.
- Completed: per-hit Dialogue ENTER/RETURN logging removed; callback and
  return counters retained; one `DIALOGUE_CALLBACK_SUMMARY` record added for
  diagnostic shutdown; Dialogue branch behavior and output assignments were
  left unchanged.
- Remaining: user runtime validation of compact log output and summary timing.
- Deferred: Dialogue classifier changes and all gameplay/cinematic behavior.
- Blocked: none within this instrumentation batch.
- Not runtime-validated: live log compactness until the next user run.
- Patch summary: converted noisy Dialogue callback logging into aggregate
  counters without changing the callback decisions.
- Changelog summary: merged diagnostic logs are now change/lifecycle-focused;
  stable release behavior remains unchanged.

## 2026-09-17 — Dialogue oracle polling recovery

- Scope: bounded diagnostic-only recovery for temporary UE4SS player/controller
  lookup failures during map transitions; no ASI or production repair.
- Paths changed: external UE4SS script
  `DialogueGroundTruthCorrelation/Scripts/main.lua` and its repository copy
  under `research/ue4ss/`; task plan under `research/deferred/`.
- Validation: reviewed the edited Lua source and confirmed the guard returns
  `false` to keep `LoopAsync` alive on lookup failure. No Lua interpreter was
  available for syntax execution. No ASI build or game launch was performed.
- Completed: protected `UEHelpers.GetPlayer()` lookup with a bounded `pcall`;
  transient failures now skip one sample and retry on the next 50 ms tick.
- Remaining: user runtime validation across map/controller transitions and a
  real static-dialogue session.
- Deferred: Dialogue classifier repair, direct standalone state access and all
  production behavior changes.
- Blocked: none for the diagnostic patch; oracle correlation remains
  unvalidated until a new UE4SS log contains edge records.
- Not runtime-validated: polling recovery in the live game.
- Patch summary: made the UE4SS ground-truth poller resilient to temporary
  `GetPlayer()` failures without changing its read-only edge semantics.
- Changelog summary: diagnostic research tooling only; no release behavior
  change.
## 2026-09-18 — Dialogue native lifecycle call-path audit

- Scope: bounded identity-gated read-only audit of `XPlayDialog*` reflected
  anchors and `UDialogManager` on Steam 2.0.5; no production repair, ASI build
  or runtime launch.
- Paths changed: `02-Research/Ghidra/` helper and launcher, headless evidence,
  and `research/reports/DIALOGUE_NATIVE_LIFECYCLE_CALLPATH_AUDIT_205.md`.
- Validation: Steam 2.0.5 SHA-256, image base and `.text` size passed; older
  Ghidra images were rejected by the identity gate. Headless process exited
  cleanly and no project lock remained. `git diff --check` passed for tracked
  changes.
- Completed: all four `XPlayDialog*` names were found only as reflected data
  records with no containing executable function; `UDialogManager` had one
  unreferenced `.data` record; no executable convergence, state writer or
  paired ENTER/EXIT seam was established.
- Remaining: none within this bounded anchor batch.
- Deferred: direct standalone `IsInStaticDialog()` access and any future
  native lifecycle search requiring a new executable anchor.
- Blocked: no targeted runtime diagnostic is justified because no validated
  candidate hook/seam was recovered.
- Not runtime-validated: all production behavior and any repair.
- Patch summary: recorded a bounded negative native call-path result and
  preserved the ownership search stop condition.
- Changelog summary: research-only static analysis; no release behavior change.

## 2026-09-18 — Neutral ZOOM transition cleanup

- Scope: bounded D6 cleanup; remove stale ADS ownership semantics while keeping
  `ZOOM_IN`/`ZOOM_OUT` read-only and neutral. Dialogue D1, HorPlus math,
  Cinematics and AspectRecalculation were explicitly excluded.
- Paths changed: `src/plugin/runtime.cpp`, `build.cmd`, the three neutral zoom
  diagnostic wrappers, `test.cmd`, removal of current-only `ads_lifecycle`
  source/harness files, the completed task plan and this task log.
- Validation: full `test.cmd` suite passed; neutral Zoom/HorPlus diagnostic
  build passed with existing Zydis C4201 warnings; normal production
  configuration compile passed to a separate task artifact; `git diff --check`
  passed. No game launch or runtime validation was performed.
- Completed: removed `AdsLifecycle`/ADS owner Dialogue suppression and stale
  ADS macro/build path; renamed current implementation to neutral
  `ZoomTransition`/`ZOOM_*`; ZOOM remains observation-only and HorPlus remains
  at the validated gameplay writer.
- Remaining: combined runtime validation for ADS-like and controller-pull-like
  transitions and Dialogue isolation.
- Deferred: Tasks 4–10; no automatic transition to the next task.
- Blocked: none within this bounded batch.
- Not runtime-validated: live ZOOM behavior and user-visible Dialogue behavior.
- Patch summary: eliminated the invalid generic-ZOOM-to-ADS-owner path without
  adding a replacement owner state.
- Changelog summary: current ZOOM diagnostics are neutral and read-only;
  ADS-specific ownership integration is no longer part of the architecture.

## 2026-09-18 — Post-cinematic raw trace containment

- Scope: bounded D4 instrumentation containment; functional cinematic
  recovery/exclusion and all camera/FOV behavior were explicitly preserved.
- Paths changed: `src/plugin/runtime.cpp`, the completed task plan under
  `research/completed/`, and this task log.
- Validation: full `test.cmd` suite passed; diagnostic candidate build
  `STALKER2CameraTweaks_ZoomHorPlusDiagnostic.asi` passed with the existing
  Zydis C4201 warnings; `git diff --check` passed. No game launch or runtime
  validation was performed.
- Completed: raw post-exit snapshot/SafeRead/logging instrumentation is now
  compile-gated and absent from the normal writer path; explicit diagnostic
  builds can opt into the raw trace. `PostCinematicRecoveryExclusion` remains
  independently active.
- Remaining: combined runtime validation of functional recovery and absence of
  the raw trace after a cinematic EXIT.
- Deferred: Tasks 3–10; no automatic transition to the next task.
- Blocked: none within this bounded batch.
- Not runtime-validated: live post-exit behavior and performance impact.
- Patch summary: removed permanent normal-production raw post-exit writer work
  without changing functional cinematic recovery/exclusion logic.
- Changelog summary: post-exit research instrumentation is isolated behind
  explicit diagnostic macros; production camera behavior remains unchanged.

## 2026-09-18 — Dialogue post-recovery re-arm repair

- Scope: bounded D1 repair for the confirmed immediate post-Dialogue
  recovery-tail Candidate re-arm. Task 5 Candidate lifetime hardening,
  positive ownership detection, ZOOM, HorPlus, Cinematics and
  AspectRecalculation were explicitly excluded.
- Paths changed: `src/dialogue/dialogue_state.hpp` and `.cpp`,
  `src/plugin/runtime.cpp`, `tests/dialogue/recovery_rearm_harness.cpp`,
  `test.cmd`, the completed task plan and this task log.
- Validation: full `test.cmd` suite passed; combined Dialogue/ZOOM/HorPlus
  diagnostic build passed; separate normal production compile passed to
  `STALKER2CameraTweaks_Task4ProductionCompile.asi`; `git diff --check`
  passed with only normal line-ending warnings. No game launch or stable ASI
  replacement was performed.
- Completed: recovery now enters `RearmPending` when it first reaches the
  existing recovery tolerance and only re-arms after two deterministic stable
  native FOV samples within the tolerance. Invalid samples fail closed and
  state-transition logging remains non-per-hit.
- Remaining: combined runtime validation of real Dialogue recovery-tail
  behavior and a later legitimate Dialogue.
- Deferred: generic Candidate lifetime hardening (Task 5), selected/active
  Dialogue policy (Task 6), and all unrelated camera research.
- Blocked: none within this bounded static/harness batch.
- Not runtime-validated: live Dialogue recovery and re-arm timing.
- Patch summary: added a minimal deterministic Dialogue recovery re-arm
  boundary instead of resetting directly to an immediately eligible Inactive
  state.
- Changelog summary: Dialogue recovery tails can no longer immediately seed a
  new Candidate in the validated state-machine model; runtime proof remains
  deferred to the combined regression session.

## 2026-09-18 — Dialogue Candidate hardening

- Scope: bounded D1 Candidate-lifetime repair after Task 4. Task 6 policy
  snapshot, positive Dialogue ownership, ZOOM, HorPlus, Cinematics and
  AspectRecalculation were explicitly excluded.
- Paths changed: `src/dialogue/dialogue_state.hpp` and `.cpp`,
  `src/plugin/runtime.cpp`, `tests/dialogue/candidate_hardening_harness.cpp`,
  `test.cmd`, the completed task plan and this task log.
- Validation: full `test.cmd` suite passed; combined Dialogue/ZOOM/HorPlus
  diagnostic build passed; separate normal production compile passed to
  `STALKER2CameraTweaks_Task5ProductionCompile.asi`; `git diff --check`
  passed with only normal line-ending warnings. No game launch or stable ASI
  replacement was performed.
- Completed: Candidate is now a bounded hypothesis. Cumulative small-step
  descent can activate it; clear upward reversal, repeated stabilization and
  invalid samples cancel it. Task 4 `RearmPending` remains authoritative.
- Remaining: combined runtime validation of legitimate Dialogue, generic FOV
  activity, controller/ZOOM, ADS-like transitions and two-dialogue recovery.
- Deferred: selected/active Dialogue policy snapshot (Task 6), positive native
  Dialogue ownership and all unrelated camera research.
- Blocked: none within this bounded static/harness batch.
- Not runtime-validated: live Candidate cancellation and false-positive rate.
- Patch summary: replaced the unbounded first-sample Candidate with a small
  trajectory tracker using cumulative descent, reversal and stabilization
  predicates.
- Changelog summary: stale Candidate hypotheses can no longer survive
  unrelated stable or reversing FOV activity until a later descent; legitimate
  slow descent remains covered by harness tests.

## 2026-09-18 — Dialogue selected/active policy snapshot

- Scope: bounded D2 repair separating next-Dialogue policy selection from the
  immutable policy used by the current confirmed Dialogue lifecycle. Candidate
  detection, `RearmPending`, positive ownership, ZOOM, HorPlus, Cinematics and
  AspectRecalculation were explicitly excluded.
- Paths changed: `src/dialogue/dialogue_state.hpp` and `.cpp`,
  `src/plugin/runtime.cpp`, `tests/dialogue/policy_snapshot_harness.cpp`,
  `test.cmd`, the completed task plan and this task log.
- Validation: full `test.cmd` suite passed; combined Dialogue/ZOOM/HorPlus
  diagnostic build passed; separate normal production compile passed to
  `STALKER2CameraTweaks_Task6ProductionCompile.asi`; `git diff --check`
  passed with only normal line-ending warnings. No game launch or stable ASI
  replacement was performed.
- Completed: selected policy remains the config/hotkey value; active policy is
  captured only at `Candidate -> Active`, remains fixed through Active/Exiting/
  RearmPending, and is released at lifecycle reset. Diagnostic discovery and
  hotkey logs now distinguish selected and active policy without per-callback
  spam.
- Remaining: combined runtime validation of F10 during Candidate, Active and
  Exiting plus next-Dialogue policy application.
- Deferred: positive native Dialogue ownership and all unrelated camera
  research.
- Blocked: none within this bounded static/harness batch.
- Not runtime-validated: live policy mutation behavior and visual Dialogue
  continuity.
- Patch summary: added a small `PolicySnapshot` abstraction and routed the
  Dialogue transform through the active lifecycle snapshot.
- Changelog summary: changing F10 selection during an active Dialogue no
  longer changes that Dialogue's policy; the new selection applies to the next
  confirmed lifecycle.
## 2026-09-18 — Gameplay mode transition contract

- Scope: bounded Task 7 implementation for explicit
  `AspectRecalculation <-> HorPlus` transitions, including safe physical
  deferral at non-Gameplay coordinator boundaries. Dialogue Tasks 4–6, ZOOM,
  Cinematics policy, HorPlus math and AspectRecalculation normalization were
  not changed.
- Paths changed: `src/gameplay/gameplay_state.hpp` and `.cpp`,
  `src/plugin/runtime.cpp`, `tests/gameplay/gameplay_mode_transition_harness.cpp`,
  `test.cmd`, the completed task plan and this task log.
- Validation: full `test.cmd` suite passed, including
  `gameplay_mode_transition=PASS`; combined diagnostic compile passed with
  `DIALOGUE_DIAGNOSTIC`, `ZOOM_TRANSITION_DIAGNOSTIC` and
  `HORPLUS_FOV_STATE_DIAGNOSTIC`; separate production compile passed to
  `STALKER2CameraTweaks_Task7ProductionCompile.asi`; `git diff --check`
  passed with only normal line-ending warnings and the known Zydis C4201
  warnings. No game launch or stable ASI replacement was performed.
- Completed: same-mode selection is a no-op; A→H clears stale replay state
  and defers physical aspect restoration until a valid Gameplay boundary;
  H→A clears pending transition, replay/source observations and diagnostic
  HorPlus cache state; invalid or unavailable runtime aspect fails closed;
  `Gameplay.Enabled=false` remains a pass-through path; deferred transition
  logging is change-only.
- Remaining: combined runtime validation of hotkey mode switching, physical
  aspect restoration and framing across cinematic/Gameplay boundaries.
- Deferred: Task 8 and all unrelated camera, Dialogue and ZOOM work.
- Blocked: none within this bounded static/harness batch.
- Not runtime-validated: live mode switching and in-game ownership timing.
- Patch summary: added an explicit mode-transition plan and runtime boundary
  that prevents stale `AspectRecalculation` replay state or HorPlus cache state
  from surviving a mode change, without changing either mode's core math.
- Changelog summary: switching gameplay modes now resets the previous mode's
  live state, applies only observed runtime aspect data at a safe Gameplay
  boundary, and fails closed when that data is unavailable.
## 2026-09-18 — Cinematic Auto aspect provenance

- Scope: bounded Task 8 provenance repair for `Cinematics=Auto`. Gameplay
  modes, FOV mathematics, Dialogue, ZOOM, explicit cinematic policies and
  release packaging were not changed.
- Paths changed: `src/plugin/runtime.cpp`, the completed Task 8 plan and this
  task log.
- Validation: full `test.cmd` suite passed; production compile passed to
  `STALKER2CameraTweaks_Task8ProductionCompile.asi`; `git diff --check`
  passed with only normal line-ending warnings and the known external Zydis
  C4201 warnings. No game launch or stable ASI replacement was performed.
- Completed: confirmed the existing Auto resolver remains client/display
  viewport based; removed the stale camera-field provenance wording and now
  log `client-display-viewport` or `native-fallback` truthfully. Removed the
  unused runtime-camera helper from this path.
- Remaining: one combined runtime regression session must confirm the source
  label and resolved aspect on representative/custom viewports.
- Deferred: release packaging and all unrelated cinematic/gameplay research.
- Blocked: none within this bounded static/build batch.
- Not runtime-validated: live Auto aspect resolution after this provenance-only
  correction.
- Patch summary: aligned cinematic Auto comments and telemetry with the
  already-established viewport resolver without changing policy behavior.
- Changelog summary: Auto cinematic telemetry now identifies the actual
  client/display viewport source instead of attributing resolution to the
  observed camera aspect.
## 2026-09-18 — Dialogue recovery endpoint diagnostic

- Scope: diagnostic-only telemetry for the confirmed Task 4 premature re-arm
  case. No Dialogue completion predicate, Candidate hardening, policy snapshot,
  HorPlus, ZOOM or Cinematics behavior was changed.
- Paths changed: `src/plugin/runtime.cpp`,
  `src/dialogue/dialogue_state.hpp` and `.cpp`, `build.cmd`, the completed
  diagnostic task plan and this task log.
- Validation: full `test.cmd` suite passed; combined diagnostic compile with
  `DIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC` passed; `git diff --check` passed
  with only normal line-ending warnings and the known external Zydis C4201
  warnings. No game launch or stable ASI replacement was performed.
- Completed: added compile-gated, change-driven reads of `RSI+0x28`,
  `RSI+0x2C` and `RSI+0x30`; telemetry records current/running/target/paired
  FOV, baseline, previous sample, deltas, direction, stable count and the
  current predicate's would-rearm result. Observation begins at Exiting,
  continues through RearmPending and remains bounded after the existing
  re-arm; unreadable fields, source changes and the sample limit fail closed
  for diagnostics only.
- Remaining: one combined runtime trace of Dialogue recovery is required to
  compare `baselineG` with the native target and actual endpoint.
- Deferred: choosing or implementing a new recovery completion predicate.
- Blocked: none within this bounded diagnostic batch.
- Not runtime-validated: native `+0x28/+0x2C/+0x30` behavior on the recovery
  path.
- Patch summary: added read-only recovery endpoint telemetry without altering
  the existing state machine or its premature re-arm behavior.
- Changelog summary: the next diagnostic run can distinguish Dialogue-entry
  baseline from native blend target and continuing recovery trajectory.
## 2026-09-18 — Dialogue recovery terminal trace extension

- Scope: bounded diagnostic follow-up to extend the Dialogue recovery endpoint
  observer from 64 to 256 samples and rename the ambiguous `runningFov` field
  to neutral `state28`. No production behavior or completion predicate changed.
- Paths changed: `src/plugin/runtime.cpp`,
  `src/dialogue/dialogue_state.hpp` and `.cpp`, the completed diagnostic plan
  and this task log.
- Validation: full `test.cmd` suite passed; diagnostic compile with
  `DIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC` passed; `git diff --check` passed
  with only normal line-ending warnings and known external Zydis C4201
  warnings. No game launch or stable ASI replacement was performed.
- Completed: extended the bounded diagnostic observation window and corrected
  telemetry naming so `RSI+0x28` is not presented as an FOV value.
- Remaining: runtime confirmation of terminal `state28` behavior.
- Deferred: any production use of `state28` or new recovery completion logic.
- Blocked: none within this bounded diagnostic batch.
- Not runtime-validated: the rebuilt terminal trace.
- Patch summary: increased diagnostic coverage without changing Task 4–6
  state-machine behavior.
- Changelog summary: the next Dialogue-only run can capture the native blend
  tail beyond the previous 64-sample cutoff.

## 2026-09-18 — Dialogue recovery native-target completion repair

- Scope: bounded Task 4 production repair. Candidate hardening, policy
  snapshot, ZOOM, HorPlus, Cinematics and AspectRecalculation were untouched.
- Paths changed: `src/dialogue/dialogue_state.hpp` and `.cpp`,
  `src/plugin/runtime.cpp`, the two focused Dialogue harnesses, the completed
  Task 4 plan and this task log.
- Validation: full `test.cmd` suite passed; production `build.cmd` passed to
  `STALKER2CameraTweaks.asi`; `git diff --check` passed with only normal
  line-ending warnings and known external Zydis C4201 warnings. No game launch
  or injected runtime validation was performed.
- Completed: recovery now tracks the validated native target from `RSI+0x2C`
  and completes only after stable convergence using the existing strict
  recovery tolerance. Invalid targets remain pending fail-closed; source
  changes cancel the stale recovery observer. The baseline is no longer used
  as the completion endpoint, and `RSI+0x28 == 0` is not required.
- Remaining: one runtime regression session on the current game build must
  verify normal Dialogue exit, the 2.0.5 premature-re-arm counterexample and
  subsequent Dialogue re-entry.
- Deferred: patch-2.0.6 Ghidra analysis and any broader Dialogue ownership
  redesign.
- Blocked: none within this bounded static/build batch.
- Not runtime-validated: native-target recovery behavior after this repair.
- Patch summary: replaced baseline-based Dialogue recovery completion with a
  source-aware, native-target convergence contract and fail-closed handling.
- Changelog summary: Dialogue recovery no longer releases on local proximity
  to its entry baseline; it waits for the native recovery target and safely
  abandons stale source ownership.

## 2026-09-18 — ZOOM transition 2.0.6 diagnostic build

- Scope: separate neutral `ZOOM_IN`/`ZOOM_OUT` compatibility diagnostic.
  Dialogue, HorPlus, Cinematics, AspectRecalculation, Gameplay mode logic and
  ZOOM behavior were not changed.
- Paths changed: completed ZOOM diagnostic task plan and this task log. The
  existing `src/plugin/runtime.cpp` telemetry and
  `build-zoom-transition-diagnostic.cmd` were reused without modification.
- Validation: full `test.cmd` suite passed; the separate diagnostic build
  passed to `STALKER2CameraTweaks_ZoomTransitionDiagnostic.asi`; `git diff
  --check` passed with only normal line-ending warnings and known external
  Zydis C4201 warnings. No game launch was performed.
- Completed: prepared a 2.0.6-ready diagnostic artifact that logs startup
  resolver/install status plus change-driven neutral `ZOOM_IN` and `ZOOM_OUT`
  events and its bounded summary.
- Remaining: runtime ADS in/out and controller pull/release validation on
  game patch 2.0.6.
- Deferred: Task 7 F11 mode-switch validation on 2.0.6.
- Blocked: none within this bounded build batch.
- Not runtime-validated: ZOOM signature compatibility on 2.0.6.
- Patch summary: enabled the existing neutral ZOOM diagnostic for the next
  2.0.6 runtime check without changing production behavior.
- Changelog summary: added a separate diagnostic ASI for validating the
  existing ZOOM_IN/ZOOM_OUT anchors on the new executable.

## 2026-09-18 — ZOOM 2.0.6 diagnostic hash repair

- Scope: corrected the executable identity gate for the separate neutral ZOOM
  diagnostic after the runtime log showed the old 2.0.5 hash. No ZOOM,
  Dialogue, HorPlus, Cinematic or gameplay behavior was changed.
- Paths changed: `src/plugin/runtime.cpp`, the completed hash-repair plan and
  this task log. The stable production ASI was not replaced.
- Validation: full `test.cmd` suite passed; the rebuilt
  `STALKER2CameraTweaks_ZoomTransitionDiagnostic.asi` build passed;
  `git diff --check` passed with only normal line-ending warnings and known
  external Zydis C4201 warnings. No game launch was performed.
- Completed: updated the diagnostic-only hash gate to the observed 2.0.6
  identity `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
- Remaining: runtime confirmation that both ZOOM anchors install and emit
  events on 2.0.6.
- Deferred: Task 7 F11 mode-switch validation.
- Blocked: none within this bounded repair batch.
- Not runtime-validated: ZOOM compatibility after the hash correction.
- Patch summary: removed the stale 2.0.5 identity rejection from the
  2.0.6-specific diagnostic build without weakening signature validation.
- Changelog summary: the next diagnostic run can reach the existing neutral
  ZOOM_IN/ZOOM_OUT resolver checks on patch 2.0.6.

## 2026-09-18 — Combined HorPlus FOV and ZOOM diagnostic build

- Scope: prepared the combined diagnostic artifact with both
  `HORPLUS_FOV_STATE_DIAGNOSTIC` and `ZOOM_TRANSITION_DIAGNOSTIC` enabled.
  Production behavior and all feature logic were unchanged.
- Paths changed: completed combined-diagnostic plan and this task log. The
  existing source and combined builder were reused.
- Validation: combined build passed to
  `STALKER2CameraTweaks_HorPlusFovStateWideboyDiagnostic.asi`; `git diff
  --check` passed with normal line-ending warnings and known external Zydis
  C4201 warnings. No game launch was performed.
- Completed: the artifact includes current Dialogue recovery, 2.0.6 ZOOM
  hash gate, neutral ZOOM events, and the three-layer HorPlus FOV telemetry.
- Remaining: runtime validation of configured FOV, native input FOV and
  HorPlus output FOV in one 2.0.6 session.
- Deferred: Task 7 mode-switch validation unless included in that runtime
  session.
- Blocked: none within this bounded build batch.
- Not runtime-validated: combined telemetry behavior in-game.
- Patch summary: replaced the incomplete ZOOM-only diagnostic handoff with
  the existing combined FOV-state plus ZOOM diagnostic artifact.
- Changelog summary: the next run will expose the full configured → native →
  HorPlus FOV chain alongside ZOOM_IN/ZOOM_OUT events.

## 2026-09-18 — Dialogue recovery single-target completion repair

- Scope: close Dialogue recovery on the first valid native FOV sample within
  the established native target tolerance. No timer, callback-absence
  heuristic, `state28` invariant or 2.0.6 Ghidra analysis was added.
- Paths changed: `src/dialogue/dialogue_state.hpp`,
  `src/dialogue/dialogue_state.cpp`, `src/plugin/runtime.cpp`,
  `tests/dialogue/recovery_rearm_harness.cpp` and the archived task plan.
  Candidate hardening, policy snapshots, ZOOM, HorPlus, Cinematics and
  AspectRecalculation were intentionally untouched.
- Validation: full `test.cmd` suite passed; production `build.cmd` passed;
  combined `build-horplus-fov-state-diagnostic.cmd` passed; `git diff
  --check` passed with normal line-ending warnings and known external Zydis
  C4201 warnings. No game launch was performed.
- Completed: a sample inside `nativeTarget ± 0.01` now completes recovery
  immediately, including the regression `89.9736 → 89.9949` with no
  follow-up callback. Invalid target remains pending and source changes still
  cancel fail-closed.
- Remaining: runtime validation on the 2.0.6 patch that exposed stale Dialogue
  ownership during the subsequent ADS transition.
- Deferred: broader native recovery endpoint research and full 2.0.6 static
  analysis remain unnecessary unless runtime evidence contradicts this repair.
- Blocked: none within this bounded implementation batch.
- Not runtime-validated: the repaired single-sample completion behavior.
- Patch summary: removed the extra confirmation-sample requirement from the
  target-convergence completion path while preserving the existing lifecycle
  guards and diagnostics.
- Changelog summary: Dialogue recovery can now release on the last native
  target sample instead of carrying stale ownership into the next zoom/ADS
  transition.

## 2026-09-18 — CameraStateSnapshot Batch 1

- Scope: implemented a pure, read-only derived CameraState snapshot with
  provenance, validity, typed source domains and diagnostic-only ZOOM evidence.
  No production behavior migration was included.
- Paths changed: `src/camera/camera_state_snapshot.hpp`,
  `src/camera/camera_state_snapshot.cpp`,
  `tests/camera/camera_state_snapshot_harness.cpp`, `src/plugin/runtime.cpp`,
  `build.cmd`, `test.cmd`, `build-horplus-fov-state-diagnostic.cmd`, the
  completed task plan, implementation report and this task log.
- Validation: full `test.cmd` passed including `camera_state_snapshot=PASS`;
  production and combined diagnostic builds passed; known external Zydis C4201
  warnings only. No game launch, Ghidra work, commit or release.
- Completed: independent substates are represented without a super-enum;
  Candidate is classified as hypothesis; Active as confirmed mod lifecycle;
  ZOOM remains observational; configured FOV remains unknown; different source
  token domains are not equated; diagnostic snapshot output is change-only.
- Remaining: runtime validation of snapshot telemetry and cross-hook coherence.
- Deferred: generation-based production invalidation, ZOOM lifecycle ownership
  and behavior migration.
- Blocked: none within this bounded batch.
- Not runtime-validated: all in-game snapshot behavior and combined timeline.
- Patch summary: added the first compositional CameraState derived view and
  diagnostic integration without changing existing camera owners.
- Changelog summary: the combined diagnostic build now exposes provenance and
  typed source evidence for future camera-state validation.

## 2026-09-18 — CameraStateSnapshot Batch 2 telemetry truthfulness

- Scope: corrected diagnostic snapshot wiring identified by the 2.0.6 combined
  run. No production behavior migration or stale-Candidate invalidation.
- Paths changed: `src/camera/camera_state_snapshot.hpp/.cpp`,
  `tests/camera/camera_state_snapshot_harness.cpp`, diagnostic wiring in
  `src/plugin/runtime.cpp`, Batch 2 task plan/report and this task log.
- Validation: full `test.cmd` passed including `camera_state_snapshot=PASS`;
  production and combined diagnostic builds passed; `git diff --check` passed
  with normal line-ending warnings and known external Zydis C4201 warnings.
  No game launch or Ghidra analysis.
- Completed: Candidate owner classification corrected; semantic change-only
  snapshot logging added; ZOOM weights corrected to RAX+0x4C/+0x50; event-local
  gameplay source, Dialogue source/target and recovery exclusion are exposed;
  named enum/provenance telemetry added.
- Remaining: runtime validation of corrected diagnostic output.
- Deferred: stale Candidate invalidation and any behavior migration.
- Blocked: none within this bounded telemetry batch.
- Not runtime-validated: all post-correction in-game telemetry.
- Patch summary: made CameraStateSnapshot telemetry reflect evidence provenance
  and source ownership without changing camera behavior.
- Changelog summary: the next combined diagnostic run will produce readable,
  semantically bounded snapshot events with corrected ZOOM and source data.

## 2026-09-18 — CameraStateSnapshot Batch 3 diagnostic truthfulness

- Scope: corrected canonical Dialogue/ZOOM source wiring and clarified that
  ZOOM telemetry records the last native observation. Candidate lifecycle and
  all production camera behavior were explicitly out of scope.
- Paths changed: `src/camera/camera_state_snapshot.hpp/.cpp`, diagnostic
  logging in `src/plugin/runtime.cpp`,
  `tests/camera/camera_state_snapshot_harness.cpp`, Batch 3 report and the
  archived task plan.
- Validation: full `test.cmd` passed; production `build.cmd` passed; combined
  `build-horplus-fov-state-diagnostic.cmd` passed; `git diff --check` passed
  with normal line-ending warnings and known external Zydis C4201 warnings.
  No game launch or Ghidra analysis.
- Completed: removed duplicate Dialogue/ZOOM source fields from
  `SourceProvenance`; logger now reads typed canonical sources; ZOOM labels
  identify last observation, sequence and availability; harness checks exact
  source tokens.
- Remaining: one combined runtime sanity run for corrected diagnostic output.
- Deferred: source-scoped Candidate invalidation and interpretation of the
  `0.25` zoom parameter.
- Blocked: none within this bounded batch.
- Not runtime-validated: all post-patch in-game telemetry.
- Patch summary: made CameraStateSnapshot telemetry truthful and removed
  duplicate mutable source representations.
- Changelog summary: logs now distinguish last ZOOM observations from current
  ownership and report the canonical Dialogue source.

## 2026-09-18 — Dialogue Candidate context repair

- Scope: bounded Candidate → Active observation-context repair. Active,
  Exiting, RecoveryRearm, CameraStateSnapshot, ZOOM, HorPlus and Cinematics
  were out of scope.
- Paths changed: `src/dialogue/dialogue_state.hpp/.cpp`, Candidate integration
  in `src/plugin/runtime.cpp`,
  `tests/dialogue/candidate_hardening_harness.cpp`, the archived task plan and
  repair report.
- Validation: full `test.cmd` passed; production and combined diagnostic builds
  passed; `git diff --check` passed with normal line-ending warnings and known
  external Zydis C4201 warnings. No game launch or Ghidra analysis.
- Completed: Candidate captures DialogueBoundary source/target; promotion
  requires unchanged coherent context and valid non-Native policy; invalid
  context cancels without same-sample reseeding; deterministic harness cases
  pass.
- Remaining: runtime validation with real Dialogue and subsequent FOV/ZOOM
  transitions.
- Deferred: broader source-scoped Candidate generation/epoch design.
- Blocked: none within this bounded implementation batch.
- Not runtime-validated: all post-repair in-game behavior.
- Patch summary: added fail-closed source/target continuity at Candidate
  promotion without changing established recovery semantics.
- Changelog summary: Dialogue hypotheses can no longer promote across a
  changed native observation context or into Native policy.

## 2026-09-19 — Runtime diagnostics config gate

- Scope: one canonical ASI with default-off runtime gating for the supported
  CameraStateSnapshot, ZOOM_IN/ZOOM_OUT and HorPlus FOV telemetry. Historical
  research probes, camera behavior and resolver ownership were out of scope.
- Paths changed: `src/config/feature_config.hpp`,
  `src/config/config_repository.cpp`, `src/config/config_template.cpp`,
  `src/diagnostics/diagnostic_runtime.hpp/.cpp`, diagnostic integration in
  `src/plugin/runtime.cpp`, `build.cmd`, `test.cmd`, and focused config/gate
  harnesses. Report: `research/reports/RUNTIME_DIAGNOSTICS_CONFIG_GATE.md`.
- Validation: full `test.cmd` passed; canonical production build passed;
  existing HorPlus FOV diagnostic wrapper build passed; `git diff --check`
  passed with normal line-ending warnings and known external Zydis C4201
  warnings. No game launch or Ghidra analysis.
- Completed: `[Diagnostics] Enabled=false` default and synchronized template;
  thread-safe runtime gate; canonical build includes supported telemetry;
  disabled mode avoids diagnostic hook installation and output; config and
  gate harness coverage passes.
- Remaining: runtime A/B check with `Enabled=false` and `Enabled=true`.
- Deferred: promotion of historical research probes and any production camera
  behavior changes based on diagnostic output.
- Blocked: none within this bounded batch.
- Not runtime-validated: actual in-game toggle behavior and telemetry output.
- Patch summary: consolidated supported diagnostics behind one default-off INI
  switch while preserving production behavior.
- Changelog summary: users can now use one canonical ASI and enable supported
  telemetry through `[Diagnostics] Enabled=true` instead of a separate build.

## 2026-09-19 — Stable gameplay baseline diagnostic design

- Scope: diagnostic-only telemetry for a future MatchGameplay baseline
  experiment and reusable CinematicActive guard evidence. No MatchGameplay,
  production baseline state, Ghidra analysis or runtime session.
- Paths changed: `src/plugin/runtime.cpp`, `build.cmd`, report
  `research/reports/STABLE_GAMEPLAY_BASELINE_DIAGNOSTIC.md`, and the archived
  task plan.
- Validation: full `test.cmd` passed; canonical `build.cmd` passed;
  `git diff --check` passed with normal line-ending warnings and known external
  Zydis C4201 warnings. No game launch or EXE analysis.
- Completed: change-only stable Gameplay endpoint observation with explicit
  hypothesis labeling and fail-safe context resets; canonical Task 9 trace now
  records cached ENTER FOV/aspect and numeric guard provenance.
- Remaining: one runtime session using Gameplay FOV 80, 100 and 110 with ADS
  transitions, optionally combined with a dynamic cinematic scenario.
- Deferred: MatchGameplay formula, configured-FOV accessor, production
  baseline classifier and any CinematicActive guard repair.
- Blocked: none within this bounded diagnostic batch.
- Not runtime-validated: all new baseline and Task 9 telemetry.
- Patch summary: made stable-endpoint hypothesis and cinematic cache decisions
  observable without granting either any production authority.
- Changelog summary: the canonical ASI can now collect the evidence needed to
  test stable gameplay FOV recovery and cinematic FOV provenance in one run.
## 2026-09-19 — Explicit native-to-HorPlus gameplay FOV pair telemetry

- Scope: make the existing diagnostic gameplay FOV event explicitly report the
  native writer input before HorPlus and the derived HorPlus result from the
  same observation.
- Changed: `src/plugin/runtime.cpp` and
  `research/reports/STABLE_GAMEPLAY_BASELINE_DIAGNOSTIC.md`.
- Not changed: production FOV behavior, baseline inference, MatchGameplay,
  Dialogue, Cinematics, ZOOM and CameraState ownership.
- Validation: `test.cmd` PASS; `build.cmd` PASS with existing external Zydis
  C4201 warnings; `git diff --check` PASS apart from normal line-ending
  warnings; no game launch.
- Result: telemetry now includes `nativeBeforeHorPlus` and `horPlusResult`;
  baseline classifier remains deferred.
## 2026-09-19 — MatchGameplay diagnostic prediction without production write

- Scope: capture exact cinematic ENTER observation separately from the latest
  pre-ENTER gameplay native/HorPlus pair and log a non-applied tangent-space
  MatchGameplay candidate.
- Changed: `src/plugin/runtime.cpp`, `build.cmd`, and diagnostic research
  reports/task plan.
- Non-goals preserved: no MatchGameplay output change, no new owner/state
  machine, no Dialogue/ZOOM/Cinematic lifecycle change, no runtime launch.
- Validation: `test.cmd` PASS; `build.cmd` PASS with existing external Zydis
  C4201 warnings; `git diff --check` PASS apart from normal line-ending
  warnings.
- Result: `E` remains an exact ENTER observation; candidate prediction is
  diagnostics-only and explicitly does not promote `E` to semantic `R`.
## 2026-09-19 — MatchGameplay prediction telemetry AFTER-sample correction

- Scope: correct diagnostic-only candidate input provenance after the first
  prediction run showed that `AFTER` records were using transformed `XMM0`.
- Changed: `src/plugin/runtime.cpp` and
  `research/reports/MATCHGAMEPLAY_DIAGNOSTIC_PREDICTION.md`.
- Production behavior: unchanged; no camera write or state-machine change.
- Validation: `test.cmd` PASS; `build.cmd` PASS with existing external Zydis
  C4201 warnings; `git diff --check` PASS apart from normal line-ending
  warnings.
- Runtime status: the first candidate run is invalid for interpreting `AFTER`
  predictions; a new clean run is required after this telemetry correction.

## 2026-09-19 — MatchGameplay cached ENTER diagnostic provenance repair

- Scope: exclude samples identified by the existing numeric guard as cached
  transformed ENTER values from native-space MatchGameplay prediction.
- Changed: `src/plugin/runtime.cpp`, `test.cmd`,
  `src/diagnostics/matchgameplay_prediction.hpp`, the diagnostic harness, and
  the associated research report.
- Not changed: production Task 9 guard, MatchGameplay output, HorPlus,
  Cinematics, Dialogue, recovery, baseline capture, or CameraState behavior.
- Validation: relevant harness PASS; full `test.cmd` PASS; `build.cmd` PASS
  with existing external Zydis C4201 warnings; `git diff --check` PASS apart
  from normal line-ending warnings; no game launch.
- Completed: cached samples are explicitly labeled
  `CACHED_TRANSFORMED_ENTER` and do not emit interpretable native prediction
  values; ordinary native samples retain the existing candidate calculation.
- Remaining: runtime confirmation during a normal Gameplay-to-Cinematic run.
- Deferred: direct-load baseline provenance and production MatchGameplay.
- Blocked: none within this bounded batch.
- Patch summary: corrected diagnostic sample-space provenance without granting
  the predictor production authority.
- Changelog summary: MatchGameplay telemetry no longer presents cached
  transformed ENTER values as native cinematic predictions.

## 2026-09-19 — Opt-in Cinematic FovMode MatchGameplay implementation

- Scope: add one INI-selected cinematic FOV mode while preserving the current
  HorPlus behavior and writer path.
- Changed: `src/config/feature_config.hpp`, `src/config/feature_config.cpp`,
  `src/config/config_repository.cpp`, `src/config/config_template.cpp`,
  `src/cinematics/cinematic_fov.hpp`, `src/cinematics/cinematic_fov.cpp`,
  `src/plugin/runtime.cpp`, `tests/config/config_persistence_harness.cpp`,
  `tests/gameplay/horplus_gameplay_harness.cpp`, `test.cmd`, `README.md`, and
  the implementation report.
- Not changed: separate ASI packaging, hook location/timing, Dialogue, ZOOM,
  AspectRecalculation, CameraState, Task 9 guard semantics, or the general
  observation architecture.
- Validation: full `test.cmd` PASS; `build.cmd` PASS with existing external
  Zydis C4201 warnings; `git diff --check` PASS apart from normal line-ending
  warnings; no game launch.
- Completed: `FovMode=HorPlus` default/current fallback; opt-in
  `FovMode=MatchGameplay`; invalid context safely falls back to HorPlus;
  deterministic config/math coverage.
- Remaining: runtime A/B comparison with the same cinematic at gameplay FOV
  112 and a control run with `FovMode=HorPlus`.
- Deferred: universal semantic validation of ENTER reference `R` and the
  broader `LatestCameraFovObservation` refactor.
- Blocked: none within this bounded batch.
- Patch summary: added optional gameplay-baseline-relative cinematic FOV
  mathematics without creating a second ASI or camera path.
- Changelog summary: users can now select the existing HorPlus cinematic FOV
  or the experimental MatchGameplay mode through the INI file.

## 2026-09-19 — Rename cinematic FOV modes and move GameplayHorPlus to ENTER

- Scope: distinguish cinematic `NativeHorPlus` from gameplay `HorPlus` and
  place the experimental gameplay-relative cinematic transform at the existing
  ENTER seam.
- Changed: cinematic/config implementation, harnesses, README, report and
  archived task plan.
- Not changed: default NativeHorPlus output, writer hook timing, cached-value
  guard, Dialogue, ZOOM, AspectRecalculation, recovery or CameraState.
- Validation: full `test.cmd` PASS; `build.cmd` PASS with existing external
  Zydis C4201 warnings; no game launch. Final diff check is still pending.
- Completed: `NativeHorPlus`/`GameplayHorPlus` naming, legacy aliases,
  ENTER-seam decision and writer branch removal.
- Remaining: runtime A/B with the same cinematic and gameplay baseline.
- Deferred: broader observation architecture and universal reference semantics.
- Blocked: none within this bounded batch.
- Patch summary: separated cinematic FOV semantics from gameplay mode naming and
  moved the opt-in transfer to the validated cinematic ENTER transform.
- Changelog summary: cinematic FOV mode names now identify whether the
  calculation is independent or gameplay-baseline-relative.

## 2026-09-19 — GameplayHorPlus retained baseline propagation repair

- Scope: publish the latest valid Gameplay HorPlus native FOV/source/aspect
  observation to the retained baseline consumed by cinematic ENTER.
- Changed: `src/plugin/runtime.cpp`, implementation report, and task plan.
- Not changed: MatchGameplay formula, NativeHorPlus fallback, Dialogue, ZOOM,
  AspectRecalculation, Cinematic EXIT, resolver logic, or CameraState design.
- Validation: full `test.cmd` PASS; `build.cmd` PASS with existing external
  Zydis C4201 warnings; `git diff --check` PASS apart from normal line-ending
  warnings; no game launch.
- Completed: Gameplay.Mode=HorPlus now retains valid Gameplay writer samples
  before Cinematic ENTER; CinematicActive samples cannot overwrite the cache;
  invalid observations fail closed.
- Remaining: runtime confirmation that `GameplayHorPlus` receives the retained
  pair and no longer falls back to `NativeHorPlus` in a normal transition.
- Deferred: broader LatestCameraFovObservation refactor and MatchGameplay
  semantic validation.
- Blocked: none within this bounded batch.
- Patch summary: fixed the production baseline propagation gap caused by the
  HorPlus writer path bypassing the legacy observation publisher.
- Changelog summary: GameplayHorPlus can now consume the established Gameplay
  baseline across the Gameplay-to-Cinematic boundary.

## 2026-09-19 — Unified cinematic FOV baseline pipeline

- Scope: unify NativeHorPlus and GameplayHorPlus behind one cinematic tangent
  transfer plus HorPlus implementation; mode selects only the target baseline.
- Changed: `src/cinematics/cinematic_fov.hpp`,
  `src/cinematics/cinematic_fov.cpp`, `src/plugin/runtime.cpp`, the gameplay
  HorPlus harness, implementation report, and task plan.
- Not changed: retained baseline propagation, cached transformed provenance,
  Dialogue, ZOOM, AspectRecalculation, CameraState, resolver behavior, or the
  NativeHorPlus default contract.
- Validation: full `test.cmd` PASS; unified math harness assertions PASS;
  `build.cmd` PASS with existing external Zydis C4201 warnings;
  `git diff --check` PASS apart from normal line-ending warnings; no game
  launch.
- Completed: one common transformation function; NativeHorPlus is the
  identity-baseline case; GameplayHorPlus uses the retained Gameplay baseline;
  invalid Gameplay context falls back to the cinematic reference baseline.
- Remaining: runtime regression confirmation after the refactor.
- Deferred: broader LatestCameraFovObservation architecture.
- Blocked: none within this bounded batch.
- Patch summary: removed duplicate cinematic FOV execution logic while
  preserving both mode semantics and fallback behavior.
- Changelog summary: both cinematic FOV modes now share one validated
  transformation pipeline and differ only by baseline selection.

## 2026-09-19 — Cinematic FOV mode F12 hotkey

- Scope: add a configurable F12 cycle for `Cinematics.FovMode`.
- Changed: hotkey/config parser and template, runtime hotkey selection,
  config harness, README, implementation report and task plan.
- Not changed: cinematic FOV math, baseline propagation, active cinematic
  behavior, Dialogue, ZOOM, Gameplay mode or resolver logic.
- Validation: full `test.cmd` PASS including config synchronization and F12
  cycle coverage; `build.cmd` PASS with existing external Zydis C4201 warnings;
  `git diff --check` PASS apart from normal line-ending warnings; no game
  launch.
- Completed: F12 cycles `NativeHorPlus` and `GameplayHorPlus`, persists the
  selected mode, and applies it only to the next cinematic.
- Remaining: runtime confirmation during the next combined game session.
- Deferred: none within this bounded batch.
- Blocked: none.
- Patch summary: added the requested runtime selector for cinematic FOV mode
  without changing the active cinematic.
- Changelog summary: F12 now switches the next cinematic between native and
  gameplay-relative HorPlus framing.

## 2026-09-19 — Camera FOV observation architecture Batch 1

- Scope: add a read-only, provenance-aware observation store for the established
  CameraWriter and CinematicEnter boundaries.
- Changed: `src/camera/fov_observation.hpp/.cpp`, `src/plugin/runtime.cpp`,
  `tests/camera/fov_observation_harness.cpp`, `build.cmd`, `test.cmd`, the
  implementation report and task plan.
- Not changed: production FOV decisions/output, Gameplay, Cinematics,
  Dialogue, ZOOM, recovery, AspectRecalculation, resolver behavior or
  CameraStateSnapshot semantics.
- Validation: full `test.cmd` PASS; `build.cmd` PASS with only known external
  Zydis C4201 warnings; `git diff --check` PASS apart from normal line-ending
  warnings; no game launch.
- Completed: per-boundary coherent store, component validity, explicit native /
  transformed spaces, provenance, monotonic publication sequence, CameraWriter
  and CinematicEnter publication, cached transformed pass-through labeling and
  deterministic harness coverage.
- Remaining: runtime inspection of the new diagnostic records.
- Deferred: production consumers, generation semantics, global latest-camera
  API and AspectRecalculation coverage.
- Blocked: none within this bounded batch.
- Patch summary: introduced one factual FOV observation schema and isolated
  latest slots without granting the observation layer production authority.
- Changelog summary: diagnostics can now distinguish native inputs, transformed
  results and cached transformed cinematic values at their originating
  boundaries.

## 2026-09-19 — GameplayBaseline projection architecture Batch 2A

- Scope: add a coherent retained GameplayBaseline projection in parallel with
  legacy retained state for equivalence proof.
- Changed: `src/camera/gameplay_baseline.hpp/.cpp`, `src/plugin/runtime.cpp`,
  `tests/camera/gameplay_baseline_harness.cpp`, `build.cmd`, `test.cmd`, the
  implementation report and task plan.
- Not changed: production consumers, Cinematic ENTER reads, legacy globals,
  Dialogue invalidation, aspect-cache restoration, CameraState, FOV/aspect
  math, resolver, hooks, ADS, ZOOM, recovery or AspectRecalculation behavior.
- Validation: full `test.cmd` PASS including Gameplay baseline harness;
  `build.cmd` PASS with known external Zydis C4201 warnings; `git diff --check`
  PASS apart from normal line-ending warnings; no game launch.
- Completed: coherent mutex-protected snapshot, explicit Gameplay eligibility,
  committed observation mapping, UPDATE/RETAIN/INVALIDATE semantics, mode
  transition invalidation and legacy-subset equivalence harness.
- Remaining: production consumer migration is a separate Batch 2B task.
- Deferred: runtime validation, legacy cache removal, aspect-cache migration and
  broader camera-state integration.
- Blocked: none within this bounded batch.
- Patch summary: added a shadow semantic GameplayBaseline derived only from
  eligible native-to-HorPlus CameraWriter observations.
- Changelog summary: the mod now has a coherent retained Gameplay baseline
  candidate for future consumer migration without changing current behavior.

## 2026-09-19 — GameplayBaseline native coverage extension Batch 2A.1

- Scope: extend the shadow GameplayBaseline projection to native-only valid
  pass-through and AspectRecalculation Gameplay writer events.
- Changed: `src/camera/gameplay_baseline.hpp/.cpp`, `src/plugin/runtime.cpp`,
  `tests/camera/gameplay_baseline_harness.cpp`, the implementation report and
  task plan.
- Not changed: Cinematic ENTER consumer, legacy globals, Dialogue invalidation,
  aspect restoration, cinematic math/aspect policy, CameraState, ZOOM, ADS,
  resolver, hooks or production consumer migration.
- Validation: full `test.cmd` PASS including expanded native/pass-through/
  AspectRecalculation cases; `build.cmd` PASS with known external Zydis C4201
  warnings; `git diff --check` PASS apart from normal line-ending warnings;
  no game launch.
- Completed: independent native/optional-HorPlus validity, pass-through whole
  snapshot replacement, AspectRecalculation factual publication/projection,
  explicit mode invalidation followed by repopulation, and legacy-subset
  equivalence coverage.
- Remaining: Batch 2B production ENTER cutover requires a separate plan.
- Deferred: runtime regression and legacy state removal/migration.
- Blocked: none within this bounded batch.
- Patch summary: GameplayBaseline now represents every supported valid native
  Gameplay target without fabricating transformed evidence.
- Changelog summary: pass-through and AspectRecalculation gameplay samples can
  now update the shadow native baseline while transformed HorPlus evidence
  remains optional and truthful.

## 2026-09-19 — GameplayBaseline → Cinematic ENTER production cutover Batch 2B

- Scope: migrate only Cinematic ENTER / GameplayHorPlus target selection to one
  coherent GameplayBaseline snapshot.
- Changed: `src/cinematics/cinematic_fov.hpp/.cpp`, `src/plugin/runtime.cpp`,
  `tests/cinematics/cinematic_fov_harness.cpp`, `test.cmd`, the Batch 2B plan
  and implementation report.
- Not changed: NativeHorPlus math, cinematic aspect policy, GameplayBaseline
  projection semantics, legacy Dialogue invalidation, aspect restoration, ZOOM,
  ADS, CameraState, hooks, resolvers or other production consumers.
- Validation: full `test.cmd` PASS; cinematic cutover, GameplayBaseline and
  zoom harnesses PASS; `build.cmd` PASS with known external Zydis C4201
  warnings; `git diff --check` PASS apart from normal line-ending warnings;
  no game launch.
- Completed: one coherent baseline read at ENTER, native-only target selection,
  authored fallback, NativeHorPlus independence, cinematic-aspect independence
  and diagnostic selection telemetry.
- Remaining: runtime regression confirmation on the next combined game session.
- Deferred: legacy-state removal and any broader baseline/latest-camera migration.
- Blocked: none within this bounded batch.
- Patch summary: Cinematic ENTER / GameplayHorPlus now consumes the retained
  GameplayBaseline native target without changing cinematic math or aspect
  policy.
- Changelog summary: GameplayHorPlus target selection is now coherent and
  fail-safe, while NativeHorPlus remains the unchanged control path.

## 2026-09-19 — Performance P1 logging and diagnostic containment

- Scope: contain confirmed synchronous logging and ungated diagnostic overhead.
- Changed: `src/plugin/runtime.cpp`, the P1 task plan and implementation
  report.
- Not changed: GameplayBaseline, HorPlus/cinematic/dialogue math or state,
  coordinator, hooks, resolvers, build-profile separation or speculative
  mutex/compiler/hook optimization.
- Validation: full `test.cmd` PASS; `build.cmd` PASS with known external Zydis
  C4201 warnings; `git diff --check` PASS apart from normal line-ending
  warnings; no game launch or performance benchmark.
- Completed: removed per-info flush, retained error/init/shutdown flushing,
  gated camera-mode SafeRead probes and made detailed gameplay FOV logging
  diagnostics-only. Existing high-rate ZOOM/FOV/cinematic gates were verified.
- Remaining: measured FPS/frametime impact is not established.
- Deferred: P2 production/diagnostic build separation and measurement-driven
  store, SafetyHook, `/O1`/`/O2` and viewport-query work.
- Blocked: none within this bounded batch.
- Patch summary: normal camera callbacks no longer force info-level file flushes
  or execute detailed diagnostic probes when diagnostics are disabled.
- Changelog summary: production steady-state logging is buffered and detailed
  camera/FOV tracing is now explicitly diagnostic-only.

## 2026-09-19 — Performance P2 production/diagnostic build separation

- Scope: separate compile-time production and diagnostic instrumentation
  profiles using the same source tree.
- Changed: `build.cmd`, new `build-diagnostic.cmd`, README build instructions,
  the P2 task plan and implementation report.
- Not changed: runtime behavior, hooks, resolvers, GameplayBaseline, HorPlus,
  cinematic/dialogue state, coordinator or math.
- Validation: full `test.cmd` PASS; production `build.cmd` PASS; diagnostic
  `build-diagnostic.cmd` PASS; distinct production and diagnostic ASI outputs
  created; `git diff --check` PASS apart from normal line-ending warnings; no
  game launch or performance benchmark.
- Completed: production build excludes high-rate research defines; diagnostic
  build retains supported instrumentation and uses an explicit artifact name.
- Remaining: runtime regression and actual performance A/B measurement.
- Deferred: P3 measurement-driven optimization.
- Blocked: none within this bounded batch.
- Patch summary: canonical production compilation is now separated from the
  research-heavy diagnostic compilation without duplicating the source list.
- Changelog summary: `build.cmd` creates the clean production ASI and
  `build-diagnostic.cmd` creates a clearly named diagnostic ASI.

## 2026-09-19 — Gameplay aspect restoration shadow projection

- Scope: add a coherent shadow state for Gameplay aspect restoration while
  leaving the legacy production consumer authoritative.
- Changed: `src/camera/gameplay_aspect_restoration.hpp/.cpp`,
  `src/plugin/runtime.cpp`, `tests/camera/gameplay_aspect_restoration_harness.cpp`,
  `test.cmd`, `build.cmd` and the implementation report.
- Not changed: production consumer, legacy state deletion, GameplayBaseline
  semantics, HorPlus/Cinematic ENTER, AspectRecalculation output, Dialogue,
  ZOOM, ADS, cached ENTER guard, hooks, resolvers and runtime game behavior.
- Validation: focused restoration harness PASS; full `test.cmd` PASS;
  production `build.cmd` PASS with known external Zydis C4201 warnings;
  `git diff --check` PASS apart from normal line-ending warnings; no game
  launch.
- Completed: coherent UPDATE/RETAIN/INVALIDATE shadow projection, valid-aspect
  without FOV/flags requirement, Auto-restore retention, Gameplay-disabled
  coverage and diagnostics-gated comparison telemetry.
- Remaining: one combined runtime shadow comparison session.
- Deferred: production consumer cutover and legacy-state deletion.
- Blocked: none within this bounded batch.
- Patch summary: restoration semantics now have a whole-snapshot shadow owner
  without changing the legacy production decision path.
- Changelog summary: added diagnostic-ready Gameplay aspect restoration shadow
  state for future validated consumer migration.

## 2026-09-19 — Gameplay aspect restoration production consumer cutover

- Scope: migrate only `ApplyPendingGameplayModeTransition()` to the coherent
  restoration snapshot; retain legacy state as shadow/reference.
- Changed: `src/camera/gameplay_aspect_restoration.hpp/.cpp`,
  `src/plugin/runtime.cpp`, `tests/camera/gameplay_aspect_restoration_harness.cpp`,
  `test.cmd`, the cutover report and archived task plan.
- Not changed: restoration producers, legacy state deletion, GameplayBaseline,
  HorPlus, Cinematic, AspectRecalculation, Dialogue, ZOOM, ADS, cached ENTER,
  hooks, resolvers and runtime behavior outside this consumer.
- Validation: focused and full harnesses PASS; production `build.cmd` PASS
  with known external Zydis C4201 warnings; targeted reader audit PASS;
  `git diff --check` PASS apart from normal line-ending warnings; no game
  launch.
- Completed: coherent production read, no legacy fallback, preserved defer /
  consume / restore behavior, current callback flags and diagnostics-gated
  reference comparison.
- Remaining: one combined post-cutover runtime validation session.
- Deferred: legacy-state deletion and cleanup.
- Blocked: none within this bounded batch.
- Patch summary: restoration consumer now has one attributable coherent
  production target while legacy state remains available for comparison.
- Changelog summary: `ApplyPendingGameplayModeTransition()` now restores from
  `GameplayAspectRestorationState` with fail-closed behavior.

## 2026-09-20 — Legacy gameplay aspect state cleanup

- Scope: remove only `g_lastObservedAspect` and
  `g_lastObservedAspectValid` after the production restoration cutover.
- Changed: `src/plugin/runtime.cpp`, cleanup task plan, cleanup report and this
  task-log entry.
- Not changed: `g_lastGameplayCameraSource/Fov`,
  `GameplayAspectRestorationStore`, GameplayBaseline, Dialogue, ZOOM/ADS,
  Cinematic, HorPlus, AspectRecalculation or cached ENTER behavior.
- Validation: no stale runtime/source references; relevant harnesses PASS; full
  `test.cmd` PASS; production `build.cmd` PASS with known external Zydis C4201
  warnings; `git diff --check` PASS apart from normal line-ending warnings; no
  game launch.
- Completed: removed legacy aspect fields, aliases, writes, comparison reads and
  legacy comparison telemetry; renamed the now-current-state trace helper.
- Remaining: one focused runtime regression after cleanup.
- Deferred: any cleanup of `g_lastGameplayCameraSource/Fov`.
- Blocked: none within this bounded batch.
- Patch summary: legacy aspect restoration state is no longer duplicated; the
  coherent restoration store remains the sole production authority.
- Changelog summary: removed obsolete legacy aspect state while preserving
  Dialogue camera-context invalidation state.

## 2026-09-20 — Global HorPlus consolidation foundation

- Scope: offline Global HorPlus production-path audit, deterministic hardening
  and runtime matrix preparation.
- Changed: expanded runtime evidence replay tests/fixtures, `test.cmd`, the
  consolidation task plan and the two Global HorPlus reports.
- Not changed: production FOV behavior, hooks, Dialogue/ZOOM ownership,
  cached-enter production guard, release artifacts and game state.
- Validation: full `test.cmd` PASS; production `build.cmd` PASS with known
  external Zydis C4201 warnings; `git diff --check` PASS apart from normal
  line-ending warnings; no game launch.
- Completed: Gameplay/Cinematic arbitrary-aspect/FOV/flags/invalid sweeps,
  baseline isolation and cross-state negative replay coverage.
- Remaining: runtime validation of resolver/native event ordering, visual
  framing, F11/F12, ADS/binocular and save/load matrix.
- Deferred: provenance-based cached ENTER replacement pending runtime evidence;
  no obsolete production state was proven for deletion.
- Blocked: none within this bounded batch.
- Patch summary: consolidated deterministic evidence around the existing common
  HorPlus transform and preserved fail-closed behavior.
- Changelog summary: expanded `test.cmd` to cover Global HorPlus invariants and
  prepared one compact runtime regression matrix.

## 2026-09-20 — Runtime evidence regression corpus foundation

- Scope: add deterministic replay fixtures and one test harness from established
  gameplay, restoration, cinematic and Dialogue evidence.
- Changed: `tests/fixtures/runtime/`,
  `tests/regression/runtime_evidence_replay_harness.cpp`, `test.cmd`, the task
  plan and the foundation report.
- Not changed: production behavior, hooks, resolvers, game runtime, release
  artifacts and existing harness implementations.
- Validation: replay harness PASS; full `test.cmd` PASS; production `build.cmd`
  PASS with known external Zydis C4201 warnings; `git diff --check` PASS apart
  from normal line-ending warnings; no game launch.
- Completed: four evidence-backed scenarios and 16 deterministic assertion
  points, including arbitrary-aspect and fail-closed coverage.
- Remaining: runtime-only resolver, UE ordering, visual and integration checks.
- Deferred: additional recorded 80/100/110 baseline and recovery trajectory
  fixtures after their provenance contracts are finalized.
- Blocked: none within this bounded batch.
- Patch summary: established a minimal machine-readable runtime evidence corpus
  without simulating UE or changing production code.
- Changelog summary: `test.cmd` now replays recorded FOV/restoration/cinematic/
  Dialogue invariants automatically.

## 2026-09-20 — Global HorPlus remaining functionality

- Scope: deterministic remainder of the Global HorPlus consolidation; no
  production hook or FOV behavior changes.
- Changed: `tests/regression/runtime_evidence_replay_harness.cpp`,
  `tests/config/config_persistence_harness.cpp`, `test.cmd`, the runtime
  matrix, the task plan and the batch report.
- Not changed: production transforms, cached ENTER guard, hooks/resolvers,
  GameplayBaseline ownership, Dialogue/ZOOM behavior, release artifacts or
  game state.
- Validation: full `test.cmd` PASS; `build.cmd` PASS with known external Zydis
  C4201 warnings; `git diff --check` PASS apart from normal line-ending
  warnings; no game launch.
- Completed: arbitrary-aspect/FOV sweeps, dynamic-aspect continuity, mode and
  cinematic policy cycles, stale-state negatives, fail-closed inputs and
  Runtime Evidence Replay extension.
- Remaining: one combined runtime session for resolver/order, visual framing,
  live F11/F12, Auto/forced display policy, save/load, Dialogue coexistence
  and legacy-aspect cleanup regression.
- Deferred: cached ENTER provenance replacement and any diagnostic/transitional
  state deletion pending separate evidence and ownership review.
- Blocked: none within this bounded batch.
- Patch summary: deterministic Global HorPlus invariants are now covered by
  `test.cmd`, including arbitrary runtime-aspect inputs and cross-state stale
  evidence protection.
- Changelog summary: expanded offline regression coverage and consolidated the
  remaining runtime checks into one matrix without changing production camera
  behavior.

## 2026-09-20 — V1 remaining work inventory and camera cleanup

- Scope: record the combined Global HorPlus runtime PASS, audit camera/FOV
  state and diagnostics, remove only unambiguous accumulated debt, and define
  the next v1 work pack.
- Changed: `src/plugin/runtime.cpp`, the Runtime Evidence Replay cinematic
  fixture/harness, `research/reports/GLOBAL_HORPLUS_RUNTIME_PASS.md`,
  `research/reports/GLOBAL_HORPLUS_RUNTIME_MATRIX.md`,
  `research/reports/CAMERA_ARCHITECTURE_CLEANUP.md`,
  `research/reports/V1_REMAINING_WORK_INVENTORY.md`, the task plan and this
  task-log entry.
- Not changed: production transform math, hooks/resolvers, GameplayBaseline,
  restoration semantics, Dialogue, ZOOM/ADS behavior, cached ENTER guard,
  release artifacts or game state.
- Validation: focused/full `test.cmd` PASS; production `build.cmd` PASS with
  known external Zydis C4201 warnings; `git diff --check` PASS apart from
  normal line-ending warnings; no game launch.
- Completed: recorded the user-provided combined runtime milestone, added a
  minimal forced-21:9 evidence fixture, removed duplicate `EARLY_CB ASPECT`
  callback logging and renamed two restoration helpers away from stale
  `Shadow` terminology.
- Remaining: no confirmed user-visible implementation gap after the runtime
  PASS; future resolver compatibility and configuration/diagnostic maintenance
  remain available as bounded follow-up work.
- Deferred: cached ENTER provenance research and any diagnostic/transitional
  state deletion not proven obsolete.
- Blocked: none within this bounded batch.
- Patch summary: reduced misleading callback telemetry while preserving every
  production owner and extended the corpus with the observed forced-aspect
  cinematic result.
- Changelog summary: documented the V1 runtime milestone and replaced stale
  restoration naming without changing camera behavior.

## 2026-09-20 — Architecture A1 Dialogue capability repair

- Scope: make the established Cinematic EXIT observation and Gameplay recovery
  dependency explicit for non-Native Dialogue; no new hook or classifier
  redesign.
- Changed: `src/plugin/feature_status.*`, `src/plugin/runtime.cpp`, the feature
  status harness, `docs/SAFETY_INVARIANTS.md`, `docs/ARCHITECTURE.md`, the A1
  task plan, and the two A1 reports/runtime matrix.
- Not changed: Dialogue classification/recovery math, ZOOM/ADS, Gameplay or
  cinematic transforms, resolvers, persistence, release artifacts, or game
  state.
- Validation: full `test.cmd` PASS; production `build.cmd` PASS; diagnostic
  `build-diagnostic.cmd` PASS; `git diff --check` PASS apart from normal
  line-ending warnings; no game launch.
- Completed: observation-only cinematic lifecycle installation, explicit
  non-Native Dialogue capability gating, Native Dialogue independence, and
  hotkey fail-closed behavior when required capabilities are unavailable.
- Remaining: runtime matrix execution on the next planned game session.
- Deferred: any alternate recovery seam or broader cross-feature lifecycle
  redesign.
- Blocked: none within this bounded batch.
- Patch summary: dependent Dialogue lifecycle behavior now fails closed when
  its two established observation capabilities are absent while unrelated
  feature status remains isolated.
- Changelog summary: clarified Dialogue's lifecycle capability dependency and
  preserved Native Dialogue and independent feature behavior.

## 2026-09-20 — Security/Safety repair batch S1–S7

- Scope: bounded repair of confirmed native-boundary, resolver, cinematic
  transaction, persistence, viewport, worker-lifecycle, and SafetyHook findings.
- Changed: callback/worker containment in `src/plugin/runtime.cpp` and
  `src/plugin/worker_lifecycle.*`; span-bounded resolver validation in
  `src/hooks/*`, `src/gameplay/gameplay_camera.cpp`, and
  `src/cinematics/cinematic_aspect.cpp`; cinematic commit gating; config
  staging flush/close checks; viewport plausibility; bounded SafetyHook trap
  cleanup/protection failure propagation; focused harnesses and `test.cmd`;
  the security/safety report and task plan.
- Not changed: camera algorithms, Dialogue classifier, HorPlus behavior,
  resolver uniqueness policy, new UE hooks, release artifacts, or game state.
- Validation: `test.cmd` PASS; `build.cmd` PASS; `build-diagnostic.cmd` PASS;
  `git diff --check` PASS apart from normal line-ending warnings; no game launch.
- Completed: S4, S5, and S6 with deterministic evidence.
- Partial: S1, S2, S3, and S7 retain explicit validation/provenance gaps as
  documented in `research/reports/SECURITY_SAFETY_REPAIR_BATCH.md`.
- Deferred: OS-level failure injection, exact SafetyHook upstream provenance,
  and full resolver near-end fault matrix.
- Blocked: none within the bounded batch.
- Patch summary: tightened native callback containment, resolver read bounds,
  cinematic installation commit ordering, durable config replacement,
  viewport plausibility, worker cleanup, and SafetyHook transaction cleanup.
- Changelog summary: added bounded safety hardening with honest partial status
  for findings that still require platform/runtime evidence.

## 2026-09-20 — Architecture A2–A6 cleanup

- Scope: bounded architecture naming/ownership correction, factual RuntimeState
  documentation, source-only callback affinity inventory, and Dialogue
  classifier documentation verification.
- Changed: added neutral `src/camera/presentation_state.*` for the shared
  presentation coordinator state; added neutral `src/camera/horplus.*` for the
  shared projection primitive; updated gameplay/cinematic/plugin callsites,
  harness link inputs and `docs/ARCHITECTURE.md`; added
  `research/reports/ARCHITECTURE_A2_A6_CLEANUP.md`.
- Not changed: runtime integration ownership, Dialogue classifier behavior,
  hooks/resolvers, synchronization, performance, release artifacts, or game
  state.
- Validation: `test.cmd` PASS; `build.cmd` PASS; `build-diagnostic.cmd` PASS;
  `git diff --check` PASS apart from normal line-ending warnings; no game
  launch.
- Completed: confirmed shared CoordinatorState and HorPlus ownership names
  are neutral and single-implemented; recorded callback affinity as not
  established from source; preserved honest Candidate/Active semantics.
- Remaining: none within this bounded batch.
- Deferred: stronger native callback thread-affinity evidence and any broader
  runtime integration extraction.
- Blocked: none within this bounded batch.
- Patch summary: moved shared camera concepts to neutral modules without
  changing algorithms or lifecycle behavior.
- Changelog summary: corrected shared presentation/projection ownership names
  and retained the existing runtime integration boundary.

## 2026-09-20 — Realtime/threading/lifecycle repair batch

- Scope: bounded R1–R7 lifecycle, hook publication, controlled shutdown,
  diagnostic TLS and snapshot-coherence review after A2–A6.
- Changed: added immutable cinematic selection capture in
  `src/cinematics/cinematic_selection.*`; added Gameplay/Cinematic callback
  gates and stopping state in `src/plugin/runtime.cpp`; made reset honor failed
  worker joins; replaced Dialogue discovery worker-thread TLS reset with an
  owner-callback generation reset; closed the same gates on minimal detach;
  added the selection harness and linked the new production/diagnostic source.
- Not changed: Dialogue classifier, camera math, SafetyHook dependency shape,
  performance/logging frequency, new UE hooks, release artifacts or game
  state.
- Validation: `test.cmd` PASS; `build.cmd` PASS; `build-diagnostic.cmd` PASS;
  `git diff --check` PASS apart from normal line-ending warnings; no game
  launch.
- Completed: R1 selection snapshot, R2 confirmed Gameplay/Cinematic gates,
  and R6 diagnostic TLS reset.
- Partial: R3 controlled shutdown, R4 SafetyHook transaction proof and R5
  worker-start OS failure proof remain limited by unavailable OS-level failure
  injection. R7 had no proven mixed-generation defect and received no new
  synchronization.
- Deferred: runtime callback ordering, shutdown/resource observation and OS
  failure scenarios added to the combined runtime matrix; performance items
  remain with the Performance audit.
- Blocked: none within this bounded batch.
- Patch summary: prevented cross-generation cinematic selection and callback
  exposure during activation/stopping while preserving safe failure behavior.
- Changelog summary: added lifecycle publication gates and diagnostic reset
  correctness without changing camera algorithms.

## 2026-09-20 — Persistence/runtime state/recovery repair batch

- Scope: bounded P1–P4 repair after the persistence/runtime-state audit.
- Changed: authoritative cinematic selection acceptance and active-snapshot
  aspect resolution; exact aspect-only native fallback; shared gameplay
  context-change evaluation for Dialogue invalidation in both Gameplay paths;
  deterministic duplicate managed-key persistence; explicit initial INI
  flush/close; focused harness coverage and the repair report.
- Not changed: recovery liveness semantics, Candidate/ZOOM redesign, camera
  algorithms, new hooks, release artifacts, or game state.
- Validation: `test.cmd` PASS; `build.cmd` PASS; `build-diagnostic.cmd` PASS;
  `git diff --check` PASS apart from normal line-ending warnings; no game
  launch.
- Completed: P1 cinematic snapshot authority, P2 native aspect-only fallback,
  P3 cross-mode Dialogue context invalidation, and P4 deterministic config
  persistence semantics.
- Remaining: runtime validation of duplicate ENTER, closed/stopping native
  fallback, cross-mode Dialogue context changes, and recovery liveness.
- Deferred: CinematicExiting terminal behavior, post-cinematic exclusion
  completion and unreadable/invalid recovery-target behavior.
- Blocked: none within the bounded batch.
- Patch summary: made lifecycle snapshots authoritative at the cinematic
  boundary, removed flag writes from the closed fallback, unified Dialogue
  invalidation evidence, and made persisted duplicate values converge.
- Changelog summary: hardened runtime-state boundaries and configuration
  persistence without changing the camera algorithms or launching the game.

## 2026-09-20 — Tests/harnesses/validation production contract batch

- Scope: bounded T1–T3C validation repair after the independent Tests /
  Harnesses / Validation audit.
- Changed: authoritative `test.cmd` inventory/self-audit; immediate compile
  and run failure propagation; cinematic aspect and Dialogue FOV harnesses;
  instruction/rel32/span coverage; camera write/resolver safety coverage;
  production `IsWritable` coverage; runtime-fixture provenance metadata; and
  misleading test labels.
- Not changed: production behavior, hooks, UE simulation, runtime claims,
  performance, visual validation, release artifacts, or game state.
- Validation: runner self-audit PASS (`31=31=31`); `test.cmd` PASS;
  `build.cmd` PASS; `build-diagnostic.cmd` PASS; `git diff --check` PASS
  apart from normal line-ending warnings; no game launch.
- Completed: T1 runner correctness, T2 cinematic/Dialogue production math
  coverage, and T3 instruction/span, writability and camera safety coverage.
- Partial: invalid-operand camera resolver coverage is unreachable through the
  fixed public signature without a new seam; documented rather than changing
  the production signature contract.
- Deferred: runtime hook installation/order, native thread affinity, visual
  framing, save/load recreation, recovery liveness and performance.
- Blocked: none within the bounded batch.
- Patch summary: made the test runner authoritative and expanded deterministic
  protection around previously uncovered production contracts.
- Changelog summary: `test.cmd` now proves every registered harness ran and
  added production-contract coverage without introducing a test framework.

## 2026-09-21 — Overlay D3D12 discovery continuation

- Scope: bounded optional overlay discovery instrumentation after the
  lifecycle-only POC; no rendering or production camera changes.
- Changed: isolated DXGI factory/swapchain discovery ASI, bounded queue/Present
  evidence store and harness, experimental build entrypoint, and the overlay
  POC report. Production `build.cmd` source list was untouched.
- Validation: `test.cmd` PASS (`33/33`); `build.cmd` PASS;
  `build-overlay-discovery.cmd` PASS; `git diff --check` PASS with the normal
  `test.cmd` line-ending warning; no game launch.
- Completed: presentation discovery instrumentation and deterministic
  evidence handling are ready for runtime observation.
- Remaining: actual STALKER 2 swapchain/queue relationship is not runtime
  validated; rendering and ImGui remain unimplemented.
- Deferred: any queue selection, graphics submission, overlay input or
  settings UI until runtime evidence supports a separate batch.
- Blocked: none within this bounded batch.
- Patch summary: added observational DXGI discovery in a separate ASI and
  fail-closed candidate correlation without changing production behavior.
- Changelog summary: prepared a controlled runtime discovery artifact for the
  post-v1 overlay POC; no overlay rendering was enabled.

## 2026-09-21 — Overlay DXGI factory coverage repair

- Scope: bounded repair after the first discovery runtime observed factory
  exports but no swapchain creation; rendering and queue policy remained out
  of scope.
- Changed: corrected `IDXGIFactory2` method slots, added
  `CreateSwapChainForCoreWindow` coverage, method-specific invocation
  telemetry and identity/method bookkeeping tests. Updated the overlay report.
- Validation: `test.cmd` PASS (`33/33`); `build.cmd` PASS;
  `build-overlay-discovery.cmd` PASS; `git diff --check` PASS with normal
  line-ending warnings; no game launch after the repair.
- Completed: static factory coverage repair and deterministic bookkeeping
  coverage.
- Remaining: a new runtime session is required to confirm which swapchain
  creation path STALKER 2 uses and to reach Present/queue evidence.
- Deferred: rendering, queue selection, ImGui, input and settings UI.
- Blocked: none within this bounded batch.
- Patch summary: expanded observational DXGI factory coverage without changing
  production camera behavior.
- Changelog summary: corrected factory method interception and prepared
  method-level evidence for the next overlay discovery run.

## 2026-09-21 — Overlay association lifecycle repair

- Scope: separate resize resource invalidation from observed swapchain/device/
  queue association; no rendering or production camera changes.
- Changed: retained structural `S↔D↔Q` evidence across successful resize,
  added Present revalidation states, failed-resize invalidation, identity
  accessors, telemetry fields and deterministic lifecycle coverage. Updated
  the overlay POC report.
- Validation: `test.cmd` PASS (`33/33`); `build.cmd` PASS;
  `build-overlay-discovery.cmd` PASS; `git diff --check` PASS with normal
  line-ending warnings; no game launch.
- Completed: static association lifecycle repair and fail-closed tests for
  resize failure, new swapchain, changed device and ambiguity.
- Remaining: one runtime session must verify `SUPPORTED` before resize,
  `RESIZE_REVALIDATION_REQUIRED` during resize and `SUPPORTED` after the next
  Present.
- Deferred: rendering, queue selection, ImGui, input and settings UI.
- Blocked: none within this bounded batch.
- Patch summary: stopped treating `ResizeBuffers` as queue-loss and made
  revalidation explicit without selecting a queue heuristically.
- Changelog summary: resize-aware presentation association is ready for the
  next discovery regression session; graphics submission remains disabled.

## 2026-09-21 — Overlay D3D12 Dear ImGui rendering POC

- Scope: isolated first rendering POC over the runtime-validated swapchain,
  device and DIRECT-queue association; no production camera or settings work.
- Changed: vendored Dear ImGui v1.91.9b, added renderer resource/fence and
  resize lifecycle ownership, static non-interactive status window, renderer
  state harness and `build-overlay-poc.cmd` experimental artifact.
- Validation: `test.cmd` PASS (`34/34`); `build.cmd` PASS;
  `build-overlay-discovery.cmd` PASS; `build-overlay-poc.cmd` PASS;
  `git diff --check` pending final review; no game launch.
- Completed: deterministic renderer eligibility, resize and fail-closed model;
  experimental graphics submission is compiled and isolated.
- Remaining: controlled runtime rendering gate for visible frames and resize.
- Deferred: input, settings integration, production promotion and UI design.
- Blocked: none within this bounded batch.
- Patch summary: added the first isolated D3D12/ImGui submission path without
  changing the production ASI.
- Changelog summary: experimental static overlay window is ready for runtime
  validation; production rendering remains disabled.

## 2026-09-21 — Overlay interactive shell

- Scope: experimental overlay visibility toggle and bounded Win32/ImGui input
  routing; no settings or camera integration.
- Changed: added `InputState`, Insert toggle, guarded WndProc forwarding,
  visible/hidden renderer state and deterministic input harness.
- Validation: `test.cmd` PASS (`35/35`); `build.cmd` PASS;
  `build-overlay-poc.cmd` PASS; `git diff --check` pending final review;
  no game launch.
- Completed: hidden overlay leaves game input path intact; visible overlay
  consumes keyboard/mouse messages for ImGui; renderer remains isolated.
- Remaining: runtime validation of Insert toggle and input capture semantics.
- Deferred: settings API, cursor/focus policy, raw input and production
  promotion.
- Blocked: none within this bounded batch.
- Patch summary: added the minimum interactive shell over the validated
  renderer without introducing a second settings/state owner.
- Changelog summary: experimental overlay can now be shown or hidden with
  Insert; runtime input behavior remains pending validation.

## 2026-09-21 — Shared runtime settings mutation boundary

- Scope: one typed production mutation boundary for existing F9-F12 hotkeys;
  no overlay controls or settings UI.
- Changed: added `RuntimeSettingsApi` and typed mutations for Gameplay mode,
  Cinematic aspect/FOV policies and Dialogue zoom; migrated F9-F12 to use it.
- Validation: `test.cmd` PASS (`36/36`); `build.cmd` PASS;
  `git diff --check` pending final review; no game launch.
- Completed: hotkeys retain their existing state/persistence behavior while
  sharing one typed mutation seam; mismatched mutation values fail closed.
- Remaining: one runtime hotkey regression and later overlay settings wiring.
- Deferred: ImGui controls, settings ownership migration and UI persistence.
- Blocked: none within this bounded batch.
- Patch summary: centralized runtime setting mutations without introducing a
  second settings owner or changing camera algorithms.
- Changelog summary: F9-F12 are ready to be runtime-validated through the
  shared settings boundary.

## 2026-09-21 — Overlay input capture repair

- Scope: narrow the experimental overlay shell's Win32 capture to ImGui's
  requested keyboard/mouse capture state.
- Changed: added separate capture predicates and forwarded unrequested input
  to the original game WndProc; Insert toggle behavior is unchanged.
- Validation: `test.cmd` PASS (`36/36`); `build-overlay-poc.cmd` PASS;
  `git diff --check` pending final review; no game launch.
- Completed: static POC no longer intentionally consumes all game input while
  visible.
- Remaining: runtime confirmation with gameplay clicks and keyboard input.
- Deferred: raw input, cursor/focus policy and settings integration.
- Blocked: none within this bounded repair.
- Patch summary: changed input routing from unconditional capture to
  ImGui-requested capture only.
- Changelog summary: overlay shell is ready for a focused input regression.

## 2026-09-22 — Overlay focus/flicker instrumentation

- Scope: add low-rate timestamps for focus/cursor and resize correlation; no
  overlay behavior changes.
- Changed: timestamped focus/activation messages, first WM_SETCURSOR after a
  focus transition, overlay visibility/cursor restore, ResizeBuffers begin/end,
  and renderer resize-rebuild telemetry.
- Validation: `test.cmd` PASS (`36/36`); `build-overlay-settings.cmd` PASS;
  `git diff --check` PASS; no game launch.
- Completed: logs can distinguish the Alt+Tab A/B sequence with the overlay
  hidden and visible without high-rate cursor-message spam.
- Remaining: user-run hidden/visible Alt+Tab test and review of the resulting
  timestamps; flicker cause is not yet established.
- Deferred: any focus/cursor/rendering behavior repair pending runtime evidence.
- Blocked: none within this bounded instrumentation batch.
- Patch summary: added timestamped focus, cursor-transition and resize
  telemetry without changing input or rendering decisions.
- Changelog summary: overlay logs now contain the event ordering needed to
  diagnose focus-switch flicker.

## 2026-09-22 — Overlay focus-debug cleanup

- Scope: remove temporary focus/Alt+Tab telemetry after the flicker was
  reproduced with the overlay hidden, visible, and with the mod absent.
- Changed: removed focus-message and first-cursor-after-focus logging plus its
  timestamp helper from `src/overlay/discovery_runtime.cpp`; preserved input,
  cursor, visibility, resize/rebuild behavior, and resize diagnostics. Added
  `research/reports/OVERLAY_FOCUS_FLICKER_RESULT.md` and archived the task plan.
- Git: branch `main` at `3120a05`; the working tree already contained unrelated
  modified and untracked files, which were preserved.
- Validation: `test.cmd` PASS (`36/36`); `build-overlay-settings.cmd` PASS;
  `git diff --check` PASS (existing CRLF notices only); no game launch during
  cleanup.
- Completed: temporary focus-debugging code is removed; the no-mod runtime
  control shows the tested flicker does not require this mod.
- Remaining: none within this cleanup scope.
- Deferred: further flicker investigation unless new contradictory evidence
  appears.
- Blocked: none.
- Not runtime-validated: the cleanup itself; runtime control evidence predates
  this diagnostic-only removal.
- Patch summary: removed investigation-only focus instrumentation without
  changing overlay behavior.
- Changelog summary: Alt+Tab flicker was confirmed with and without the mod;
  unnecessary focus telemetry has been removed.

## 2026-09-22 — Overlay semantic snapshot and integration statuses

- Scope: add a read-only semantic snapshot, pure Camera Integration projector,
  and contextual overlay status text; no camera behavior or hook changes.
- Changed: extended the settings API with per-fact validity/provenance/freshness;
  exposed existing runtime capabilities, pending flags, captured cinematic and
  dialogue lifecycle facts, retained-baseline usability, coordinator, and a
  strict current-process client viewport observation. Added the projector,
  two deterministic harnesses, test/build registration, and
  `research/reports/OVERLAY_SEMANTIC_SNAPSHOT.md`.
- Git: branch `main` at `3120a05`; working tree already contained unrelated
  modified and untracked work, which was preserved. No commit.
- Validation: `test.cmd` PASS (`38/38`); `build.cmd` PASS;
  `build-overlay-settings.cmd` PASS; `git diff --check` PASS with line-ending
  notices; no game launch or runtime test.
- Completed: UI distinguishes configured values, capabilities/pending state,
  and observed lifecycle facts without claiming an atomic/current-FOV view.
  Camera Integration fails closed for unavailable evidence and keeps
  NativeHorPlus independent from GameplayHorPlus.
- Remaining: combined runtime validation of the integrated overlay and its
  status presentation.
- Deferred: current native/effective FOV panel until active-path provenance and
  freshness are established; no camera behavior changes in this batch.
- Blocked: none within this bounded batch.
- Not runtime-validated: all new overlay semantic/status presentation and
  integrated artifact behavior.
- Patch summary: added evidence-backed overlay statuses and a pure deterministic
  integration assessment without adding camera decisions.
- Changelog summary: overlay now explains feature availability, pending state,
  active lifecycle evidence, and whether configured Gameplay/Cinematic paths
  are aligned, independent, or cannot be assessed.
