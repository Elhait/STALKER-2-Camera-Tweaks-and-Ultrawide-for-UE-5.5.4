# ASI update runtime failure — analysis plan

## Log naming refinement — user authorized 2026-11-16

Use literal `.PID.log` as the fixed fallback filename, not a numeric process
suffix. Keep canonical names when available. Both slots remain exclusive and
are truncated only when a new session successfully opens them; a third writer
must not corrupt either active owner's records. Main/Overlay logs must state
the actual process ID at startup; Startup already has a PID header.

Extend existing log-ownership regression to assert fixed fallback, reuse with
truncation, original-content preservation and refusal while both slots are
occupied. The regression was observed FAIL before this refinement. Then run
focused lifecycle/startup harnesses, build ordinary Release and provide link.
No automatic deletion of historical numeric logs from the game directory.
No FOV, configuration, driver or resource-policy changes in this refinement.

Refinement result: regression FAIL before and PASS after; lifecycle/native-log
and startup-journal harnesses PASS. Standard production Release built and
published only in the project root, SHA-256
`2E93B844C5BE2E3F383C2EBD90F99B966F7C854661A4E8711A104EA065EA9368`.
Supervisor cleanup verified, with a separate host Windows process-list check.
The full suite was not repeated for this naming-only refinement; runtime
confirmation of fixed filenames is pending. Historical numeric log files in
the game directory have not been removed.

Status: `SURGICAL LOG-OWNERSHIP REPAIR — OFFLINE VALIDATED; RUNTIME PENDING`.
The user authorized production repair after the initial analysis-only stage.

## Authorized repair batch — overlapping process log ownership

The user now authorizes surgical repair after examining the current diff.
Additional engine and loader evidence disproves the earlier same-lifetime
interpretation: process 20020 started at 18:09:26.996 and exited at 18:09:32.604;
process 25220 started at 18:09:29.364. Slop_yCam_Loader independently records
both PIDs. RTX MFG explicitly reports sharing violation error 32 in PID 25220
and falls back to its PID-named log. The engine creates a second log at
18:09:31 while the first closes. CameraTweaks' startup journal belongs only
to PID 20020. These are overlapping launches, not one worker freezing while
Gameplay continues in its own process.

NativeFile denies concurrent writers. Initialize aborts the entire camera
runtime if NativeLogSink cannot open the shared basename. This is the causal
failure path supported by the combined evidence. The exact second-process
error is only sent to OutputDebugString and was not captured; the regression
will exercise that same sharing conflict directly. This log-open fatal path
already exists in HEAD and must not be misrepresented as a newly introduced
FOV policy defect. More startup hooks/work in the diff can alter overlap
timing, but that timing contribution is not established.

Scope: add process-qualified fallback only to diagnostic log opening on a
sharing violation; preserve exclusive writes and all ordinary NativeFile
configuration semantics. Cover main/Overlay/startup logs. First reproduce
held canonical filename in the existing native log harness, verify original
content is not truncated, fallback succeeds, and non-sharing failures remain
fail-closed. Then focused validation, full suite, and build only on request.
Never install into the game directory. No FOV formulas or policies change.

## Current incident — cold PC start, 18:09:27 on 2026-11-16

This incident supersedes the assumption that replacing the ASI or accumulated
research processes is necessary to trigger the failure. The user reports the
failure immediately after a PC start and installing the mod. It does not prove
that all older failures have the same cause.

Evidence is preserved in `research/evidence/asi-startup-failure-2026-11-16-180927/`:
the three supplied logs, effective INI, and contemporaneous UE4SS log. No game
files were changed. No game process remained at inspection time, so thread
stacks, loaded-module identity and live hook bytes could not be inspected.

Confirmed observations:

- Runtime and installed ASI SHA-256:
  `491829BABFC0830498072A53ACAE5944147AB124BD8976750E26CA9FA1A716AE`.
  Game SHA recorded by Runtime:
  `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Startup journal PID is 20020, initialization thread 25260, Overlay owner
  thread 25200. Initialization reaches `CAMERA_CORE_READY` at 18:09:29.
- Gameplay/Cinematic/Dialogue hooks report installation; this is capability
  evidence, not callback execution or correction evidence.
- Overlay starts, creates its display window and arms WinEvent hooks. Eight
  logged scans find only Overlay plus a hidden 0x0 client window; eligible=0.
  Presenter attachment and startup toast are absent. This is a real failure
  to reach usable Overlay, despite successful camera initialization.
- Camera worker reports zero transactions through 18:09:31.434; the main
  log stops at 6,162 bytes. It has no worker-stop summary or shutdown record.
- UE4SS continues through 18:12:21, and its later telemetry records player
  movement. Thus the overall launch continued beyond our recorded startup.
  UE4SS timestamps alone do not prove identical PID; correlate explicitly in
  the next live observation.
- INI enables Overlay, Diagnostics, RuntimeCombine, GameplayHorPlus and
  Dialogue Reduced. Hotkey `Enabled=false` in the main log describes feature
  cycling, not the Overlay master setting or Delete availability.

Source findings and limits:

- `NativeLogSink` writes each record through `NativeFile::Write`/`WriteFile`;
  ordinary CRT buffering is not an explanation. The 16 MiB log cap has not
  been reached, and there is no `LOG_LIMIT_REACHED` marker. An I/O error can
  silently suppress later records; its occurrence here is unconfirmed.
- `CameraStateWriterLogWorker` emits a summary every second even when no
  writer callbacks occur. Its early silence therefore needs an explanation
  independent from merely not entering Gameplay.
- Overlay discovery has an eight-record diagnostic budget. Later absence of
  scan records does NOT establish that scanning stopped.
- Detached Overlay owner waits with `INFINITE` when input is not ready and
  there is no startup-data probe deadline. The timeout branch does not call
  `FindGameWindow`. A missed visibility/refresh event can leave it detached.
  This is an identified recovery gap, not a proven shared root cause.

## Bounded repair investigation and decision gates

1. Preserve this capture and compare it with the prior failed capture and a
   successful startup. Correlate process identity, first callback, presenter
   attach, worker progress and shutdown. Do not repeat the FOV audit.
2. First resolve why independent diagnostic progress disappears. On the next
   affected launch, inspect only that game process: PID/module path, worker
   existence and thread wait stacks, main/Overlay log handles, stop-event
   state, callback admission and actual installed instruction bytes. Use a
   bounded read-only snapshot; do not attach an unbounded trace or suspend the
   game for ongoing analysis. If the game is already closed, this evidence is
   unavailable, not a reason to guess a fix.
3. Distinguish branches: worker blocked/suspended, worker ended/stop requested,
   log I/O suppression, hook overwritten/bypassed, wrong process/module, or
   independent Overlay discovery failure. Check lock owners before changing
   waits; installation success cannot substitute for hook readback/execution.
4. If existing observation cannot distinguish these branches, propose a small
   INI-gated startup health record: explicit write failure status, stop reason,
   worker progress and first callback/first eligible window. Keep state
   observation independent of the same logger/lock being diagnosed. Obtain
   implementation scope before adding it; retain the single ordinary ASI.
5. Reproduce each confirmed causal path in an existing production-entry
   harness before repair. For Overlay, model hidden initial window, absent
   SHOW notification, later eligible window and recovery; verify bounded retry
   cost and unchanged shutdown/input ownership. For worker/hook defects, model
   the actual failing boundary rather than asserting that initialization PASS
   means operational PASS.
6. Apply only evidenced repairs, then focused regressions and the full suite.
   Build Release in the project and provide its link. Never replace ASI/INI
   in the game directory. Runtime acceptance requires toast/Delete and actual
   Gameplay correction in the same launch, plus normal exit/relaunch. A PC
   reboot is not the repair or routine cleanup step.

Expected implementation surfaces depend on the diagnosed branch:
`src/overlay/composition/composition_runtime.cpp`, existing Overlay lifecycle
harnesses; or `src/plugin/runtime.cpp`, `worker_lifecycle.*`, native callback/
hook owners and their existing harnesses. Log I/O handling belongs to
`src/platform/win32/native_log_file.hpp` only if an actual I/O failure is found.
Do not change all these areas speculatively. Risks include hook reinstallation
over another owner, hot-path logging cost and unnecessary discovery work.

Current result: startup capability is confirmed, operational failure reported
and Overlay non-readiness observed; the common root cause remains unknown.
The next discriminating evidence is the live worker/hook state at failure.

## Objective

Determine why, after installing an updated ordinary ASI and launching the game,
both FOV correction and the Overlay can become unavailable until the PC is
restarted. Establish whether the failures share one cause or are separate. Do
not treat a PC reboot as routine recovery.

The user confirms the game is closed before replacing the ASI. The plan must
not assume an in-process file replacement or ask the user to repeat that
already-answered sequence.

## Current evidence

The runtime logs from 2026-11-16 are preserved in
[`../evidence/asi-restart-issue-2026-11-16/`](../evidence/asi-restart-issue-2026-11-16/).

- The installed ASI and Release candidate had the same SHA-256:
  `05A303F48493DBA389C1F9EC2D9A6F9FC50CE62B311744B9C08E4F13558C6429`.
- The runtime log reports the same ASI hash, successful primary writer
  observer installation with `correctionEnabled=true`, and Gameplay,
  Cinematics, Dialogue, Hotkeys and Diagnostics as `AVAILABLE`. These lines
  confirm initialization of those components; they do not prove in-game FOV
  output or Overlay readiness.
- `[Overlay] Enabled=true`. The Overlay worker and its display window started,
  and the WinEvent hooks were armed. However, repeated discovery scans found
  no eligible game window; the sampled process window was rejected as
  invisible, and the final recorded refresh state was
  `no_eligible_game_window`. No `OVERLAY_PRESENTER_ATTACHED` or startup-hint
  creation is present. Thus this capture confirms the Overlay had not reached
  its usable presenter state.
- Camera diagnostics reported `transactions=0`. The contemporaneous UE4SS log
  reported waiting for a playable save/pawn (`Pawn unavailable`). Therefore
  this capture does not establish whether FOV correction worked after entering
  loaded Gameplay.
- Static source inspection shows Overlay discovery is normally requested by
  window events and owner wakeups. No explicit periodic rediscovery schedule
  for the detached/no-eligible-window state was found. A missed or absent
  visibility event is a candidate explanation for the Overlay result, not yet
  a proven cause of the reported reboot requirement.
- The available evidence does not explain why a PC restart restores both
  features, nor establish that the FOV failure shares the Overlay failure.

## Investigation sequence

1. Reconcile the preserved ASI, game-image, startup, runtime and Overlay
   identities with the exact launch that exhibited the failure. Inspect the
   full Overlay lifecycle from worker start through host-window selection,
   presenter initialization, startup toast, terminal disable and cleanup.
2. Trace the camera path independently: initialization commit, hook admission,
   first writer transaction, correction decision and failure/disable paths.
   Distinguish a hook that installed from a correction actually applied in a
   loaded save.
3. Review owned worker and resource lifetimes across normal game exit and the
   next launch. Check only project-owned processes/resources and preserve the
   user's other applications. Do not infer a leak from the fact that reboot
   helps.
4. Identify the smallest regression that reproduces each confirmed failure.
   For Overlay, cover initial discovery while the game window is hidden and
   its later transition to visible, including whether the owner receives a
   refresh and attaches the presenter. For camera correction, use a loaded-save
   transaction and assert both transaction presence and expected output.
5. Only after a concrete cause is established, implement the narrow repair and
   add the regression before changing production behavior. Keep Overlay and
   camera fixes separate unless evidence proves a shared owner or dependency.
6. Build the ordinary Release ASI and validate its identity. Any runtime
   confirmation uses that same ASI and the existing `[Diagnostics] Enabled`
   setting; do not create a separate diagnostic ASI.

## Runtime evidence to preserve later

If offline analysis requires another user-run session, request one bounded
launch with the ordinary ASI. Before any PC reboot, preserve:

- `STALKER2CameraTweaks.log`, `STALKER2CameraTweaksOverlay.log`, and
  `STALKER2CameraTweaksStartup.log`;
- UE4SS startup/load records covering the same process lifetime;
- the loaded ASI SHA-256, process ID and timestamps;
- whether the startup toast appeared, whether Delete opened the Overlay, and
  whether FOV correction changed a verified Gameplay writer output after a save
  was loaded.

Do not ask the user to run PowerShell setup, file-copy or INI-edit commands.
Only provide a PowerShell command if a later capture genuinely requires
launching a separate process, consistent with the workspace rule.

## Acceptance criteria

- Establish a concrete trigger and causal path for the Overlay failure, or
  state the remaining evidence gap precisely.
- Establish whether FOV correction is independently failing in loaded
  Gameplay, sharing the Overlay cause, or was only unverified in the capture.
- Add deterministic regression coverage for every confirmed reproducible
  production defect before applying its fix.
- Confirm that a normal game close and fresh launch recover the mod without a
  PC reboot, or document the remaining external/runtime dependency.
- Verify cleanup for every process launched for analysis or tests; do not
  terminate the game or unrelated processes as cleanup.

## Non-goals

- No code, configuration, mode, or release-artifact changes in this deferred
  planning phase.
- No separate diagnostic ASI, new FOV policy, or change to GameplayHorPlus.
- No speculative driver, registry, power-plan, service, or system-wide changes.
- No claim that the current capture proves a camera-correction failure.

## Repair validation

The earlier analysis-only non-goals and same-process hypotheses above are
historical; the authorized overlapping-process batch at the top governs the
current repair. The user reports all three correction modes stayed native.

`overlapping_startup_log_ownership` failed before the production change and
passed after it. `test.cmd -NonGraphics` completed with 62/62 invocations PASS,
0 FAIL, one explicit hardlink ownership SKIP (error=5), and WARP omitted.
The repair changes only the diagnostic file-opening helper and startup journal
integration, plus its regression. Game files are not altered. Runtime proof
requires initialization/camera transactions and usable Overlay in the actual
playable PID, including when its launch overlaps the earlier process.
