# Overlay startup refresh diagnosis and targeted diagnostics — 2026-10-17

## Outcome

The old live run does not establish why the Overlay stopped after
`OVERLAY_WINDOW_EVENTS_ARMED`. Its logs establish that the Overlay worker and
both event hooks started, and that `CAMERA_CORE_READY` followed about 0.845 s
after the worker start. They contain no discovery scan, HWND selection, or
`OVERLAY_DCOMP_INIT_BEGIN` evidence. The missing stages are therefore an
instrumentation gap, not proof that a particular candidate was rejected or a
refresh was lost.

Static review found a conditional refresh-delivery defect: WinEvent and game
window callbacks posted a refresh thread message without checking whether the
post succeeded or retaining a durable refresh request. The initial
`CAMERA_CORE_READY` wake is separate: it signals the owner command event, and
the owner scans when no surface exists. Thus the callback-post defect is real,
but it is not established as the cause of the recorded startup failure.

## Traced startup contract

1. `StartOverlayDiscovery` creates terminal, drain and owner-command events,
   then starts the existing owner worker. A failed event/thread creation returns
   failure to the runtime startup path; no new worker or graphics mechanism was
   added.
2. `Worker` calls `PeekMessage` to create its message queue before publishing
   `g_uiThreadId`. It creates the separate display HWND and arms the process
   `CREATE/SHOW` and `LOCATIONCHANGE` WinEvent hooks.
3. The worker performs an initial discovery if camera-core readiness already
   arrived. Otherwise `NotifyOverlayCameraCoreReady` sets the readiness state
   and signals the owner-command event; the owner wake scans when there is no
   surface. This is not dependent on `PostThreadMessage` succeeding.
4. `FindGameWindow` enumerates current-process HWNDs and selects the largest
   visible, unowned, non-child window with a readable client rectangle of at
   least 320x200. The previous run has no per-candidate evidence to show which
   filter, if any, rejected the game window.
5. On the owner thread, `RefreshPresenter` returns before Renderer initialization
   when core readiness is absent, no eligible HWND exists, client geometry
   cannot be read, the renderer is disabled, the HWND is minimized, or the
   display HWND is missing. With a non-minimized candidate and no surface, the
   route is `Initialize`; `GAME_WINDOW_SELECTED` is recorded immediately before
   `Renderer::Initialize`, whose first DComp milestone is
   `OVERLAY_DCOMP_INIT_BEGIN`.

## Change made

- Added queue-ready publication after queue creation and before exposing the
  owner thread ID.
- Replaced callback-only refresh posting with a coalesced atomic pending bit
  and the existing owner-command event as the primary wake. If signaling that
  event fails, a checked `WakeMessage` post is attempted; both results and
  Win32 error codes are recorded. The redraw path likewise records its checked
  message-post result while retaining the owner event wake.
- Added bounded candidate counts and one sample HWND/client size per rejection
  category, selected HWND/client size, refresh request/dispatch, and
  transition-based `RefreshPresenter` return/route diagnostics. Hook
  registration now logs success/failure and the Win32 error.
- Added a deterministic policy harness for coalesced pending refreshes and a
  request arriving while an earlier request is being consumed.
- No camera, SafetyHook, DComp, input, z-order, or rendering behavior was
  intentionally changed. No timer or polling loop was introduced.

Diagnostic emission is gated by `[Diagnostics] Enabled=true`; fixed budgets
bound scan/request/dispatch/state/post logs. Startup hook-registration and
message-queue readiness markers remain single startup records.

## Evidence and limits

| Finding | Status | Evidence |
| --- | --- | --- |
| Queue could previously be posted to before it existed | Confirmed ordering hazard; fixed | Queue creation now precedes thread-ID publication |
| Callback refresh could be lost on failed message post | Confirmed conditional defect; fixed | Checked fallback post plus independent coalesced pending state and owner event |
| `CAMERA_CORE_READY` wake could be lost solely because `PostThreadMessage` failed | Not supported | It signals the owner-command event; owner wake scans when no surface exists |
| Game HWND was continuously rejected by a particular filter | Unknown | Old log had no scan counts, candidate HWNDs, dimensions or rejection reasons |
| Owner received/consumed a refresh, then `RefreshPresenter` returned early | Unknown | No old dispatch/return-reason diagnostics |
| `Renderer::Initialize` or DComp failed in the old run | Not indicated | No `GAME_WINDOW_SELECTED` or `OVERLAY_DCOMP_INIT_BEGIN`; no proof of an attempted Initialize |
| SafetyHook prevented Overlay startup | No dependent path established | Overlay worker/discovery is a separate optional subsystem; current log shows no evidence linking hook state to this missing transition |

One conditional limitation remains explicit: if WinEvent hook registration
fails, the startup marker reports the failure, but this change does not add a
retry timer for registration. The observed run reports `OVERLAY_WINDOW_EVENTS_ARMED`,
so that limitation does not explain this run. A later diagnostic run with
`OVERLAY_WINDOW_EVENTS_PARTIAL` would be evidence for a narrowly scoped
registration-recovery follow-up, not grounds for unbounded polling.

## Validation and candidate

- `test.cmd -NonGraphics`: PASS, 62/62 registered harness invocations;
  graphics/WARP case omitted. The first sandboxed attempt could not create the
  hard-link persistence fixture (`ERROR_ACCESS_DENIED`); rerun with the needed
  filesystem permission passed, and the supervisor verified process-tree
  cleanup.
- Production Release build: PASS, `OVERLAY_STARTUP_JOURNAL` on, startup
  timeline and dialogue discovery diagnostics off. CMake emitted long object
  path warnings for unrelated harness target directories in the isolated
  output tree; the production ASI compiled and linked successfully.
- `git diff --check`: PASS (only existing line-ending normalization warnings).
- No game, WinDbg, WARP, or graphics fixture was run by the assistant during
  build validation. The user later performed a manual live run, recorded
  separately below. The assistant did not install the ASI.
- New diagnostic candidate: `build-artifacts/security-hardening/overlay-startup-diagnostic/STALKER2CameraTweaks.asi`
- Size: 3,566,080 bytes.
- SHA-256: `CE3583444BFBD906D99FA7288DB1990938549D45F9E0CDDD62B17D1E1FBD3D68`.
- Existing candidate files were not overwritten. Verified hashes include
  `DC2877E864C1C9B73CC2CB285C97EB2285837441C33CB3866D6133C5D21AD7DE`,
  `CC40B287F5C72A5AD7DE154241C0630094F0D49CA7523564F637327EC23F67CB`, and
  `492A5884528ED8BA7C6401D15FF178B9EF2AFA77DD1903F8A687A342D7591298`.

## Post-reboot runtime evidence

The user subsequently restarted the PC and ran the same candidate. The live
runtime identity line confirms mod SHA-256
`CE3583444BFBD906D99FA7288DB1990938549D45F9E0CDDD62B17D1E1FBD3D68`; the
game image SHA-256 was
`61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.

The post-reboot trace resolves the observed sequence:

- Initial scans again found two process HWNDs: the Overlay display and one
  invisible candidate with client size `0x0`; no eligible game HWND existed.
- Subsequent out-of-context events were received and dispatched. The game later
  created HWND `67584`; once it had usable client geometry (`5138x1487`, then
  `5120x1440`), discovery selected it. The initial invisible HWND was not
  selected as the host.
- `CAMERA_CORE_READY` was at QPC `597536344`; `GAME_WINDOW_SELECTED` was at
  `871364479`, with QPC frequency 10,000,000. Thus the eligible HWND arrived
  about **27.38 seconds after camera-core readiness**. Overlay worker start to
  camera-core readiness was about **0.93 seconds**.
- After selection, the logs record `OVERLAY_DCOMP_INIT_BEGIN`, device and
  surface readiness, first frame, input hook activation, startup hint
  draw/commit, visible state, and active Overlay input ownership. The user
  confirmed the Overlay opened through Delete and worked; the log records
  multiple visibility transitions.

This confirms that, in this run, discovery's repeated event-driven refresh
eventually selected the correct later-created HWND. It also demonstrates that
the `IsWindowVisible` filter did not reject the eventual visible host: it
rejected an earlier invisible zero-size HWND, while a distinct later HWND was
selected. It does **not** identify why the prior pre-reboot run never observed
an eligible HWND, nor prove that reboot itself caused the difference. No code
change was needed after this successful capture. The run confirms Overlay
startup/presentation and user interaction, but does not by itself establish
normal process shutdown/finalization because no corresponding finalizer record
was observed in the captured log excerpt.
