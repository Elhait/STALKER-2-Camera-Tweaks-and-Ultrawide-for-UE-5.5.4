# One targeted Overlay toast-to-idle ETW capture

Scope: unchanged ASI, no automatic game launch, no driver/settings changes.
The recording is system-wide; filter by the captured game PID in WPA rather
than filtering out DWM/driver threads at collection time. ETL can contain
process names, paths and activity of other applications; do not publish it raw.

## Preflight (without the game)

Run `capture_overlay_idle_etw.ps1 -Mode Preflight -Elevate` from PowerShell.
Accept the Windows UAC prompt. This records a short CPU/wait workload with the
same built-in `CPU+GPU+DesktopComposition` file-mode profiles used for the game.
Administrator rights are required; sandbox escape alone is not elevation.
Results go to ignored `build-artifacts/overlay-idle-etw/`.

Inspect the real ETL, not only profile configuration:

- CSwitch, ReadyThread and SampledProfile stack records for the preflight PID.
- DXGI, DxgKrnl, Dwm-Core and DirectComposition event records, not just headers.
- START/STOP_REQUEST markers and UTC/QPC sidecar anchors.
- Trace statistics, including lost events/buffers. Capture errors block approval.

`RECORDED` is deliberately not a PASS. The reviewed approval file
`research/tools/overlay_idle_etw_preflight.json` must name status
`PASS`, profiles `CPU+GPU+DesktopComposition`, machine/OS identity and the reviewed evidence folder.
Do not create it based only on a successful WPR start or provider availability.

## Single game session, after successful preflight

Run `capture_overlay_idle_etw.ps1 -Mode Capture -Elevate`.
Alternatively, double-click `capture_overlay_idle_etw.cmd`.
Wait for **Recording started**, then launch the game normally. Keep the current
INI, ASI and graphics settings unchanged. Let the welcome toast expire; do not
open the menu until after the toast-to-idle interval. Recording ends
automatically after 120 seconds (maximum override: `-Seconds 180`). No manual
stop command is necessary. Merge/rundown can take additional time.

If using PresentMon alongside it, start its existing capture before the game
and preserve the CSV; the script does not start or reconfigure PresentMon.
ETW already captures graphics events, so a second PresentMon run is not required.
Record whether/when the freeze was visible. Do not change Diagnostics just to
collect ETW; there is only one ASI with runtime INI control.

Wait for `result.json` or `failure.txt` in the new result directory, then provide
`capture.etl`, `anchors.jsonl`, identity/configuration files and copied logs.
The script snapshots the installed ASI, game EXE and INI hashes before recording;
it does not install anything. Log copies occur at capture end, not necessarily
after game exit. Orphaned sessions with this controller's exact naming are
saved/stopped only when their controller PID no longer exists. A live controller
blocks a new capture. Unrelated recordings are never cancelled. A failed stop can leave the named session live:
inspect `controller.log` before any further capture.

If an orphan's save returns `0x80070003` (recording path missing), the script
cancels that exact dead controller's named session and records the trace as
unrecoverable. No other stop error enables cancellation. `-Mode Recover -Elevate`
performs only cleanup and never starts a capture. After the 13:58 failed save,
the 14:00 cleanup-only check found no remaining controller sessions; syntax and
session enumeration were checked. The missing-path cancellation fallback was
not exercised by that check, since the failed save had already ended the session.

## Analysis and limits

Open ETL in Windows Performance Analyzer. Inspect CPU Usage (Precise), CPU Usage
(Sampled), thread lifetimes, GPU/DXGI/DWM events and markers across the freeze.
Use the new startup journal's `OVERLAY_WORKER_START tid=...` for the owner.
Identify the frame-clock worker from thread-start/module-offset evidence and
current source; do not reuse TIDs from a previous process or infer its identity
only from a wait near toast expiry. Private function names may be unavailable
without the exact build's symbols; retain module offsets and executable identity.

CSwitch/ReadyThread stacks establish scheduling/wait transitions, not a complete
stack sampled continuously through a wait. Reconstruct wait intervals and the
thread making it runnable. A stall in a kernel/graphics stack alone does not
prove the initiating Overlay API. A successful Commit is not proof of display.
Existing hide records have no precise timestamp; ETW may expose a window event,
but no new ASI marker is promised. Align by QPC/UTC, not the historical CSV's
unexplained fixed clock offset.

File-mode traces and merging use disk space and add overhead. Keep at least
20 GiB free for the bounded game capture. Do not loop captures automatically.
If event loss or excessive recording overhead compromises interpretation,
report that limitation instead of turning the trace into runtime PASS.

References: Microsoft [WPR command-line options](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/wpr-command-line-options)
and [wait analysis](https://learn.microsoft.com/en-us/shows/defrag-tools/43-wpt-wait-analysis).

## Verified local preflight, 2026-10-13

Evidence: `build-artifacts/overlay-idle-etw/20261013-134230-Preflight-24408/`.
WPR started/stopped successfully with elevated rights through UAC. No game was
launched. CPU/wait workload lasted about 8.24 seconds; total ETL including
rundown covers 32.9840625 seconds. ETL size: 1,254,096,896 bytes. Lost buffers
and events: **0**. Actual graphics provider records and START/STOP markers were
verified. Nonempty controller-PID CSwitch, ReadyThread and CPU Profile stack
reports establish actual stack delivery, not merely configured stack flags.

Initial processing failed (`xperf` exit -2147467259, missing `-butterfly`).
Corrected processing on the same ETL exited 0. Stack event-name filters require
`.*CSwitch.*`, `.*ReadyThread.*`, and `.*Profile.*`; literal `SampledProfile`
produced an empty report despite real CPU sample events. The script now uses
the verified filters. The first full event dump expanded to about 8.39 GB;
future preflight exports are bounded to a two-second excerpt. Raw files remain
ignored; no evidence was deleted. Capture permission is unlocked by the reviewed
approval JSON, not by the failed original controller result. The receipt now
lives under `research/tools/` so cleaning ignored build outputs does not erase it.
The original preflight raw directory was later found absent; the receipt
preserves the earlier reviewed result, not a claim of a new capture.

PASS is limited to recording capability on this machine. The game-session
capture, exact private symbols, frame-clock TID identification and cause of the
11.71-second freeze remain pending. The profile has measurable recording and
rundown overhead; do not interpret this local preflight as proof of negligible
in-game overhead.
