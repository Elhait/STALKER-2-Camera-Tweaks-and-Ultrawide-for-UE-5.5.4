# DirectComposition Overlay and frame pacing investigation

Date: 2026-10-12. Baseline: `e756442bcf9942923b4531fb513af893f6aecae7`.
Status: baseline audit and temporary C experiment complete; live A/B/C established
a closed-state interaction with the NVIDIA indicator; approved production adoption
passed offline validation. **Quantified FG/VRR and presentation-path comparisons
remain pending.** The RX 9070 XT stutter and absence with v1 are
reported observations, not captures collected by this investigation. The reporter's
FG mechanism (in-game FSR FG or driver AFMF) is currently unknown.

## Baseline lifecycle and ownership (before production adoption)

| Responsibility | Current source / behavior |
| --- | --- |
| Graphics ownership | `Renderer` owns private `ID3D11Device`, `IDXGIDevice`, `IDCompositionDesktopDevice`, target, visual and `IDCompositionSurface` through ComPtr. `ImGuiD3D11Renderer` retains that private device/context and UI buffers. No Overlay swapchain exists. |
| Startup | `TryStartConfiguredOverlay` gates `StartOverlayDiscovery` before any Overlay worker. Worker creates Renderer, compositor-clock worker/events, WinEvent hooks; waits for camera-core readiness before D3D11/DComp/ImGui resources and game-window subclass. |
| Attachment | `InitializeImpl`, `RebindWindow`, `RecoverGraphicsDevice` set visual content to the surface and target root to the visual. Drawing commits the tree. Target uses the game HWND, preferring a topmost composition target. |
| Open/close | Window procedure handles configurable toggle on key-up (Delete default) and Escape dismissal. `SetVisible` changes a flag and marks the surface dirty. Close releases mouse capture and schedules the final transparent clear. **It does not detach the root or release resources.** |
| Idle | `RenderImpl` skips a hidden, clean surface with no notifications. `UpdateRenderTimer` disables compositor-clock demand and cancels fallback timer when idle. Worker blocks in `GetMessage`; clock worker blocks on its control/stop events. Static open UI also does not render continuously. |
| Notifications | Startup hint lasts 10,000 ms from accepted draw/attachment publication. It is created even when settings are closed; Auto-language synchronization can delay creation. Other notifications also render/fade while settings are closed. Expiry schedules a final clear. |
| Closed events | Notifications, explicit redraw requests, geometry/DPI/window changes and resource recreation can still cause work. Ordinary camera publication wakes the presenter only while UI is visible. These event paths are distinct from steady idle. |
| Present / Commit | No Overlay `Present`, `Present1` or game swapchain/factory interception. Composition drawing uses BeginDraw, EndDraw and Commit. Resize additionally commits SetContent; reset commits SetRoot(nullptr). No periodic idle Commit path was found. |
| Delete dependency | Delete uses the retained game HWND subclass. Detaching a visual can preserve this hook; removing the input hook on every close would remove this reopening path. Closed messages still traverse the subclass and renderer mutex before/after forwarding. |
| Disabled configuration | `[Overlay] Enabled=false` skips the startup callback, therefore no Overlay worker, pacing thread/events, WinEvent/input hooks, graphics resources, target attachment or startup hint. Notification enqueue returns early. Camera modules and their own hotkeys remain independent. |
| Resize / loss | Resize prepares and draws a replacement, commits SetContent, then publishes the generation. Minimize suspends rendering. Device removal permits one bounded recreation; terminal failure disables only Overlay. |
| Teardown | `ResetComposition` empties root and commits before releasing owned ComPtrs. It is called for rebind, recovery and Renderer shutdown/disable. Overlay worker's message-loop exit does not invoke Renderer::Shutdown or delete its resident Renderer. Supported game lifetime is process-resident; normal process exit relies on OS reclamation. Full dynamic unload is not established. |

Source anchors: `src/overlay/presenter/renderer_runtime.hpp` (ownership,
SetVisible/needsRenderWork), `renderer_runtime.cpp` (InitializeImpl, RenderImpl,
DrawImGuiSurface, StartPendingNotificationLifetimes, OnWindowGeometry,
ResetComposition), `src/overlay/composition/composition_runtime.cpp`
(ProcessOverlayWindowMessage, UpdateRenderTimer, Worker, FrameClockWorker),
`src/plugin/optional_overlay_startup.hpp`, `runtime.cpp` (startup and notifications).

**BigChenga's architectural observation is correct:** the hidden production root
and its surface remain attached. Its claimed frame-pacing consequence remains a
hypothesis. No application draw/Commit during idle does not establish zero DWM,
driver or display-plane cost. Private surfaces can still participate in the OS
composition tree even though game Present is never intercepted.

Microsoft documents that [SetRoot(nullptr) empties the tree](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiontarget-setroot).
Changes are submitted through [Commit](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-commit);
[WaitForCommitCompletion](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-waitforcommitcompletion)
is a separate completion wait. The experiment introduces no such blocking wait.
Its `root_commit_accepted` records API acceptance, not proof of DWM processing or
scan-out. Allow a settling interval and use external traces for that distinction.

## Temporary A/B/C implementation

| Variant | Difference | Prepared artifact |
| --- | --- | --- |
| A | Same ASI as B; INI Overlay.Enabled=false | `build-artifacts/fp/A/STALKER2CameraTweaks.asi` and INI |
| B | Current hidden attachment policy, with bounded observation counters | `build-artifacts/fp/B/STALKER2CameraTweaks.asi` and INI |
| C | Same counters; closed settled state commits SetRoot(nullptr) | `build-artifacts/fp/C/STALKER2CameraTweaks.asi` and INI |

All use the required ASI basename. B/C share identical INIs. A changes only
Overlay.Enabled. The prepared base is the canonical default preview (RuntimeCombine,
Auto, GameplayHorPlus, Adaptive; Diagnostics=false). For a specific user's repro,
regenerate from **that same user's INI** with `-BaseIniPath`; do not change camera
settings between variants. The runtime source/camera code is unchanged.

C holds root attached while UI is visible, any notification remains, or startup
notification creation is pending. Reconciliation occurs after successful frame
preparation/clear and, for resize, after replacement resource publication. If
there is no new damage, it reuses the already initialized retained surface.
It reattaches for a new UI/toast before starting notification lifetimes. Failed
SetRoot/Commit uses the existing optional Overlay disable boundary. HWND input
subclass, workers, target, D3D11 device and surface remain alive. Device recreation
can briefly attach its initial clear surface before the next settled frame;
exclude that transition from steady-state captures.

The original research tool copied 402 tracked build inputs into an ignored source snapshot.
Only the copied Renderer, composition runtime and CMake receive instrumentation;
the inline probe assets also live in that snapshot. Canonical source/build files,
Git index, installed mod and release ZIPs were not changed. B has no additional
tree mutations; C changes only root attachment policy. Both count RenderImpl,
DrawImGuiSurface and Commit calls and emit snapshots on transitions/toggle, not
each frame. Clock-demand changes are logged equally in B/C.

Historical preparation requires the pre-adoption baseline. The tool now rejects
current production, which already detaches, rather than silently mislabeling it B.
Existing snapshots and assets under `research/prototypes/overlay_attachment`
preserve the experiment; normal builds validate the adopted production behavior.

## Temporary experiment offline checks

| Check | Result / limit |
| --- | --- |
| B/C complete ASI builds | Exit 0 each; 18-locale resource preflight PASS. Final logs have only two pre-existing vendor Zydis C4201 warnings per build. Initial long output paths and fixture driver warning were corrected; final fixture builds have no warnings. |
| Actual Renderer, real D3D11/DComp, hidden HWND subprocess | B and C exit 0. Actual startup hint retains root; close after expiry yields B=attached, C=detached. Three open/close cycles, closed resize, minimize/restore, hidden toast/expiry, HWND rebind and explicit Renderer shutdown passed. Fixture shares the candidate's actual compiled production objects except DLL entry; settings callbacks are controlled. |
| 500 ms requested idle wait | Observed 516 ms between pre/post snapshots in each native fixture: **0 additional RenderImpl, 0 draws, 0 Commit attempts**. B root accepted=1; C=0. This exercises needsRenderWork gating with manual fixture pumping, not the full production worker or DWM pacing. |
| Focused original harnesses | composition_presenter_state, input_state, optional_overlay_startup: exit 0 each, built from the snapshot. Disabled-start callback not invoked; notification/scheduler/input contracts pass their existing scenarios. |
| Input/source preservation | SHA-256 checks: all 402 canonical inputs unchanged. No Gameplay/Cinematic/Dialogue/runtime source change. Native camera callbacks were not rerun in the game. |
| Capture analyzer | Synthetic multi-swapchain/NA/percentile check and actual fixture idle-log parsing PASS. Synthetic values are tooling checks, not frame-time results. |
| Real game Delete path | Subsequent user-run C verified three open/close cycles with matching accepted root transitions; see live results below. |
| Actual device-removed recovery, game shutdown | Not established by the supplied C logs. HWND rebind and explicit Renderer shutdown are narrower fixture checks. |
| Frame pacing with FG OFF / ON | **No measurements supplied.** The FG mechanism/settings and local GPU/driver have not been established; RX 9070 XT is the original reporter's context. |

Raw build/fixture logs, source manifest and artifact hashes are in ignored
`build-artifacts/fp`. Research-only fixtures never launch the game or initialize
native game hooks. API acceptance on a hidden HWND does not establish visual or
driver behavior in S.T.A.L.K.E.R. 2.

Artifact SHA-256:

| File | SHA-256 |
| --- | --- |
| A/B ASI | `EB29B394810F55E23BD6C73DF060379A7B1281D13F895D36AEE0B70FBB321CE6` |
| C ASI | `78357B990DC705DD3973339E0EA4FE733E2484A2DCB1461F7C33F859BC2B624E` |
| B/C INI | `508F99B0D0EAD161E93955C32909C56CFA33E4C15D875EC468B0E56444CAFE4D` |
| A INI | `779016614F495ECFD37A285E6D17A1B8994D7E2C73CE0E249C9BB579EA636A15` |

## Runtime comparison protocol and current results

The user ran A/B/C and reported another overlay's visible behavior. These are
compatibility observations, not a completed six-cell FG OFF/ON timing matrix.
There is still no measured frame-pacing delta.

| Live run on 2026-10-12 | Log evidence | User visual observation |
| --- | --- | --- |
| A, 15:16 | ASI EB29…21CE6; Overlay disabled; camera modules available. Overlay.log was stale from 02:38, not created by A. | G-SYNC Indicator stayed visible throughout. |
| B, 15:27 | Same ASI and EXE as A. Closed root=1, clock demand=0; 195,297 ms from idle to next toggle with unchanged render/draw/Commit counters. | Indicator disappeared. |
| C, 15:46 | ASI 7835…624E. Three open/close cycles each committed root 1 then 0. First closed interval 66,328 ms and second 21,015 ms had no new render/draw/Commit calls. A later closed interval had geometry-only frame preparation, with no additional draws/Commits. | Indicator disappeared on opening and reappeared on closing. |

All three main logs identify EXE SHA-256
`61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
Reported camera selections and Diagnostics=false match. C raw logs were preserved
in ignored `build-artifacts/fp/live-C-20261012` with a SHA-256 manifest. A/B results
are from the earlier direct log reads and user observations; their files were
overwritten by later runs. Exact NVIDIA GPU/driver, FG state and display settings
were not captured, so do not extend this result to the RX 9070 XT reporter's setup.

The indicator is an observable compatibility symptom. This experiment supports
the role of attachment in that symptom even without ongoing application commits.
It does not establish that actual G-SYNC stopped, nor identify an MPO, Independent
Flip, DWM or NVIDIA internal mechanism. BigChenga's retained-attachment suggestion
provided the useful investigation direction.

Use the same save/scene, route duration, resolution, display mode, FOV, driver,
refresh rate, VRR/VSync, FPS cap, upscaler, FG mechanism and third-party tools.
Record EXE/ASI/INI SHA-256, GPU/driver/Windows versions and PresentMon version.
Warm shaders/assets through the route before collecting equivalent 60-second
captures. Restart the game between variants/FG settings. Load only one ASI named
STALKER2CameraTweaks.asi. Preserve the user's original files before switching.

For B/C, wait until the startup hint expires; open/close once with Delete; require
`idle_begin` with root accepted B=1/C=0 and clock demand off where that API exists.
Allow another 2 seconds to settle. No toast, resize, focus change, UI or hotkey
notification during the capture. After capture, press Delete once so `toggle_before`
records counters at the end of the closed interval; that press is outside metrics.
A must show disabled configuration in its fresh main log. A does not create a new
Overlay log; do not use a prior run's old Overlay log as A evidence.

Pilot all six cells; repeat any apparent B/C difference at least three times with
rotated order (ABC/BCA/CAB) and comparable warm-up. Compare run-to-run variation
before attributing a difference. If no effect is seen in a single pilot, record
that limited result rather than asserting universal neutrality.

Use an external PresentMon console/CapFrameX capture with its on-screen overlay
disabled. An example for a console version supporting these documented flags:

```text
PresentMon.exe --process_name Stalker2-Win64-Shipping.exe --timed 60 --terminate_after_timed --output_file B-FG-on-1.csv --no_console_stats --qpc_time
```

Check that installed version's `--help`. Track display metrics; do not use
`--no_track_display`. The [PresentMon console documentation](https://github.com/GameTechDev/PresentMon/blob/main/README-ConsoleApplication.md)
distinguishes application presents from display timing and notes that frame-type
tracking depends on provider instrumentation. Capture available display durations,
application frame times, PresentMode, dropped/undisplayed counts and supported
FrameType metadata; never assume application presents alone count generated frames.
If the provider cannot expose actual FG display cadence, use a suitable display
capture or external measurement and document that gap.

Analyze each game swapchain/frame stream separately with
`research/tools/analyze_overlay_frame_pacing_capture.ps1 -CsvPath ... -MetricColumn ... -OverlayLogPath ...`.
Choose the metric explicitly according to capture schema. It reports median,
nearest-rank p95/p99, maximum, >2x-median and >50/>100 ms counts; it excludes invalid
durations and preserves stream identity. Compare distribution/tails and PresentMode,
not average FPS alone. To investigate DWM/driver work despite zero app commits,
collect a matched graphics ETW trace (DWM/DxgKrnl/Win32k with GPUView/WPA) using the
same tracing overhead in B/C. No ETW session was started here.

Separately exercise Delete reopen/close during startup toast and after idle, a
hotkey toast while closed, resolution/DPI changes, minimize/restore and normal game
exit. Device removal/recreation remains an explicit gate if it cannot be safely
triggered in a repeatable environment.

## Decision

The confirmed compatibility interaction justifies removing unnecessary settled
closed-state composition attachment. The user approved production adoption on
2026-10-12. This is general attachment lifecycle management; no NVIDIA detection
or indicator-specific workaround is used. Quantified FG/VRR claims remain open.

## Production adoption and validation

Renderer owns `visualAttached_`; its root transitions use the existing serialized
owner path. A small transaction helper publishes the flag only after SetRoot and
Commit succeed. A failure disables only Overlay through its existing boundary;
frame/geometry/startup return values now propagate that failure. Clear/draw and
resize publication precede reconciliation; reattachment precedes toast lifetimes.
Input hook, Delete, private device/surfaces and readiness remain available while
detached. Rebind/recovery mark the newly committed root correctly; settled frames
reconcile it again. Prototype counters, variant flags and clock-demand logging
are absent from production. Only event-based root attachment logs are retained.

| Production check | Result |
| --- | --- |
| Full canonical test.cmd | Exit 0; 60 targets / 63 invocations; 102 actual-adapter cases x Diagnostics OFF/ON = 204 children; four N06-M01 invocations PASS. |
| Root transaction regression | Actual production helper: attach/detach order, no redundant calls, rejected SetRoot, rejected/throwing Commit, and no premature flag publication PASS. |
| Localization source/resource audit | 18 locales PASS. Its old exact `return rendered;` matcher was updated to require propagated attachment success, retaining committed notification checks. Initial template test constants caused unreachable-code warnings; final dynamic fault cases compile without those warnings. |
| Production build.cmd after tests | Exit 0. Only unchanged vendor Zydis C4201 warnings; no new production warnings. |
| Actual production Renderer fixture | Exit 0: startup toast, three open/close cycles, settled hidden state, closed resize, visible/closed minimize/restore, notification/expiry, visible/closed HWND rebind and explicit Renderer shutdown. Uses actual production objects/resources, controlled settings and a hidden test HWND. |
| Camera/runtime/INI/release scope | No camera/runtime, INI, build registry, vendor or release-package changes. |
| Final production game smoke | Pending; experimental C live evidence is not a game run of the newly built production artifact. |

Production artifact: `build-artifacts/detach/STALKER2CameraTweaks.asi`.
SHA-256: `BF5B1E9460D5A75AF32D0CB09A86CCDE6FA86A0B1EE04BBF569CDA373391ABCD`.
Raw production build/test/fixture outputs are beside it. The game was not launched
and the installed ASI/release ZIPs were not replaced during this adoption.

Actual device-removed recovery and game-process shutdown remain narrower evidence
limits. Helper failure tests do not impersonate fault injection into a real driver.

## Deferred compatibility research

Follow-up user testing with Performance HUD enabled before game launch found
that empty-root idle preserved the G-SYNC Indicator but the HUD disappeared;
opening the mod UI brought back the HUD and hid the indicator. HUD reactivation
also made activation ordering relevant. NVIDIA coexistence is therefore not
resolved by root detach. The read-only [coexistence audit](DCOMP_NVIDIA_OVERLAY_COEXISTENCE_AUDIT_2026-10-12.md)
records this evidence separately and identifies retained HWND target-layer
occupancy as a candidate, not an established NVIDIA mechanism. Keep the approved
detach; no additional production change was made.

| Deferred item | Evidence / impact | Scope and revisit trigger |
| --- | --- | --- |
| DCOMP-ACTIVE-COEXISTENCE | Open: C still makes the indicator disappear while UI is attached. Other NVIDIA UI and OptiScaler/ReShade coexistence were not measured. | Investigate actual active-UI conflicts and display ordering from a repeatable case; assess candidate target/backend changes separately before implementation. |
| DCOMP-PRESENTATION-PACING | Open: no quantitative FG/VRR, PresentMode or MPO evidence. Source and counters do not identify OS/driver mechanisms. | Matched A/B/C display/ETW captures with GPU, driver and FG mechanism identified; measure distributions and presentation paths before proposing pacing changes. |

Do not infer universal compatibility or select another backend/target mode from
the indicator symptom alone. Preserve the existing runtime architecture and open
native-runtime safety verification gates.
