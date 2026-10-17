# Overlay-Owned HWND — Black Background Follow-up

**Date:** 2026-10-13  
**Scope:** Investigate the first G1-E live visual failure and prepare one
isolated candidate.  
**Game launch / ASI installation:** Not performed by this follow-up.

## User-observed result

With the Overlay shown during initialization and while open, the user observed
an opaque black background covering the game. Treat this as a live visual
failure. The G1-E offline results remain valid only for their tested contracts;
they do not establish transparent rendering in the game.

## Evidence from supplied logs

The provided Overlay log records:

- `OVERLAY_DISPLAY_WINDOW_CREATED owner_thread=1 unowned=1`
- successful DComp device, target/surface initialization and surface resize;
- `OVERLAY_STARTUP_HINT_DRAWN_AND_COMMITTED`;
- successful render submission and `first_visible_frame` on both menu openings;
- root attachment transitions and ordinary visibility changes, without a
  logged DComp/API failure.

Startup journal entries confirm renderer activation and an initial committed
surface. These are API/lifecycle observations, not evidence of the resulting
desktop pixels or alpha values. The logs therefore neither prove nor disprove
the user's black-background observation.

## Source finding and bounded hypothesis

The unowned display HWND is created with `WS_EX_LAYERED | WS_EX_TRANSPARENT`
and receives `SetLayeredWindowAttributes(..., 255, LWA_ALPHA)`. Its actual
image is provided by a DirectComposition visual whose surface format is
`DXGI_FORMAT_B8G8R8A8_UNORM` with `DXGI_ALPHA_MODE_PREMULTIPLIED`; drawing clears
the surface with zero RGBA before issuing ImGui draw calls.

The HWND is nevertheless created without `WS_EX_NOREDIRECTIONBITMAP`, despite
using DirectComposition rather than a conventional HWND redirection surface
for its content. A black opaque HWND backing/redirection surface is therefore
a plausible explanation for transparent pixels revealing black instead of
the game. This is a **hypothesis**, not a confirmed cause: DirectComposition
surface creation, premultiplied alpha and successful commits do not reveal
what DWM actually presented.

The relevant Microsoft contracts are documented at:

- [Extended window styles — `WS_EX_NOREDIRECTIONBITMAP`](https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles)
- [SetLayeredWindowAttributes — alpha 255 is opaque](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setlayeredwindowattributes)
- [DirectComposition CreateSurface — premultiplied alpha](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-createsurface)

## Candidate change

Added `WS_EX_NOREDIRECTIONBITMAP` to the existing Overlay HWND extended styles.
The existing `WS_EX_LAYERED`, `WS_EX_TRANSPARENT`, `SetLayeredWindowAttributes`
call, DComp target, surface format, alpha mode, rendering, input bridge and
lifecycle are otherwise unchanged. This isolates the missing-redirection
surface hypothesis while preserving the tested click-through configuration.

## Validation

| Check | Result |
| --- | --- |
| Canonical `test.cmd` after the source change | PASS, exit 0; 60 targets / 63 invocations |
| Actual-adapter Diagnostics OFF/ON matrix | PASS, 102 scenarios × 2 = 204 processes |
| N06-M01 Zoom fixtures | PASS, 4/4 |
| Localization/resource audit | PASS, 18 locales |
| Isolated ASI build | PASS, exit 0 |
| In-game transparency/input validation | Pending user run |

The build reported two C4201 warnings in vendored `external/safetyhook/Zydis.h`.
No warning was emitted for the changed `composition_runtime.cpp` translation
unit.

Candidate path:

`build-artifacts/cm/g1e-alpha-candidate/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`

SHA-256:

`2F718B90AD9A09BBB716830102CBE6A9CF21A45FA02D51DED7EE9EE1CADBE9A4`

The candidate has not been copied to the game or release directories.

## Next live check

Use this candidate for one focused verification: game visible through all
transparent areas at startup and with the menu open; menu pixels render
normally; mouse clicks still pass through to the game; Delete toggles the
menu; closing it restores the unobstructed game. If black remains, capture a
screenshot and fresh Overlay/Startup logs. Do not infer NVIDIA/FG compatibility
from a successful alpha-only check.

**Initial status at report creation:** Original candidate visual validation `FAIL` (user-observed black
background). Follow-up candidate build/test `PASS`; fix effectiveness remains
`UNVERIFIED` pending live visual validation.

## Follow-up: invisible candidate and display reconciliation

The user subsequently reported that candidate `2F718B90...CADBE9A4` displayed
no Overlay at all. The fresh main log confirms this artifact identity and
`Diagnostics.Enabled=true`. Its Overlay log shows accepted render submissions
and menu toggles, but no startup-notification lifetime start and no
`OVERLAY_INPUT_OWNERSHIP=OVERLAY_ACTIVE`. The prior `first_visible_frame` marker
was emitted after drawing/commit without requiring display readiness, so it
cannot establish that this candidate's HWND was actually shown.

**The NOREDIRECTIONBITMAP candidate is rejected for live use.** Its addition
has been removed. The previous missing-redirection-surface hypothesis is not
established by the logs or the local pixel experiment. In particular, local
comparison with the actual Renderer displayed both transparent background and
UI with all three configurations: legacy constant alpha, constant alpha plus
NOREDIRECTIONBITMAP, and the G1-B configuration without either additional call.
Thus constant alpha 255 alone must not be described as a demonstrated cause of
the game's black screen.

### Confirmed source defect and changes

`ReconcileCompositionAttachment` returned immediately when the root attachment
already matched UI demand. Initialization sets `visualAttached_ = true` while
the newly created display HWND is hidden. With a pending startup notification,
the attachment demand is also true. The early return could therefore skip
`UpdateOverlayWindowBounds` until a separate host activation or geometry event.
Opening a menu while that notification remains pending also needs no root
change and could take the same early return. This is a concrete missing display
reconciliation path; it does not prove it caused every reported game symptom.

The current candidate:

- uses the G1-B HWND configuration: LAYERED/TRANSPARENT/NOACTIVATE/TOOLWINDOW,
  without NOREDIRECTIONBITMAP or `SetLayeredWindowAttributes`;
- reconciles display visibility when its desired state differs from the cached
  display state even if root attachment has not changed;
- records `first_visible_frame` only after display readiness is accepted;
- adds change-deduplicated `OVERLAY_DISPLAY_STATE` diagnostics with requested
  visibility, root attachment, host activation/minimization, display visibility
  and game/foreground/display HWND identities. Diagnostic reads and formatting
  are gated by the existing runtime Diagnostics setting.

### Validation of the current candidate

The new local fixture includes the actual production composition source for
window creation and links the actual Renderer and its production dependencies.
It uses a colored surrogate host and desktop pixel capture, rather than
equating API success with transparency. It installs no game hooks.

Source: `research/prototypes/overlay_owned_hwnd/renderer_alpha_smoke.cpp`.  
Runner: `research/tools/run_overlay_owned_hwnd_renderer_alpha.ps1`.

The initial foreground-dependent sandbox and desktop attempts returned
`Unverified`, exit 2. Pixel-only measurement then supplied an explicit surrogate
activation observation; it proved rendering only, without claiming actual
foreground ownership. Its initial menu measurement was corrected to capture
one desktop bitmap and render multiple sizing frames before sampling.

The final native initial-show run used `SendInput` only on its own surrogate
window. It confirmed real foreground before Renderer initialization, then
verified display visibility without any later activation/geometry callback:
`INITIAL_SHOW same_root_without_later_host_callback=PASS`. The cursor and
foreground are restored after the fixture finishes.

| Final check | Result |
| --- | --- |
| Full canonical `test.cmd` on final production source | exit 0; 63/63 invocations, 204/204 adapters, 4/4 Zoom fixtures |
| Single isolated ASI build, after tests | exit 0; two existing vendor Zydis C4201 warnings |
| Actual Renderer startup/menu pixel fixture | exit 0; all four background corners RGB 40,100,160 in both phases; visible UI samples 84 for toast and 344 for menu |
| Native initial-show fixture | exit 0; display shown without a later host callback |
| Display-state diagnostic deduplication | PASS; unchanged frames add no display-state records |
| Game validation of the current candidate | Pending |

Candidate:

`build-artifacts/cm/g1e-display-candidate/production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi`

SHA-256: `C7287C8C52233EC5B8F8E32A0DBC6C38209DDA15F9C07FE99C505CA4FDC75C7C`.

Raw local validation output is in
`build-artifacts/overlay-owned-hwnd-renderer-alpha/` (`canonical-test.log`,
`candidate-build.log`, `current.log`, `current-initial-show.log`). The earlier
G1-E and rejected NOREDIRECTIONBITMAP ASIs remain in their original build
directories. The current candidate has not been installed or launched in game.

The supplied invisible-candidate session logs are preserved under
`research/evidence/overlay-compatibility/owned-hwnd-display-2026-10-13/invisible-2F718B90/`.
Git status remains readable, but `git diff --check` reported that it could not
run in a work tree, including with an explicit work-tree argument. That check
is not reported as PASS. The edited source sections were reviewed by readback;
no index mutation or destructive Git operation was performed.

**Current status:** confirmed missing display-reconciliation branch repaired;
current offline validation PASS. Game transparency/visibility and NVIDIA/FG
compatibility remain pending. The original in-game black-background mechanism
remains unknown.

## Latest user-run live session — C7287C8C

The user reports visible Overlay and stable NVIDIA Performance HUD/G-SYNC
Indicator behavior. The initial message described no freezes; a subsequent
clarification explicitly confirms that the 11.708-second trace gap corresponds
to a real in-game freeze. Live stability is therefore FAIL. The indicator disappears while the mod menu
is shown; the user accepts that behavior. The Performance HUD is reported to
work with this candidate, providing a practical positive result for continued
G1 evaluation. This does not establish whether VRR is active or disabled.

The main log confirms ASI SHA-256
`C7287C8C52233EC5B8F8E32A0DBC6C38209DDA15F9C07FE99C505CA4FDC75C7C`,
game SHA-256 `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`,
PID 30008 and runtime Diagnostics ON. Overlay logs show successful toast lifetime
start, two accepted menu open/close cycles, matching GameHwnd/foreground HWND,
input ownership on each opening and cursor/input restoration on each closing.
No failure/error/exception markers were found in the supplied three logs.
No finalizer/shutdown outcome is recorded, so internal teardown is not credited
as verified by this session.

Preserved inputs:
`research/evidence/overlay-compatibility/owned-hwnd-display-2026-10-13/live-C7287C8C/`.
CSV SHA-256: `8F20F4E324AE7B09AACC22998606A35F23544C5F3FE2725141123DAFDE50ABC4`.

### PresentMon results

The CSV contains 3698 records: 3691 DXGI records for swapchain
`0x221CD731540` and seven separate `Other` records for swapchain `0x0`.
The streams were analyzed separately. DXGI capture spans the CSV's timestamp
labels 16:16:12.250–16:17:35.343, about 83.093 seconds.

Median DXGI `FrameTime` is 16.6423 ms, p95 25.4228 ms, p99 84.5227 ms and
maximum 2057.6631 ms. Results were cross-checked with the existing PowerShell
capture analyzer, independently of the JavaScript calculation. Percentiles
use nearest rank.

CSV timestamp labels are three hours ahead of the main log's displayed times
(six hours ahead of the startup journal's FILETIME UTC). The offset was checked
against two menu openings/closings: the QPC-derived toggle times match the
Composed/Independent Flip transitions within about 4–22 ms. The reason for
the timestamp-label offset is unknown; it is not a measured game delay.

| Captured event window | Maximum DXGI FrameTime |
| --- | --- |
| Initial captured visible-toast segment, before Independent Flip at CSV 16:16:21.077 | 108.1939 ms |
| Within ±1 s of that Independent Flip transition, near expected toast expiry | 32.1193 ms |
| Within ±1 s of first menu opening | 67.3630 ms |
| Within ±1 s of first menu closing | 33.1364 ms |
| Within ±1 s of second menu opening/closing | 30.7269 ms |

These captured UI transitions show no multi-second frame-time spike.
The exact toast-expiry QPC is not logged; its association with the first
Independent Flip transition is supported by the locale/startup timing and
ten-second lifetime, rather than a direct timestamped detach observation.

There are nevertheless four long DXGI frames, all after that toast interval
and before the first menu opening, while the Overlay is settled closed:

| CSV time label (interval start) | FrameTime | GPUWait | PresentMode |
| --- | --- | --- | --- |
| 16:16:44.765 | 1512.4619 ms | 1490.0968 ms | Hardware: Independent Flip |
| 16:16:53.557 | 2057.6631 ms | 2025.3967 ms | Hardware: Independent Flip |
| 16:17:07.214 | 1383.0072 ms | 1367.8440 ms | Hardware: Independent Flip |
| 16:17:20.554 | 1087.0309 ms | 1074.2275 ms | Hardware: Independent Flip |

These are recorded long intervals; they are separate from the specifically
user-confirmed 11.708-second freeze. They do not coincide with a recorded Overlay opening or closing.
`GPUWait` is a metric here, not proof of a particular driver/fence failure.
The cause is unestablished; do not assign it to the mod or system.

A separate 11.7084961-second gap occurs between CSV rows 704 and 705,
16:16:24.060–16:16:35.769. Row 704 has FrameTime 16.5215 ms and row 705
has FrameTime 7.2035 ms: the gap is not represented by the neighboring
frame-duration values. The user's explicit confirmation establishes this
interval as a real freeze; the neighboring short metrics must not be used to
dismiss it as a harmless recording artifact. The capture has no coverage
of the initial ASI initialization: its first record is approximately 46.75 s
after initialization-worker start and 8.083 s after the first DComp commit.

**Session result:** NVIDIA HUD coexistence is positive; live stability is FAIL
because of the confirmed 11.708-second freeze. Its cause remains unknown.
Initial mod loading is outside capture coverage. Successful UI transition
timings do not override the failed stability criterion.

## Bounded read-only freeze audit

The audit uses the preserved PID 30008 session above and the current source
paths corresponding to the C728 candidate. It changes documentation only;
no production/vendor source, build, fixture run, game launch or ASI install
was performed during this audit. Work remains in this chat.

### Timeline

Timestamp labels in the CSV run three hours ahead of the main log's displayed
clock. QPC/FILETIME conversion was verified against both menu open/close cycles.
Fractional CSV timestamps were converted to 100 ns ticks before calculating
the interval, independently confirming its length.

| Event | Main-log clock equivalent | CSV clock label / timing |
| --- | --- | --- |
| Initialization worker starts | 13:15:25.500 | 16:15:25.500 equivalent |
| First DComp surface commit | 13:16:04.167 | 16:16:04.167 equivalent |
| Auto locale accepted, preceding toast creation | 13:16:11.030 | 16:16:11.030 equivalent |
| Independent Flip resumes, near expected toast expiry | 13:16:21.077 | 16:16:21.077428100 |
| Confirmed freeze starts | 13:16:24.061 | 16:16:24.060912100; mapped QPC 22086801886 |
| Confirmed freeze ends | 13:16:35.769 | 16:16:35.769408200; mapped QPC 22203886847 |
| First menu opening | 13:17:23.784 | 48.0148056 s after the freeze ends |

The freeze starts 58.5607566 s after initialization-worker start, 19.892706 s
after the first surface commit, and 2.983484 s after the Independent Flip
transition near toast expiry. The actual toast detach/hide records have no
timestamp, so that last association is approximate, not a measured DComp-call
duration. Menu opening did not trigger the confirmed freeze in this session.

### Overlay state and candidate causal paths

| Path | Current evidence | Assessment |
| --- | --- | --- |
| Overlay visibility | Ordered log records show root detached/display hidden after the toast, followed later by successful menu opens. No visibility transition is timestamped within the freeze. | Latest observed UI state is closed/hidden; exact owner activity during the gap is unmeasured. |
| Target destroy/recreate | Normal close uses final clear, SetRoot(null), Commit and ShowWindow(SW_HIDE). DComp target and surface remain resident; ResetComposition is a distinct reset/rebind/recovery/terminal path. No matching reset/rebind/recovery failure record appears. | The previous target-release experiment's destroy/recreate path is not the normal close path of C728. No observed invocation ties it to this freeze. |
| Owner idle/render scheduling | SelectPresenterFrameState selects Idle after notification expiry with no pending redraw. UpdateRenderTimer turns compositor demand off/cancels fallback animation timer; owner waits on messages/events, and the frame-clock worker parks on its own events. | A continuing per-frame render loop is not expected in settled idle. There are no interval counters or stack samples proving the actual worker state during the freeze. |
| Hidden redraw / host events | Game geometry, DPI, rebind and owner wake events can still execute work while the menu is closed. Same-size OnWindowGeometry may render/commit. The display HWND is excluded from its own WinEvent observer. | Closed does not guarantee zero work. There is no timestamped callback/Commit entry-exit coverage to identify an operation during the freeze. No completed geometry update is logged between toast hide and first menu open. |
| Game WndProc waiting on owner | OverlayWindowProc uses atomic admission, bounded command enqueue and try-only bridge operations, then native continuation. Settings, persistence and Renderer mutex work run on owner. The game-language native read remains a direct game-thread operation, but its accepted locale record predates the freeze by about 13 s. | No direct synchronous Renderer mutex/owner join path was found in the audited callback. Native game continuation and its waits are not measured. |
| Terminal finalizer | It runs only after the owner exits the message loop and closes one-way admission. No terminal/disabled/destroyed marker is recorded. Later successful menu/input cycles in the same lifetime are inconsistent with completed terminal admission closure. | No evidence supports finalizer execution as the freeze trigger. Its blocking drain/join operations belong to terminal shutdown, not ordinary toast/menu close. |
| Presentation / GPU / game work | The interval has no present records. Neighboring FrameTime/GPUWait/CPUWait values do not account for its duration. At resumption PresentMode briefly becomes Composed: Flip, then Independent Flip returns. | CSV cannot distinguish a game-thread stall, Present/driver/DWM stall, or owner-originated graphics interaction. The post-freeze mode transition may be an effect; it does not establish the cause. |

### Evidence needed to discriminate causes

The existing logs have no timestamped owner heartbeat while closed, operation
entry/exit durations covering the interval, game-thread wait stacks, driver
events or CPU scheduling samples. No raw `.etl` trace was found in the supplied
PresentMon instrument directory. CSV alone cannot recover those facts.

A future focused ETW capture with thread scheduling/wait stacks and graphics
events across the freeze could discriminate between game/native continuation,
presentation pipeline and owner activity. That is a proposed next evidence
step, not a new test run or instrumentation implemented by this audit.

**Audit conclusion:** freeze confirmed; stability FAIL; no code-level cause
established by the available evidence. Keep the positive HUD result. There is
no justified DComp/input/finalizer implementation change from this audit.

### Approved next evidence step: ETW preparation (2026-10-13)

Prepared `research/tools/capture_overlay_idle_etw.ps1` and a CMD launcher for
one bounded file-mode `CPU+GPU+DesktopComposition` recording before game launch.
See `research/tools/OVERLAY_IDLE_ETW_CAPTURE.md` for the procedure and verified
preflight details. Named session ownership preserves existing recordings;
installed ASI/EXE/INI identity, UTC/QPC anchors and logs accompany the ETL.

Local elevated preflight without the game: capture PASS, no lost events/buffers,
real scheduling/CPU stacks and DXGI/DxgKrnl/DWM/DirectComposition events verified.
Initial xperf stack export failure was corrected and rechecked on the same ETL;
it was not counted as PASS. Recording and rundown overhead is documented.

No ASI/source changes, builds, installation or game launch occurred in this
step. Actual game ETW capture and freeze mechanism remain pending; G1 live
stability remains FAIL, HUD compatibility remains positive, and N03–N06
statuses are unchanged.

### First successful game ETW capture — 2026-10-13 14:02

Evidence: ignored `build-artifacts/overlay-idle-etw/20261013-140228-Capture-2460/`.
ASI remains `C7287C8C…FDC75C7C`; game SHA remains `61BC1E03…E08D56B`.
Runtime log reports Diagnostics enabled. No new game launch, ASI changes or
installation were performed by the analysis. The user supplied this capture.

Recording covers UTC 11:02:33.0314753–11:04:54.3430085 (local 14:02–14:04),
141.3115332 seconds including rundown. WPR saved the 9,108,979,712-byte ETL;
trace statistics report zero lost events/buffers. The controller detected an
earlier same-named process PID 31604, but startup journal identifies the actual
mod-bearing game as PID **30120**, owner TID **32612**. Analysis is filtered to
30120; the controller's first PID must not be treated as authoritative.

DXGI extraction finds 2,571 matched Present Start/Stop pairs for game swapchain
`0x000001de4d6c3db0`, presenting TID **30472**. Maximum paired Present duration
is **19.218 ms**. Two long start-to-start gaps are:

| ETL-relative interval | Duration | Local clock interval |
| --- | --- | --- |
| 55.838110–63.889709 s | 8.051599 s | 14:03:28.870–14:03:36.921 |
| 75.657814–88.607185 s | 12.949371 s | 14:03:48.689–14:04:01.639 |

These are gaps between game DXGI Present calls, not 8/13-second Present calls.
The first surface commit is near 14:03:27.615; accepted locale is logged at
14:03:35.627. Assuming toast creation follows that locale and its usual ten-
second lifetime, the second gap starts roughly three seconds after expected
toast expiry, reproducing the earlier timing pattern. Hide/notification records
still lack exact timestamps; this is not a direct measurement of hide-to-stall.

CSwitch events establish that TID 30472 switched out **Waiting / UserRequest**
at 75.696835 s and resumed at 87.037654 s: **11.340819 s off CPU**. It then
waited another **1.480024 s** (87.059384–88.539408 s). These waits explain much
of the 12.949371-second presentation gap. `UserRequest` does not identify a
mutex, fence, responsible component or initiating API by itself.

Bounded stack reports for 75–89 s contain real CSwitch stacks for the game
presenting thread and Overlay owner. The presenting thread's aggregate stacks
include the game, Streamline interposer, OptiScaler, RTSS, Steam Overlay and
graphics modules. Owner aggregate stacks include Win32 message/window paths.
Aggregate presence is **not attribution of the long wait**. Function symbols
are largely unresolved. The exact 75.696835-second switch-out is present in the
CSwitch event stream, but a 3-microsecond, PID/TID-filtered stack report yields
zero hits; the time-shifted dump labels nearby frames with the incoming TID 484.
Those frames cannot safely be presented as the blocking stack of outgoing TID
30472. The exact blocking stack remains unresolved in this bounded extraction;
do not replace it with the aggregate module list. Frame-clock TID is not yet established.

Generated analysis files remain ignored: `trace-stats.txt`, `dxgi-events.csv`,
`cpu-scheduling.txt`, `thread-events.csv`, `target-thread-waits.json`, and bounded
stack reports. The full ETL was not expanded into an unbounded event CSV.

**Result:** capture usable; presentation pause and long waiting interval on the
presenting thread verified. This narrows the immediate mechanism to work/waits
between Present calls, not a synchronous multi-second DXGI Present. It does not
yet identify the waited object or establish that toast hide caused it. Preserve
G1's positive HUD outcome and live stability FAIL; no production fix is justified
from aggregate stacks alone. No additional live capture is requested.
