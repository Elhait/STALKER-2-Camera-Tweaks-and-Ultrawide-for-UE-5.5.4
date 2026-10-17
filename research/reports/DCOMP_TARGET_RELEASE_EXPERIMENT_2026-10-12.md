# DirectComposition target-release experiment

Date: 2026-10-12. Status: **Preserved experimental baseline; mixed live result; production adoption held.**
Offline validation passed; subsequent user testing reported improved HUD behavior,
a game crash, and invisible mod UI on the following launch. See the live review below.
Plan: [approved experiment](../plans/DCOMP_TARGET_RELEASE_EXPERIMENT_2026-10-12.md).
Context: [NVIDIA coexistence audit](DCOMP_NVIDIA_OVERLAY_COEXISTENCE_AUDIT_2026-10-12.md).

## Isolated change

The experimental ASI uses a working-tree source snapshot, including the current
production root-detach changes. Only its copy of renderer_runtime.cpp differs:

- After the accepted hidden SetRoot(nullptr) + Commit, release IDCompositionTarget.
- Keep the device, visual, surface, ImGui and input integration resident.
- On menu/toast demand, recreate the target with the existing TRUE/FALSE fallback,
  reuse the existing visual and attach/commit before publishing toast lifetimes.
- On failed reacquisition, use the existing optional Renderer disable boundary.
  No retry loop, new configuration or camera behavior changes.
- Event-only experiment logs record target creation layer/HRESULT and release.

Initialization, HWND rebind and graphics recovery still use existing startup
resource creation; their accepted hidden frame then releases the target. Startup
notifications legitimately retain the target until they expire. No claim is made
that a minimized but still logically open UI releases its target.

Canonical production/vendor inputs, installed ASI and release packages were not
changed. Existing unrelated working-tree edits were preserved. No game was launched.

## Validation

| Check | Result |
| --- | --- |
| Existing composition presenter transaction harness | PASS, exit 0 before isolated build; unchanged helper coverage, not a new target-lifetime test. |
| Snapshot candidate build | PASS, exit 0; all 61 registered translation units and embedded resources. Localization audit 18 locales PASS. Only the two existing vendor Zydis C4201 warnings. |
| Actual experimental Renderer / hidden HWND | PASS, exit 0; startup hint, three open/close cycles, idle, closed resize, visible/closed minimize/restore, toast expiry, visible/closed HWND rebind. |
| Actual target availability | A second device successfully creates TRUE target after idle release; creation fails while the Renderer owns it. Verified across three toggles. |
| Controlled layer contention | Foreign TRUE target forces successful FALSE fallback. Closing frees FALSE for a foreign target. With both foreign layers occupied, reopening returns false and disables only Renderer; another Render does not retry. Foreign target release then makes TRUE available. |
| Shutdown | Explicit Renderer shutdown after controlled terminal failure. Not proof of game-process teardown. |
| Source parity | Every canonical manifest input unchanged; exactly one snapshot build input differs: renderer_runtime.cpp. Camera/runtime/vendor/build registry/config inputs are byte-identical. |
| NVIDIA HUD / G-SYNC / actual presentation path | Not exercised by hidden-HWND fixture; pending user game test. |

The observed target-creation failure HRESULT was -2003302400. Successful creation
returned 0. These are controlled Windows API observations, not NVIDIA internals.
The fixture uses public Renderer paths, real DComp targets on a local hidden HWND,
controlled settings and a separate device to occupy layers; no production seam.

Full canonical suite was not rerun for a research-only snapshot. The prior
production full-suite result remains scoped to production, not this experimental
artifact. Device removal, driver fault injection and real Delete/NVIDIA shortcut
delivery were not exercised by the fixture.

## Artifact and evidence

Experimental file: `build-artifacts/tr/STALKER2CameraTweaks.asi`.
SHA-256: `5D2AFAB2DDCDC1C2B054122BC80020EC3A41FE6CA13243851E33B2458ADDB55B`.
Keep this exact basename when testing. No new INI is needed; retain the current
settings with Overlay.Enabled=true. Back up the currently installed ASI before
manually replacing it. Do not load production and experiment simultaneously.

Ignored evidence beside the artifact: source_manifest.json, build.log,
fixture-build-E.log and fixture-run-E.log. Reproduction tools:
prepare_overlay_target_release_experiment.ps1 -Build, then
run_overlay_attachment_fixture.ps1 -ExperimentDirectory build-artifacts/tr -TargetRelease.

## One user-run discriminator

Historical procedure used for the initial handoff. **Superseded:** following the
reported crash, invisible menu and toast-expiry stall, no further game experiments
are requested. The current phase is analysis of existing implementation/evidence.

Enable NVIDIA Performance HUD before launching the game. Wait for the mod's
startup toast to disappear. Observe HUD + G-SYNC Indicator with the menu closed,
then open/close the menu three times. Restart the HUD with its shortcut and repeat
open/close. Preserve settings and scene; report both NVIDIA components' visibility
for each state. Target release/create records belong to STALKER2CameraTweaksOverlay.log.

If behavior improves, review target lifetime as a compatibility candidate without
claiming NVIDIA uses the same layer. If unchanged, investigate presentation path
through a bounded external capture instead of broad Renderer changes. No permanent
production adoption is authorized by preparation or these offline PASS results.

## Live follow-up and crash review

User observation: NVIDIA Performance HUD stayed visible; G-SYNC Indicator
disappeared only with the mod menu open. A second launch crashed. On the following
launch the mod Overlay did not visibly appear. This is a positive HUD observation,
not an acceptable overall stability/coexistence result.

The supplied mod logs describe the **subsequent launch**, not the crashed session:
startup journal PID 32040, main log beginning 20:50:27 local time, experiment SHA
5D2AFAB2DDCDC1C2B054122BC80020EC3A41FE6CA13243851E33B2458ADDB55B.
The crash occurred earlier at 20:50:05, PID 17080. Do not use the newer Overlay
log's successful transitions as proof of the crash session's ordering.

### Crash evidence

- CDB inspected the UE4SS minidump's stored exception, registers, exception stack
  and loaded-module information. The stored exception code is 0x00004000,
  raised through KERNELBASE!RaiseException; it is not an observed access violation.
- The dump contains the UTF-16 fatal message at 0x00000088979fd920:
  `Streamline/DLSSG present failed`, StreamlineD3D12RHI.cpp:555,
  `DXGI_ERROR_INVALID_CALL`. The game log
  `Stalker2-backup-2026.10.12-17.50.05.log` records the same error at UTC 17:50:05.303
  (local 20:50:05.303).
- Exception stack reaches game D3D12/Streamline code through NVIDIA's
  `190_E658703.dll`, module metadata identifying SL.DLSS_G 2.14.0.0 from
  `C:/ProgramData/NVIDIA/NGX/models/sl_dlss_g_0/versions/134656/files/`.
  RenoDX's crash record independently matches PID/time, code and stack.
- No Camera Tweaks frame appears in that exception stack. This does not exclude
  an indirect effect of its composition changes. The immediate fatal failure is
  established; the trigger and target-release causality remain unverified.
- The dump also contains NVIDIA capture, RTSS, ReShade, RenoDX, a local DXGI
  wrapper and other modules. Module presence alone is not causal evidence or
  proof that every loaded FG provider was actively selected.

### Subsequent invisible-UI evidence

Startup records CAMERA_CORE_READY, RENDERER_ACTIVATED, OVERLAY_FIRST_FRAME and
INPUT_ACTIVATED. Overlay records startup-hint draw/commit, target release, two
VISIBLE/HIDDEN cycles, successful TRUE target reacquisition (hr=0), root attach
and release. No `experiment_target_reacquire_failed` or terminal Renderer disable
appears. Thus these logs do **not** support failed initialization or occupied-layer
reopen failure. They establish application/API acceptance, not pixels visible on
the user's display. Actual external composition/visibility failure remains open;
no persisted disable or cross-process retained target is established.

### Disposition

Do not promote target release or continue ordinary use of this experimental ASI
until the crash/visibility issue is resolved. Recommend manually returning to the
previous root-detach build (`build-artifacts/detach/STALKER2CameraTweaks.asi`,
BF5B1E9460D5A75AF32D0CB09A86CCDE6FA86A0B1EE04BBF569CDA373391ABCD) to isolate
target lifetime. No installed file was replaced during analysis. Do not change
camera algorithms or blame NVIDIA/another wrapper solely from the fatal stack.

Raw logs, game fatal log and dump were preserved in ignored
`build-artifacts/tr/crash-review/live-20261012/`; CDB transcripts are dump.txt and
threads.txt in crash-review. Dump SHA-256:
990E5DA6BBD2E8952A9F16F6E06ED52974ABAE7AD4A3D06BE502A9ECC17784CE.
No source edits, rebuilds or game launches were performed for this diagnosis.

## Toast-expiry lifecycle analysis — subsequent 21:21 session

The user rebooted and reported a repeatable several-second game stall when a
visible welcome toast disappears. NVIDIA HUD and G-SYNC Indicator disappear during
the stall and return afterward. When the game window is inactive and the user does
not see the toast, the stall is absent. This is user-observed timing, not an API
duration measurement. It is sufficient to stop gameplay experiments and analyze
the existing transition; no repeat test is requested.

Latest logs: PID 17152, experiment SHA 5D2A...DB55B, Diagnostics=false, main log
start 21:21:36.266 local time. Overlay owner thread ID is 16008. The Overlay log
shows startup draw/commit, root attached=0, `released=1`, then geometry update;
no target-reacquisition error, terminal disable or graphics recovery is recorded.
There are no menu-open/recreate records in this session's captured log. Prior
invisible-menu and crash evidence belongs to earlier sessions.

### 1. Execution thread and blocking propagation

Actual final-toast path:

```text
Overlay owner: FrameClockMessage (or fallback timer) + g_rendererMutex
  Render -> UpdateNotifications removes expired toast, marks dirty
  RenderImpl -> final damage clear -> EndDraw -> Commit [clear batch]
  StartPendingNotificationLifetimes -> ReconcileCompositionAttachment
    SetRoot(nullptr) -> Commit [detach batch] -> publish attached=false
    compositionTarget_.Reset() [COM Release] -> log released=1
  UpdateRenderTimer turns off animation demand -> unlock

Game HWND message thread:
  OverlayWindowProc -> same g_rendererMutex -> processing/pass-through
  original window procedure -> same mutex again for FinishWindowMessage
```

Anchors: canonical `src/overlay/composition/composition_runtime.cpp:578-604`
and `:839-873`; experimental snapshot `renderer_runtime.cpp:1311`, `:1405-1446`,
`:1562-1569`, `:1671-1693`, `:1204`.

**Confirmed coupling:** target release executes synchronously as a COM call on
the Overlay owner while holding a mutex also acquired by the game HWND procedure,
even for hidden/pass-through messages. Therefore a long BeginDraw/EndDraw/Commit/
Release call can block that message thread. It is not directly executed by the
game's Present callback. This is a concrete possible propagation path, not proof
that Release actually took seconds or that the message thread caused the observed
zero-frame interval. A recursive mutex only permits same-thread recursion; it
does not prevent cross-thread waiting.

No explicit sleep, GPU fence wait, WaitForCommitCompletion or game-Present wait
exists in the normal toast-end path. D3D11 context Flush occurs in graphics
shutdown/recovery, not ordinary target release. No normal-path retry loop or
per-expiry full-device destruction was found. Synchronous API invocation does
not imply the composition commands are synchronously executed by DWM.

### 2. Submission ordering versus completion

Source order is valid at the submission level: draw/clear EndDraw and Commit
precede SetRoot(nullptr) and its Commit; attachment is published only after the
second Commit succeeds; target Release follows. The device, surface and visual
are retained. The owner submits two separate batches, not one atomic clear+
detach+target-destruction transaction with NVIDIA's presentation.

DirectComposition processes command batches asynchronously. Commit acceptance
does not establish completion; WaitForCommitCompletion is the separate processing
wait. Neither establishes that NVIDIA HUD or a generated game frame reached the
display. Sources: [DComp architecture](https://learn.microsoft.com/en-us/windows/win32/directcomp/architecture-and-components),
[Commit](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-commit),
[WaitForCommitCompletion](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-waitforcommitcompletion).

**No documented requirement to wait before releasing this target was established.**
Do not diagnose the absence of a wait as a proven lifetime violation, or add a
blocking completion/GPU wait under the shared mutex as a speculative fix. Local
fixtures proved layer availability and accepted transitions, not completion
latency or display integration with Streamline.

### 3. Race analysis

No application-side concurrent target destruction/recreation was found in the
examined paths. DComp mutations, notifications, rendering and geometry/rebind
are serialized by the owner/shared mutex. Delete changes desired visibility
under that same mutex and posts a redraw; it cannot race another thread's release
in the middle of the protected transaction. Incoming runtime notifications are
queued separately and consumed by the owner on a following frame. The clock
worker only posts coalesced tick messages; it does not own the target.

An external composition/presentation transition can still overlap asynchronous
DWM processing, native DLSSG Present and third-party overlay updates. No shared
transaction spans those systems. That is an unverified inter-system interaction,
not evidence of a C++ data race. Latest toast session has only release, no
reacquisition, so a release/recreate race is not necessary to explain that stall.

### 4. Successful API calls but invisible menu

The previous session's accepted target creation and Commit exclude the recorded
fail-closed reacquisition path, not external visibility failure. `ready()` reflects
the local presenter phase/surface generation. `visualAttached_` reflects accepted
API submission. Neither reads actual DWM/display visibility.

There is also an observability limitation: RenderImpl emits activation trace
stage `FirstVisibleFrame` at snapshot line 2436 **before** its caller reconciles
the newly recreated target (line 1568). Even when emitted, it is not proof of
successful later attachment or display. Diagnostics were OFF in these supplied
sessions; this marker does not explain the invisible menu or stall. The clean
event-driven scheduler does not independently verify/retry a visually missing
but accepted frame. Do not introduce continuous drawing as a workaround.

### Findings and disposition

| Finding | Status | Consequence |
| --- | --- | --- |
| Shared mutex carries possible owner API latency into game HWND processing | Confirmed structure; actual measured blocking unknown | Treat as a critical integration boundary for any subsequent design; do not simply move COM resources across threads or remove locking. |
| Target Release occurs directly after accepted detach Commit | Confirmed; completion latency/driver behavior unknown | Immediate release is not yet a proven invalid API sequence; synchronous waits are not justified as a fix. |
| Local target destroy/recreate race | Not found in examined serialized paths | Do not add duplicate ownership, atomics, queues or another mutex to fix a hypothetical local race. |
| NVIDIA elements temporarily disappear and recover at toast expiry | User-observed; OS/driver mechanism unknown | Consistent with presentation/composition reconfiguration, but not proof of actual VRR disable or target-release causality. |
| Previous Streamline/DLSSG invalid Present and invisible menu | Confirmed fatal / user-reported visibility symptom; same cause not established | Retain as independent unresolved evidence; do not combine into a single proven root cause. |

Production stays at root detach. Experimental target release is not approved for
promotion; no additional gameplay runs are requested. A future proposed repair
must distinguish shared-lock blocking from external presentation reconfiguration
and preserve one graphics owner, Delete and camera independence. Existing logs
lack per-call durations or a hang-thread snapshot, so this audit cannot honestly
name the single slow API. That limit does not justify repeating the user's game
scenario. No code, instrumentation, builds or tests were changed/run for this
analysis; native safety gates retain their existing status.

Latest three logs were preserved under ignored
`build-artifacts/tr/crash-review/toast-20261012-212136/`. Overlay log SHA-256:
21379B6B95F3AE115718F19CAF8D51C1059139909E038CE709223AFC4BB1AE68.
All canonical build-input hashes still match the snapshot baseline.

## Instrumental follow-up — no synchronization change

Following user approval, an isolated timing artifact was prepared under
`build-artifacts/trt/`. Canonical production remains root-detach. The artifact
retains the target-release experiment's ordering and lock scope; it is **not a
fix**, is not installed, and is not being requested for another game run now.

Source: research/prototypes/overlay_attachment/target_timing.hpp and the checked
snapshot patch add_overlay_target_timing_snapshot.ps1. Preparation supports
`-OutputDirectory build-artifacts/trt -Timing -Build`; fixture runner supports
`-ExperimentDirectory build-artifacts/trt -TargetRelease -Timing`.

### Captured intervals and delivery

- QPC pairs wrap every SetRoot, Commit and target ComPtr Reset in the snapshot.
  Surface Commit and attachment Commit have separate names; remaining commits
  retain the generic name. Duration includes the native call, not subsequent
  lifecycle log formatting or flush. HRESULT/void return and exceptions preserve
  the original behavior.
- Both window-procedure acquisitions have named wait intervals, ending after
  lock acquisition. No attempt is made to unlock earlier or change recursion.
- Only intervals >=2 ms publish a sample. Recording uses 64 fixed slots, bounded
  scans with lock-free atomic slot claims, QPC/thread IDs; no strings, allocations,
  logger calls or additional mutex are used by sample capture. Overflow drops
  samples with a counter instead of waiting or growing storage.
- Overlay owner drains outside renderer-lock scopes. New diagnostic formatting,
  logger mutex and disk writes therefore occur outside g_rendererMutex. A slow
  window sample posts a coalesced diagnostic-only thread message for idle delivery;
  it does not request a renderer frame. Records include begin/end QPC, frequency,
  thread and duration_us. Slots are not guaranteed chronological; compare QPC
  intervals, not log line order. Records emitted after a stall are retrospective.
- This research-only capture is active regardless of Diagnostics.Enabled; no new
  INI key or production diagnostic gate is introduced. Existing lifecycle logger
  still performs synchronous flush and is unchanged. Its cost, BeginDraw/EndDraw,
  target creation and external DWM/driver work are not individually timed here;
  absence of these named slow samples would not prove absence of other blocking.

### Offline results

| Check | Result |
| --- | --- |
| Isolated build, final rebuild | Exit 0, 61 production inputs; 18-locale audit PASS; final changed TUs compile without new warnings (initial full build only existing vendor C4201). |
| Timing thresholds and fixed buffer | PASS: subthreshold suppressed, inclusive 2 ms captured, exact fields, 64 retained / 6 dropped for 70 synthetic samples. |
| Wrapper behavior / sink failure | PASS: E_FAIL unchanged, callback exception propagated normally, empty buffer has no log, throwing diagnostic sink contained. |
| Controlled real mutex contention | PASS: a local holder thread holds the original recursive mutex for 40 ms; timed acquisition records a wait. The sleep is fixture-only, not in ASI code. |
| Actual experimental Renderer HWND fixture | Exit 0: startup, repeated toggle, idle, notification expiry, resize, minimize/restore, rebind, layer availability, FALSE fallback and both-layer fail-closed behavior unchanged. |
| Example native timing from local fixture | Final fixture recorded CommitSurface=2110 us, tid=5732. This is a hidden-HWND local measurement, not the user's game/DLSS stall. No inference of the game culprit follows. |
| Canonical parity | Only snapshot Renderer/composition cpp files differ from tracked baseline, plus copied timing header. All canonical manifest build inputs unchanged. |

Artifact `build-artifacts/trt/STALKER2CameraTweaks.asi`, SHA-256:
011EF5A64F7B3AC2D66FAD0792DEF261E41138A02BF23C7CF9BC0DE8EC33D8FB.
Build/fixture logs and input metadata reside beside it. No full suite/game run
or release/installation changes were performed for this research artifact.

### Can the critical section safely be shortened now?

**Not by simply unlocking around native calls.** The HWND procedure reads local
presenter readiness/disabled state and calls SetVisible, which writes non-atomic
surfaceDirty_. The owner concurrently updates presenter state, dirty state,
notifications and resources under the shared mutex. Additionally,
DisableAfterOverlayException can be invoked from the HWND procedure's exception
boundary and calls Renderer::Disable/Shutdown on that calling thread under the
same mutex. This qualifies the earlier owner-only statement: normal target
transactions are owner-thread operations; exceptional cleanup can originate from
the window thread, but the shared lock still excludes concurrent mutation.

Removing the owner's lock would permit races in those shared fields and could
allow exception cleanup to reset graphics resources during a draw. A viable
future design would first bound what window/input processing can read/mutate,
hand requests/terminal graphics cleanup to the existing owner, and then separate
short shared-state locking from owner-only graphics execution. That is a reviewed
ownership/synchronization change, not an approved instrumentation edit. No such
redesign or new Runtime owner is introduced here.

Existing Log also locks g_logMutex and flushes the file from lifecycle paths,
including some while g_rendererMutex is held. This is another possible critical
section cost; it is not measured by the DComp wrappers and not established as
the freeze cause. Do not misattribute all future mutex waiting to target Release.

Conclusion: measurement capability is prepared and locally verified. The actual
game's slow API and causal overlap with window-thread waiting remain unmeasured.
No old log can supply missing per-call durations retroactively. Production detach
and the decision not to adopt target release remain unchanged.

## Instrumented game log — 21:50:30 session

User supplied three logs without an additional observation of whether the
multi-second stall occurred in this particular run. Artifact identity matches
011EF5A64F7B3AC2D66FAD0792DEF261E41138A02BF23C7CF9BC0DE8EC33D8FB;
PID 29596, Diagnostics=true, Overlay owner tid=14544, HWND-procedure tid=20120.

| Recorded operation | Count | Durations |
| --- | --- | --- |
| CommitSurface | 5 | 3.477, 11.620, 2.386, 18.094, 4.641 ms |
| WindowMutexBefore | 1 | 10.026 ms |
| SetRoot, CommitAttachment, ReleaseTarget, WindowMutexAfter | 0 slow samples | No >=2 ms record; do not read this as zero duration. |
| Timing buffer overflow | 0 reported | No dropped-count marker in supplied log. |

The window mutex wait spans QPC 28039186699-28039286964. **It does not overlap
any recorded slow DComp call.** The following 18.094 ms CommitSurface starts at
28039724268, 43.7304 ms after that wait ends. Thus the largest Commit must not
be presented as the cause of the recorded mutex wait. Waiting on the shared lock
is now observed, but the work that held it during that interval is not identified.
Unmeasured rendering/BeginDraw/EndDraw, state work and existing log flush remain
possible local costs; none is established by these records.

Startup toast draw, root detach and target release are recorded, followed later
by one accepted open/close cycle with successful TRUE target recreation. No
multi-second sampled API/mutex interval, terminal disable, target acquisition
failure or graphics recovery is present. This session does not establish the
previous crash's cause or prove the previously reported visual symptoms absent.

**Interpretation:** the proposed synchronous SetRoot/Commit/Release -> multi-second
window mutex stall mechanism is not demonstrated in this run. Sub-2 ms submission
or Release can still trigger an asynchronous OS/driver presentation change; its
downstream cost is outside these timers. Do not discard that hypothesis, blame
the 18 ms Commit, remove synchronization or add a blocking wait from these data.
Production adoption remains held and no further user test is requested here.

Evidence preserved under ignored `build-artifacts/trt/live-20261012-215030/`.
Overlay log SHA-256: BF812C0184CD738899BE4A9587B4A1BB65DE95E5FABF77BD56E2B93007B27382.
Analysis only: no source edits, build, game launch or installation.

## PresentMon correlation review — PID 30044

Reviewed the supplied CSV and three fresh logs, plus the pasted prior analysis.
The prior analysis is context, not verified evidence. Independent CSV import with
the spreadsheet analysis tool and QPC conversion expose an important correction.

CSV contains 4251 records: **4250 DXGI records** for swapchain 0x1D28E724130 and
one separate `Other` / 0x0 record. Do not count the latter as another game DXGI
frame or combine its timing with that stream. PID 30044 matches the startup journal;
ASI identity remains the instrumented 011E...D8FB, Diagnostics=true.

### Time basis and limits

Journal anchor: QPC 36189558477, FILETIME UTC 134363054832745466,
frequency 10000000. Convert QPC deltas to 100 ns FILETIME units before applying
Europe/Kiev's UTC+3 offset for these dates. Conversion independently checked using
JavaScript BigInt and Windows FromFileTimeUtc.

CSV CPUStartDateTime is labelled 2026-10-13 01:05, while matching mod text/journal
events are 2026-10-12 22:05 local (19:05 UTC). This is a three-hour discrepancy
relative to mod local time, not merely the ordinary UTC/local conversion. Its
origin is not established. Working alignment subtracts three hours from CSV
labels. It is supported by both menu toggles: owner-independent toggle QPC maps
to local 22:06:39.3181443 and 22:06:42.9690320; CSV mode changes occur at labelled
01:06:39.3242822 and 01:06:42.9898396. Do not use those nearby mode changes as
independent exact timestamp anchors or assume zero residual clock error.

The CSV has no raw CPUStartQPC column and root release has no individual QPC
marker. Therefore exact release/Present causal ordering cannot be reconstructed
to millisecond precision. CPUStartDateTime denotes the CPU start of the recorded
frame, not the instantaneous time the OS changes composition or a Present call.
[PresentMon column definitions](https://github.com/GameTechDev/PresentMon/blob/main/README-ConsoleApplication.md).

### Correction: 16.32 is toast startup, not release

The 16.921 ms CommitSurface spans QPC 36520022308-36520191525, mapping to local
22:05:16.3209297-22:05:16.3378514. Auto locale synchronized at 22:05:16.264;
the Overlay log places startup notification lifetime publication in this owner
iteration before its deferred timing record. This is the first committed toast
frame, **not** its final clear.

Renderer starts notification lifetime after committed drawing and attachment;
StartupHintDurationMs is 10000. Subsequent animation frames normally emit no slow
sample and no lifecycle message. The adjacent `attached=0 / released=1` lines
therefore cannot be assigned the timestamp of that preceding slow Commit. The
ten-second animation can lie entirely between those log lines.

Normal-path expiry is expected around local 22:05:26.3 (CSV working label
01:05:26.3), subject to GetTickCount64 granularity, scheduling/logging delay and
the unresolved CSV clock offset. This is an estimate, not a measured release
time. The prior claim of release at 01:05:16.32 inside the first long interval,
and its resulting approximately 13-second distance to the largest interval,
are **withdrawn as unsupported**.

### Actual recorded intervals and transitions

Times below retain CSV labels; no claim that the labels are Kyiv wall time.

| CSV frame CPU start / interval | DXGI evidence | Interpretation |
| --- | --- | --- |
| 01:05:12.0967534 -> 01:05:17.4861303 | FrameTime 5389.3769 ms, Composed Flip | First long CPU-frame interval overlaps toast startup under the working alignment, not demonstrated toast expiry/release. |
| Around 01:05:26.2902279 | Composed -> Hardware Independent Flip; that frame is 16.5873 ms | Near expected toast expiry. No multi-second frame interval at this transition. Exact release time unknown. |
| 01:05:29.3237837 -> 01:05:37.7685352 | FrameTime 8444.7515 ms; associated frame Composed Flip; preceding displayed frame lasts 8487.8213 ms | Genuine long frame/display interval. Begins roughly three seconds after normal-path estimated expiry, not thirteen. Neither target causality nor which event came first is established by frame-mode attribution. |
| 01:05:37.7725442 | Return to Hardware Independent Flip, frame 25.4377 ms | Presentation resumes in the recorded stream. |
| 01:05:54.8753954 -> about 01:05:57.0698858 | FrameTime 2194.4904 ms, Hardware Independent Flip | Long intervals also occur without Composed mode. |
| 01:06:39.3242822 / 01:06:42.9898396 | Composed entry/exit near menu toggle QPC; frames 15.4206 / 16.2555 ms | Supports attachment/presentation association, without a comparable long stall on these toggles. |

The largest frame additionally reports CPUBusy=8439.6959 ms, CPUWait=5.0556 ms,
GPUWait=8432.6484 ms and GPUBusy=8.5803 ms. These frame-level classifications are
not sampled CPU stacks or proof of a particular GPU fence/driver wait. In
particular, this is not evidence that the CPU or GPU computed for eight seconds.
FrameType is absent, so the CSV cannot establish complete generated-frame cadence.
The two recorded CommitSurface calls are only 15.809 / 16.921 ms; no slow SetRoot,
attachment Commit, ReleaseTarget, window mutex wait or dropped-buffer marker appears.

### Conclusion

Presentation-mode changes and long frame/display intervals are established for
this stream. The claim that target release occurred during the first 5.389-second
interval is not. Estimated toast expiry instead lies near a short-frame transition
to Independent Flip; the 8.445-second interval occurs later. This weakens a simple
immediate release -> seconds-long stall account without excluding an asynchronous
effect. Mode change alone is neither a demonstrated necessary nor sufficient cause.

No new game recording is requested. Do not redesign synchronization, insert waits,
or implement deferred release at an arbitrarily named "safe point" from these
data. Production root detach and target-release adoption hold remain unchanged.
The spreadsheet skill was used for read-only CSV import and independent timing
checks; the input CSV was not edited or re-exported.

Evidence preserved under ignored `build-artifacts/trt/live-20261012-220443/`.
CSV SHA-256: DEF03DB8C77DA176028A6DECF0E360141557DC5053A53D7CFC7301BB4ABA0465.

## Static regression audit — latest user disposition

The subsequent artifact-history decision below supersedes the audit's rollback
proposal as the next action. Keep this audit and its rationale as historical
analysis; do not execute a rollback or mark the whole target-release direction
REJECTED from it.

The user identifies the startup freezes as a regression introduced by the mod's
latest changes. Treat that as user-reported regression attribution for prioritizing
the source review, not a reason to keep investigating ordinary game startup or
request another reproduction. The precise native failure mechanism remains a
separate question. The earlier incorrect release/first-gap timestamp correlation
stays withdrawn; this disposition does not reinstate it.

### Compared subjects and source integrity

| Variant | ASI SHA-256 prefix | Actual source delta |
| --- | --- | --- |
| Root-detach control | BF5B...ABCD | Current canonical Renderer retains target, detaches root. This is the chosen control, not an independently established universal no-stall baseline. |
| Target release | 5D2A...B55B | Exactly one manifest build input changes: Renderer cpp, in CreateCompositionTarget and ReconcileCompositionAttachment. |
| Instrumented target release | 011E...D8FB | Same lifecycle delta; timing wrappers, two window-lock measurement wrappers, owner initialization/drain and copied diagnostic header. |

Checked current working-tree inputs against the target-release preparation's
baseline hashes: no canonical build input changed since that snapshot. Renderer
header and composition worker are byte-identical between root control and the
uninstrumented experiment. Runtime, camera algorithms, INI/parser, vendor,
resources, compiler registry and callback registration have no experimental delta.
Reviewed complete no-index diffs, not only the preparation script's intended edits.
No builds, tests, source changes, installation or game launches in this audit.

### Exact behavioral regression candidates

**RG-01 — HWND target destruction on ordinary toast/UI expiry.**
At experimental renderer_runtime.cpp:1442-1445, ordinary settled-hide now calls
compositionTarget_.Reset after accepted detach. Previously target lifetime was
resident until existing reset/rebind/recovery/terminal cleanup. This is the only
new graphics lifetime intervention on the expiry path. It frees the actual
HWND-layer binding, not merely visual pixels; a fast COM call does not rule out
subsequent OS/driver effects. **Primary regression candidate**, not a proven
internal DWM/Streamline mechanism. The reported original target-release freeze
and crash predate the timing wrappers, so those wrappers are not necessary to
explain the originally reported regression.

**RG-02 — repeated target acquisition with retained visual/surface.**
At :1409-1420, a visible menu/toast now reacquires target before SetRoot/Commit.
CreateCompositionTarget (:848-865) was changed to reuse the existing visual
instead of always creating one. Local surface generation/readiness/damage cache
remain intact while the external HWND target identity changes. API-success does
not validate the new target's displayed output. This is the principal new reopen
boundary relevant to the invisible-menu report. It is not a demonstrated invalid
visual/device association: the visual, surface and new target share the retained
device, and the old root was detached. Do not invent a required new surface, GPU
flush or full redraw as a fix without a concrete violation.

Layer collision and TRUE-to-FALSE fallback are possible after releasing the
binding, and local fixtures exercise them. However supplied invisible-menu logs
show TRUE creation hr=0 with no fallback/reacquisition failure. **Fallback is not
the explanation supported for that observed session.** Delete's handler, original
HWND association checks and rebind/reset path are unchanged.

**RG-03 — additional lifecycle log writes.**
Target create/release adds bounded event logs. The existing logger flushes
synchronously; these calls add local cost, including while the renderer mutex is
held, but no per-frame logging loop was added. No measured multi-second flush
cost is available. Secondary candidate only; not evidence of a new log flood.

### Changed versus unchanged ordering

```text
Both variants:
  expired notification removal -> final damage clear -> EndDraw -> Commit
  -> SetRoot(nullptr) -> Commit -> attached=false

Only target-release variant:
  -> target Reset / HWND-layer binding release

Next opening, both variants:
  draw prepared surface -> reconcile root -> SetRoot(visual) -> Commit

Only target-release variant, inside reconciliation:
  missing target -> CreateTargetForHwnd -> reuse visual -> attach
```

Startup duration, committed-lifetime start, notification queue, clear shader path,
surface resource publication, ordinary resize/minimize/rebind, mutex acquisition
order and owner frame scheduler were not rewritten. Native Commit counts/order
on normal expiry are the same; the new operation is target destruction. On reopen
there is an added target creation, not a new draw/Commit algorithm or retry loop.

No additional direct game Present mutation, stale cached HWND use, concurrent
release/recreate, invalid COM device affinity, premature surface/device freeing
or missing local fail-closed check was established in this bounded diff review.
That finding must not be misrepresented as proof of graphics compatibility.
The established shared-lock/exception-cleanup constraints remain; changing them
would not remove the isolated target-lifetime intervention.

### Minimal remediation proposal, not implementation

The bounded rollback target is **the two experimental lifecycle functions**:
remove ordinary-hide target Reset/reacquisition and restore root-only attachment
management (including the original target/visual creation behavior). Keep the
accepted production root detach, all camera changes and existing synchronization.
An already-built root-detach artifact is available; no automatic rollback or
replacement was performed. This removes the identified regression delta but is
not claimed here as a measured cure for all stalls or full NVIDIA coexistence.

Do not replace this rollback proposal with delayed release at an invented safe
point, waits, retry, a new mutex or Renderer redesign. Target release remains
withheld from production; no further user game test is requested. Reported mod
regression is retained as such, with RG-01/RG-02 as the concrete changed integration
boundaries and the internal failure mechanism unresolved.

## Artifact history reconciliation — latest decision

Scope: reconcile chat chronology, plans/reports, saved source manifests and actual
ASI hashes. No changes to production, experiments, installation or binaries.
User decision: preserve progress and both research baselines; no broad rollback,
deletion or blanket REJECTED status. Production remains root detach. Target-release
promotion remains held, not rejected as an entire compatibility direction.

### Original A/B/C labels versus later builds

Original A/B/C names belonged to the first attachment experiment. **Original C
was root detach, not target release.** Do not reuse C/D to describe later target
release/timing artifacts: that hides a real source distinction.

| Artifact / verified identity | Mechanism | Chronology and observed result |
| --- | --- | --- |
| Original A/B: EB29B394810F55E23BD6C73DF060379A7B1281D13F895D36AEE0B70FBB321CE6 | A disables Overlay through INI; B retains attached hidden root | A at 15:16: indicator remains. B at 15:27: indicator disappears. Same binary, different Overlay.Enabled. Historical A bundle file is currently absent; matching B binary and original metadata remain. |
| Original C-root: 78357B990DC705DD3973339E0EA4FE733E2484A2DCB1461F7C33F859BC2B624E | SetRoot(nullptr), target retained; initial probe counters | Log at 15:46:57 confirms identity. User: indicator disappears on menu open, returns on close. Performance HUD was not part of this first reported test. Binary/source/logs remain preserved. |
| Production root: BF5B1E9460D5A75AF32D0CB09A86CCDE6FA86A0B1EE04BBF569CDA373391ABCD | Adopts root detach with production transaction/error propagation; no initial prototype counters | Built after original C success. Follow-up user reports root-detach HUD/indicator interaction and activation-order effects; that follow-up message lacks an independent ASI identity record. Do not silently transfer every original C observation to BF5B. Binary remains preserved. |
| E-TR: 5D2AFAB2DDCDC1C2B054122BC80020EC3A41FE6CA13243851E33B2458ADDB55B | Root detach plus target Reset/reacquisition, retained visual, event logs; no QPC timing wrappers | User's first E-TR feedback already includes improved HUD behavior, a second-launch crash and next-launch invisible menu. Dump timestamp 20:50:05 precedes archived next-launch log at 20:50:27 identifying 5D2A. After reboot the user reports welcome-toast-expiry stall; archived 21:21:36 log still identifies 5D2A, Diagnostics=false. |
| E-Timed: 011EF5A64F7B3AC2D66FAD0792DEF261E41138A02BF23C7CF9BC0DE8EC33D8FB | E-TR lifecycle plus QPC call/mutex recording and deferred drain | Prepared only after the E-TR crash/invisible-menu/stall reports. Archived 21:50:30 and 22:04:43 sessions identify 011E; latter matches PresentMon PID 30044. No additional target-lifecycle fix appears between E-TR and E-Timed. |

### What actually changed between the relevant builds

Root -> E-TR: exactly the two Renderer functions reviewed above, including target
release/reacquisition and visual reuse. E-TR -> E-Timed: timing wrappers/header,
window-mutex timing RAII using the same recursive mutex, initialization and
out-of-lock diagnostic drain. No new clear, toast duration, HWND discovery,
camera semantics, target release policy, delay or retry was added in that step.

Rechecked E-TR Renderer hash against its preparation manifest and all E-Timed
patched-input hashes against its final manifest: match. All four key ASIs
(C-root, production root, E-TR and E-Timed) still exist and hash to the recorded
identities. No unrecorded intermediate lifecycle build was identified in these
saved artifacts/reports between initial E-TR HUD success and the 5D2A stall report.

The crash session's exact in-memory ASI hash is not established by the later
20:50:27 log; the user's first E-TR report and the archived original E-TR stall
session independently put problems **before timing instrumentation**. Therefore
returning to uninstrumented E-TR preserves a useful research artifact, but cannot
be described as a demonstrated regression-free fix.

### Preserved disposition

Keep original C-root, production root, E-TR and E-Timed with their distinct source
and evidence. Preserve the positive HUD result in E-TR alongside its instability
reports. Do not reject all attachment work, promote E-TR, discard an artifact,
or initiate another user game test from this reconciliation. The earlier rollback
was a proposal, never an executed change, and is not the current requested action.
