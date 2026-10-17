# Backlog Reconciliation — 2026-10-17

## Purpose and method

Inventory of every Markdown document in `backlog/` before cleanup, cross-checked against the current backlog index, task logs, research archive, current project summary, supported-build manifest, and dated reports. The disposition below records both the review and the cleanup performed on 2026-10-17; no historical plan was deleted.

Inventory: **62 Markdown files** — 40 under `backlog/active/`, 19 task/playbook documents at the backlog root, and `README.md`, `TASKLOG.md`, `TASKLOG_PREVIOUS.md`. The README listed only nine entries, including one broken path, and therefore was not a reliable active-work index.

Disposition meanings:

- **Keep active** — a concrete task still appears actionable; refresh its game-build identity and scope before execution where noted.
- **Archive completed** — the bounded task/implementation is complete; retain its result and limitations in the document/report.
- **Defer / re-scope** — the question is unresolved, blocked, superseded, or tied to stale evidence; do not execute the old plan as written.
- **Historical archive** — retain the plan as provenance, but it is not current work. Its exact research outcome should remain in the body/report rather than being reinterpreted as a new PASS.
- **Move to process documentation** — reusable procedure rather than a development task.

## Plans deferred after user review

| File | Recommended status | Reason / required follow-up |
|---|---|---|
| `CINEMATIC_RUNTIME_STATE_DIFFERENTIAL_TRACE_TASK_PLAN.md` | Deferred at user's direction | The unresolved natural-exit lifecycle question is tied to older game images and has no current priority. Revalidate supported-build identity before any future trace. |
| `CUTSCENE_FOV_DOWNSTREAM_STATE_TASK_PLAN.md` | Deferred at user's direction | The downstream state/blend analysis targets 2.0.3; it is not current-build evidence. |
| `CUTSCENE_HORPLUS_IMPLEMENTATION_TASK_PLAN.md` | Deferred at user's direction | Runtime acceptance and transition correctness remain gates; the 2.0.3 hook contract must be revalidated before any future implementation. |
| `CUTSCENE_HORPLUS_TRANSITION_BOUNDARY_TASK_PLAN.md` | Deferred at user's direction | Its diagnostic boundary remains unresolved and is not currently prioritized. |
| `PRODUCT_IDENTITY_RENAME_TASK_PLAN.md` | Deferred at user's direction | Only a public-title mismatch remained; it is not current active work. |
| `RESEARCH_PROVENANCE_BLAST_RADIUS_AUDIT_TASK_PLAN.md` | Deferred at user's direction | Batch 3 was queued against a 2.0.4 baseline; refresh only if this audit is reprioritized. |
| `VORTEX_OVERRIDE_INSTRUCTIONS_ARCHIVE_TASK.md` | Deferred at user's direction | The task's installer value is undecided and the current package allowlist excludes the Vortex JSON. |

All seven were moved from `backlog/active/` to `research/deferred/`; no current task plan remains active.

## `backlog/active/` — archive or defer

### Bounded work completed

| File | Recommended disposition | Evidence / boundary |
|---|---|---|
| `active/CINEMATIC_CONTINUATION_AND_RESTORATION_LOG_FIX_2026-10-09.md` | Archive completed | The bounded logging and rapid-reentry fixes and offline validation are recorded in `research/reports/CINEMATIC_CONTINUATION_AND_RESTORATION_LOG_FIX_2026-10-09.md`. The broader 0-FPS cause remained unresolved; do not conflate it with this completed scope. |
| `active/LOCALIZATION_18_LOCALES_TASK_PLAN.md` | Archive completed | The 18-locale implementation and scoped offline validation are present in current source and later regression evidence. Its “closure pending confirmation” is administrative, not unfinished implementation. Retain its RTL/shaping limits. |
| `active/MODULAR_GAMEPLAY_CINEMATICS_DIALOGUE_REFACTOR_TASK_PLAN.md` | Archive completed | The plan marks its implementation, integrated regressions, suite and production build complete. |
| `active/NATIVE_RUNTIME_EVIDENCE_B1_B7_TASK_PLAN_2026-10-09.md` | Archive completed | Its own final status records the audit and evidence work complete; remaining runtime-only limits are documented. |
| `active/RUNTIME_CPP_DECOMPOSITION_IMPLEMENTATION_PLAN_2026-10-08.md` | Archive completed | The plan and final checkpoint state B1–B7 closed with explicit Remain decisions. Do not reopen file splitting by default. |
| `active/RUNTIME_CPP_DECOMPOSITION_POST_V3_0_RETURN.md` | Archive as superseded reminder | The “after v3.0” trigger has passed and the parent decomposition is complete. |
| `active/PANINI_CVAR_RUNTIME_FEASIBILITY_TASK_PLAN.md` | Archive completed / negative production result | CVar tuning was exhausted; Panini was active but the tested output had a persistent visual artifact. See its result report. |
| `active/PANINI_RENDER_PASS_ARTIFACT_ISOLATION_TASK_PLAN.md` | Archive completed / bounded negative | Tested toggles did not isolate the artifact. The report explicitly closes blind CVar tuning and requires a new evidence class for any future work. |

### Unresolved or superseded investigations — defer, do not execute as written

| File | Recommended disposition | Why it is not current active work |
|---|---|---|
| `active/ADS_WORLD_FOV_VIEWMODEL_CONSUMER_TASK_PLAN.md` | Defer | It awaits a 2.0.4 runtime capture, while current project conclusions defer production viewmodel repair until a safe bridge/owner is established. Resume only with a current-build anchor and a specific user decision. |
| `active/CAMERA_MODULE_REMEDIATION_F1_F8_TASK_PLAN.md` | Archive as historical plan; track any surviving finding separately | It is a large 2.0.4-era plan with F1–F8 stages already dispositioned and F9 explicitly separate. Current camera-state/provenance records supersede it as the live status source. |
| `active/CINEMATIC_ASPECT_RATIO_AXIS_CONSTRAINT_OWNERSHIP_TASK_PLAN.md` | Defer | Bounded ownership/projection searches found no promotable current-build boundary; further work requires a new independent anchor. |
| `active/CINEMATIC_AXIS_CONSTRAINT_COOKED_CONTENT_AUDIT_TASK_PLAN.md` | Defer / re-scope | Content metadata is a bounded lead, not an established runtime owner; the plan still has blocked native/content correlation and targets old image evidence. |
| `active/CINEMATIC_AXIS_CONSTRAINT_RUNTIME_REFLECTION_TASK_PLAN.md` | Defer | Bounded reflection work did not establish a promotable effective consumer; later batches are explicitly blocked. |
| `active/CINEMATIC_AXIS_POLICY_OUTPUT_OWNERSHIP_AND_RUNTIME_LIFECYCLE_TASK_PLAN.md` | Archive completed bounded investigation; defer ownership question | Its recorded result is negative activation evidence with downstream application unresolved. No production change follows from the partial observation. |
| `active/CINEMATIC_PROJECTION_REMAP_FEASIBILITY_TASK_PLAN.md` | Defer | The plan says the two bounded static directions are exhausted and the next batch is blocked pending a genuinely new anchor. |
| `active/LETTERBOX_REAUDIT_TASK_PLAN.md` | Defer and revalidate from current executable identity | The experiment was safe in the tested 2.0.3 scenario but produced incorrect cutscene FOV/projection. It is not release-ready and must not be retried in its existing form. |
| `active/UE5_PROJECTION_CONSTRUCTION_OWNERSHIP_TASK_PLAN.md` | Defer | Batch 1 found no confirmed current-build projection owner; subsequent work is blocked pending new evidence. |
| `active/D3D12_NOTIFICATION_OVERLAY_FEASIBILITY_TASK_PLAN.md` | Archive/defer the alternative overlay branch | Its COM/VMT path was blocked as unsafe. Production now uses the Overlay-owned HWND and does not need this swapchain-rendering route. Preserve its failure evidence. |
| `active/DXGI_COM_SAFE_SWAPCHAIN_INTERCEPTION_TASK_PLAN.md` | Archive/defer | Planned dependency of the blocked D3D12 overlay branch; obsolete for the implemented Overlay-owned HWND architecture. |
| `active/EXISTING_ACTIVE_SWAPCHAIN_OWNERSHIP_TASK_PLAN.md` | Archive/defer | Gate A found no independent acquisition anchor and explicitly says not to continue without new evidence. |

### Historical version-specific plans

These 13 `v1.0.0` plans describe earlier product/release cycles. Archive each according to its own final result; none should appear as a current task merely because it remains under `active/`:

- `active/V1_0_0_BATCH4_PRODUCTION_REGRESSION_TASK_PLAN.md`
- `active/V1_0_0_DELAYED_ASPECT_HANDOFF_DIAGNOSTIC_TASK_PLAN.md`
- `active/V1_0_0_FINAL_ARCHITECTURE_HARDENING_TASK_PLAN.md`
- `active/V1_0_0_FINAL_TECHNICAL_RELEASE_TASK_PLAN.md`
- `active/V1_0_0_HORPLUS_21_9_ELIGIBILITY_TASK_PLAN.md`
- `active/V1_0_0_HORPLUS_CINEMATIC_DOUBLE_TRANSFORM_TASK_PLAN.md`
- `active/V1_0_0_HORPLUS_CINEMATIC_WRITER_TRACE_TASK_PLAN.md`
- `active/V1_0_0_HORPLUS_DIAGNOSTIC_LOGGING_SANITY_TASK_PLAN.md`
- `active/V1_0_0_HORPLUS_DYNAMIC_STATE_TELEMETRY_TASK_PLAN.md`
- `active/V1_0_0_HORPLUS_GAMEPLAY_DIAGNOSTIC_POC_TASK_PLAN.md`
- `active/V1_0_0_NATIVE_GAMEPLAY_BASELINE_DIAGNOSTIC_TASK_PLAN.md`
- `active/V1_0_0_OFF_POST_EXIT_DIAGNOSTIC_OBSERVER_TASK_PLAN.md`
- `active/V1_0_0_POST_EXIT_CAMERA_STATE_TRACE_TASK_PLAN.md`

## Backlog-root plans — no longer active backlog entries

| Files | Recommended disposition |
|---|---|
| `ASI_FOV_TRANSITION_SMOOTHING_TASK_PLAN.md` | Defer; no current approval/evidence shows this remains a required feature. |
| `ATOMIC_GAMEPLAY_MUTATION_DEDUP_REFACTOR_TASK_PLAN.md`; `COMBINED_ATOMIC_CINEMATIC_GAMEPLAY_HANDOFF_CANDIDATE_TASK_PLAN.md`; `GAMEPLAY_FIX_ATOMICITY_TEST_TASK_PLAN.md` | Archive as historical atomicity work. The plans describe bounded experiments/variants; preserve their runtime-validation boundaries rather than promoting an old research artifact. |
| `POST_CINEMATIC_GAMEPLAY_ATOMIC_EXIT_HANDOFF_TEST_TASK_PLAN.md`; `POST_CINEMATIC_GAMEPLAY_REPLAY_AT_RECOVERY_TEST_TASK_PLAN.md`; `POST_CINEMATIC_GAMEPLAY_REPLAY_ATOMICITY_TEST_TASK_PLAN.md`; `POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST_TASK_PLAN.md` | Defer/archive the old replay-timing branches. Current production recovery/transaction contracts and later F2/F4 work supersede these experimental plans; do not revive timers/replays without new evidence. |
| `CINEMATIC_FOV_TRANSITION_BYPASS_PASS1_TASK_PLAN.md` | Archive the completed observation-only pass. |
| `CINEMATIC_FOV_TRANSITION_BYPASS_PASS2_DATA_WRITE_TASK_PLAN.md`; `CINEMATIC_FOV_TRANSITION_BYPASS_PASS4_RUNTIME_CALLER_TASK_PLAN.md` | Defer. Required debugger/caller evidence was not established; no production bypass is justified. |
| `POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE_TASK_PLAN.md`; `POST_EXIT_FOV_HANDOFF_INTEGRATED_TRACE_TASK_PLAN.md`; `POST_EXIT_FOV_STATE_CONSUMER_TRACE_TASK_PLAN.md`; `POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE_TASK_PLAN.md`; `POST_EXIT_KNOWN_WRITER_CALLSITE_AUDIT_205_TASK_PLAN.md` | Archive as completed bounded research stages, preserving each plan's unresolved caller/owner limits and version identity. They are not standalone active implementation tasks. |
| `POST_EXIT_FOV_PRODUCER_STORE_TRACE_205_TASK_PLAN.md` | Defer pending confirmation of the exact recorded result. Do not infer beyond the tested 2.0.5 scenario. |
| `POST_EXIT_NATIVE_FOV_HANDOFF_RESEARCH_TASK_PLAN.md` | Defer. The plan itself says no native caller/transition anchor was recovered and requires a new independent anchor. |
| `RELEASE_PREPARATION_TASK_PLAN.md` | Keep at the backlog root as a permanent process reference. Its own canonical contract explicitly forbids moving or archiving it; it is indexed separately from active tasks. |

## Index and path defects

1. Before cleanup, `backlog/README.md` listed 9 active tasks, with stale/missing paths, and omitted 31 documents under `backlog/active/`.
2. The Vortex task's decomposition prerequisite was complete; its package-content requirement still needed an explicit decision because the package allowlist excludes the JSON.
3. The localization “closure pending confirmation” note was stale administrative status, not unfinished implementation.
4. Root-level plans mixed historical tasks with process guidance and task logs.

## Cleanup completed

- Moved **31 completed/historical plans** to `research/completed/`, **24 deferred plans** to `research/deferred/`, and **3 rejected/superseded plans** to `research/rejected/`.
- Left `backlog/active/` with no task plans; updated `backlog/README.md` to state that there is no active plan and to point to the disposition report.
- Kept `RELEASE_PREPARATION_TASK_PLAN.md` in place because its own process contract says it is permanent and must not be moved or archived.
- Kept both task logs intact; no historical log entries were rewritten.
- Updated known current cross-references to moved plans. Older tasklog entries remain historical snapshots of their original paths.
- Verified `backlog/active/` is empty and the README's report/process links resolve. No plan or source was deleted.

No source code, INI, ASI, release package, test suite or game session was changed or launched for this cleanup.
