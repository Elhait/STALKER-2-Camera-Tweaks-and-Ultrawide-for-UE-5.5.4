# Whole-mod startup failure: overlapping log ownership

## Evidence reconciliation

The supplied CameraTweaks logs are from PID 20020, not proof of camera runtime
initialization in the subsequent playable PID 25220. Earlier analysis treating
all supplied logs as one continuing process is superseded.

| Boundary | Evidence |
| --- | --- |
| First process | RTX MFG canonical log: PID 20020 start 18:09:26.996; normal ExitProcess at 18:09:32.604 |
| CameraTweaks initialized there | Startup journal PID 20020; identity hash 491829…716AE; zero transactions through 18:09:31.434 |
| Overlapping second process | RTX MFG .25220.log: start 18:09:29.364; canonical log open error 32; its own PID fallback succeeds |
| Independent corroboration | Slop_yCam_Loader.log records pid=20020 at 16:09:27Z and pid=25220 at 16:09:29Z |
| Subsequent engine session | Stalker2.log opens 18:09:31; prior engine log closes that second; UE4SS continues through 18:12:21 |

## Code path

`NativeFile::OpenFile` uses `FILE_SHARE_READ` for writing, preserving exclusive
writer ownership. The first process holds the canonical log. The second
process cannot open that same path: Windows reports sharing violation.
`NativeLogSink` throws on open failure, and `Initialize` catches it, closes
readiness and returns `ERROR_OPEN_FAILED` before creating camera workers,
starting Overlay or installing camera hooks. This explains simultaneous native
Gameplay/Cinematic/Dialogue and no Overlay. It also explains why the existing
log misleadingly contains AVAILABLE: those records belong to the earlier PID.

The second CameraTweaks error itself was emitted only to OutputDebugString,
not captured. The combined runtime evidence strongly supports this mechanism;
the exact OS conflict is reproduced by the new harness before and after repair.
Runtime confirmation of the repaired artifact remains pending.

## Diff review and attribution

The current tracked diff covers 268 files, largely research records, fixtures
and tests. Production changes concentrate in runtime.cpp, provider/lifecycle
transactions, camera/FOV policy, new observation hooks and diagnostics. Those
policy decisions occur after the log initialization boundary. Capability
installation, baseline trust, provider selection or FOV formulas cannot account
for a second process aborting before any of them executes. Overlay discovery
itself is unchanged in this diff; its hidden-window records describe the first
process's short startup, not evidence for a global Overlay-only repair.

The exclusive open plus fatal logger failure already exists in HEAD. The new
initialization work can change launch timing, but no evidence establishes which
change caused the overlap to become visible. Do not claim a newly introduced
FOV bug or fabricate a specific introducing commit.

## Surgical repair

Diagnostic log opening retains the canonical filename if available. Only an
`ERROR_SHARING_VIOLATION` selects `basename.<current PID>.log`. Both files keep
exclusive ownership; the earlier log is neither overwritten nor shared.
NativeLogFile covers main/Overlay logs; the startup journal uses the same
opening helper. Config/staging NativeFile behavior is unchanged.

Regression `overlapping_startup_log_ownership` in worker_lifecycle_harness:
holds the canonical file open, attempts a second logger, verifies distinct
contents and cleanup. It failed on the original code and passed after repair.
This uses two owned handles to exercise the Windows sharing contract without
launching the game. It does not prove the repaired game session yet.
