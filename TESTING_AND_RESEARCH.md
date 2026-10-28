# Testing and Research

This document summarizes the project's testing and research work across its major development stages: what was investigated, what the evidence established, what was validated, and what remains unknown. It is not a product overview or a transcript of every run. Exact procedures, raw logs and provenance remain in the linked reports and evidence; current product behavior and architecture belong in [`SUMMARY.md`](SUMMARY.md).

## Stage overview

- **Camera and native-runtime validation:** Gameplay, Cinematics, Dialogue, native hooks, camera ownership and selected transition/recovery paths were examined through source review, deterministic policy tests, production-adapter harnesses, recorded evidence and bounded live observations. The supported executable and callback assumptions are maintained in [`docs/SUPPORTED_BUILD_MANIFEST.md`](docs/SUPPORTED_BUILD_MANIFEST.md).
- **G0–G5 Overlay migration:** feasibility and failure-handoff fixtures preceded the Overlay-owned HWND integration. Offline work covered input admission, owner-thread cleanup, host lifecycle, geometry/scheduler behavior and regression. Integrated use then confirmed the Overlay/menu and NVIDIA HUD coexisting in the observed scenario; the accepted G-SYNC Indicator behavior and compatibility evidence limits are recorded below.
- **F2–F4 remediation:** the final-release hardening sequence addressed identified runtime, hook, configuration, input, viewport and test-containment findings. F4 preflight and remediation added bounded lifecycle/configuration checks and a cumulative non-graphics regression run.
- **F3 configuration lifecycle:** INI generation, preservation/migration, idempotence, malformed-file recovery and release-ownership behavior were tested separately from runtime camera behavior.
- **F5–F8 startup notification:** lifecycle diagnostics and deterministic state tests examined toast readiness, attachment, redraw/commit and TTL. A later user-observed startup run supplied positive visual evidence, while cross-run reliability and the physical presentation boundary remain unresolved.
- **Camera State UI:** the read-only third-column inspector, internal scrolling, responsive layout and localized open/close control were exercised through focused layout/localization checks and user visual review. This does not constitute a full DPI/aspect-ratio matrix.
- **Shutdown closure:** SL01/SL02 paths were traced to concrete CRT/file and static-container teardown mechanisms, hardened, then checked with focused tests, the safe non-graphics suite, a production build and an ordinary user-run game exit. The demonstrated contract and remaining process-termination limits are stated below.

## Test infrastructure and current evidence

- `test.cmd` is the canonical suite. CMake + Ninja builds isolated harnesses; the runner executes registered invocations serially, supervises process trees and propagates nonzero child results, including Windows exception codes and timeouts.
- The canonical registry contains **62 ordered invocations**; adapter scenarios and child-process totals vary by suite/profile and must not be conflated with invocation count. Historical runs recorded different totals as the registry evolved. The latest SafetyHook non-graphics regression recorded **62/62 invocations**, **112 native cases** and **218 child processes**, with graphics-mode children omitted. See the dated reports for each run's exact registry and scope.
- Hybrid migration established dependency-tracked incremental builds. The recorded warm-run medians (about 119 seconds versus about 391 seconds in the earlier runner) characterize the measured machine/setup, not universal performance.
- Exit-contract fixtures cover success, positive/negative failures, access violation, timeout and prevention of running stale executables after compile failure. Localization/resource audits cover 18 locale catalogs and embedded resources; they do not prove perfect Arabic shaping, RTL layout or all runtime glyph rendering.
- Focused policy, adapter and native-hook fixtures prove only their named contracts. Offline PASS does not prove game callback scheduling, live hook behavior, graphics presentation or full process/driver teardown.

The Auto Interface Language resolver was checked statically against all retained Ghidra game images except 1.9: 2.0.2–2.0.6. The reader signature and game-buffer release anchor each had exactly one executable-image match on all five images. The release anchor mapped through each AMD64 PE exception table to the expected function entry; its offset varies between versions, so production resolves the entry from function metadata instead of a fixed offset. The current installed game executable was not analyzed, and this check does not establish runtime language detection. Per-image hashes and raw Ghidra output are recorded in [`language-signature-cross-build-audit-summary.md`](../../02-Research/evidence/language-signature-cross-build-audit-summary.md).

The latest production build exposed and fixed a compile error in the PE exception-directory lookup (`NumberOfRvaAndSizes` was read from the wrong PE header structure). The default CMake tree was aligned with the candidate build layout at `build-artifacts/cm/candidate`. Release compilation then completed all 66 build steps with MSVC 19.44.35224.0. The built ASI hash was `4B59746220C3528B2411D6582E992FB1FB33B225D824C3315DA0670D16093E94`; the run used `CAMERA_TWEAKS_OUTPUT_NAME` to keep the published root and release-assets ASIs untouched. A full `test.cmd` run and in-game validation were not performed for this resolver change. The subsequent `build.cmd` keypress-wait edit was reviewed in source but not exercised by another build invocation.

Infrastructure details: [test architecture audit](research/reports/TEST_INFRASTRUCTURE_ARCHITECTURE_AUDIT.md), [performance baseline](research/reports/TEST_EXECUTION_PERFORMANCE_BASELINE.md), [hybrid migration plan](research/reports/TEST_INFRASTRUCTURE_HYBRID_MIGRATION_PLAN.md) and [implementation report](research/reports/TEST_INFRASTRUCTURE_HYBRID_MIGRATION_IMPLEMENTATION_REPORT.md).

## Research findings by subsystem

### Camera domains

- The Gameplay camera writer and register/source contracts are bounded by [`docs/SUPPORTED_BUILD_MANIFEST.md`](docs/SUPPORTED_BUILD_MANIFEST.md). Signature resolution fails closed when validation is ambiguous; a game executable update needs fresh resolver and runtime evidence.
- `RuntimeCombine` synchronizes Gameplay camera aspect to the current viewport, verifies read-back, then applies Hor+ FOV correction. Gameplay, Cinematics and Dialogue coordinate camera ownership and recovery. Offline transaction tests cover selected source/generation and transition contracts, not every game callback schedule.
- Cinematic framing/FOV has historical live and current deterministic coverage for automatic/forced aspect, ENTER/continuation/EXIT and Gameplay recovery. A prior baseline-acceptance defect was fixed with source/generation/ownership bounds. Current-policy selection is distinct from observed camera state.
- Dialogue supports `Adaptive`, `Reduced` and `Disabled`; there is no `Native` mode. Disabling correction leaves the game's default dialogue zoom behavior (70° target) in control. Adapter/domain and historical runtime evidence cover selected lifecycle, consecutive-dialogue, cinematic-isolation and ADS cases.
- Native aspect reevaluation can change the rendered result without the final stored aspect field matching physical display aspect. The engine-side reevaluation operation remains unidentified; storage-only writes are not treated as equivalent.

### Overlay migration G1–G5

- The Overlay renders through a dedicated, unowned top-level `OverlayHwnd` with a persistent DirectComposition target. `GameHwnd` remains the host/input/language/geometry boundary. The display window is click-through; the existing Overlay owner thread owns ImGui, D3D11, DirectComposition and display-HWND teardown. The Overlay does not hook game DXGI/Present.
- The G1–G5 Overlay-owned HWND migration and subsequent lifecycle/geometry work are complete. In observed game use, startup and menu display, input transitions and NVIDIA Performance HUD coexistence worked. The G-SYNC Indicator disappearing while the menu is open is accepted behavior and does not establish that VRR itself stops; these graphics states are not directly instrumented by mod logs.
- Current UI includes a read-only Camera State inspector in an independently scrollable third column and a localized “Camera Details / Open / Close” toggle. The user reviewed the live layout and reported the functional layout as good; offline/localization checks are not a full visual matrix across all DPI/scales.
- Compatibility evidence and its limits are recorded in [`OVERLAY_OWNED_HWND_G5_COMPATIBILITY_STATUS_2026-10-13.md`](research/reports/OVERLAY_OWNED_HWND_G5_COMPATIBILITY_STATUS_2026-10-13.md).

### Configuration and release ownership

- The INI is a user-owned runtime file generated beside the ASI, not a release ZIP/Vortex payload. Known older layouts are canonicalized while compatible values are preserved; malformed/unrecognized input is backed up before supported recovery. A canonical no-op does not rewrite the file. User-authored comments are intentionally not retained; generated canonical documentation replaces them.
- Overlay persistence changes only the requested values and avoids unnecessary writes. Focused persistence/idempotence, migration, malformed recovery and packaging checks passed.
- Details: [`INI_CONFIGURATION_LIFECYCLE_F3_2026-10-14.md`](research/reports/INI_CONFIGURATION_LIFECYCLE_F3_2026-10-14.md).

## Startup notification (F5–F8)

- F5–F6 retained one-shot notification identity, Auto-locale behavior, root-attachment demand and the 10-second draw/commit-gated TTL. Deterministic coverage exercises startup-only attachment and menu transitions; it is not a physical-pixel test.
- F7 added a readiness contract separating runtime initialization, Overlay readiness, observed game data and toast eligibility. A successful Auto-locale fallback is not mislabeled as observed game language. Manual menu opening remains independent. Focused deterministic checks and the recorded canonical suite passed for that stage.
- F8 confirmed the toast and menu share ImGui → D3D11 → DirectComposition rendering, commit and reconciliation paths. Logs/vertices/commit do not establish that startup pixels reached the user's screen; toast-specific pixel evidence is not captured by current instrumentation.
- Live outcomes varied: one F7 run was not seen despite successful internal lifecycle; a later user-observed run showed the toast before Delete for approximately ten seconds. Therefore automatic toast visibility has a positive live observation, but is not established as reliable across launches. No single presentation root cause or universal fix is claimed. Keep the current readiness gate and TTL absent new evidence.
- Reports: [F5](research/reports/F5_STARTUP_OVERLAY_TOAST_FINAL_HARDENING_2026-10-15.md), [F6](research/reports/F6_STARTUP_TOAST_ROOT_CAUSE_AND_REGRESSION_2026-10-15.md), [F7](research/reports/F7_STARTUP_TOAST_READINESS_GATE_2026-10-16.md), [F8](research/reports/F8_STARTUP_TOAST_PRESENTATION_VERIFICATION_2026-10-16.md).

## F2–F4 remediation and shutdown hardening

- The F2 R1–R9 remediation addressed test containment, runtime failure paths, native reset/continuation, configuration, input and camera viewport contracts. F4 then addressed concrete preflight findings; implementation evidence is in the linked reports.
- The identified SL01/SL02 CRT teardown paths are fixed for the specifically reviewed file/logging and selected global-container cases: process-lifetime/resident storage and native file/log sinks remove those automatic destructor paths, and ImGui automatic settings I/O is disabled. Focused shutdown checks, the non-graphics canonical suite, production build and ordinary game exit passed.
- This is not an unconditional shutdown guarantee. Crashes, explicit unload, arbitrary third-party/CRT teardown, driver/DWM completion and every possible concurrent interruption remain outside the demonstrated contract. `ExitProcess` does not promise a final mod log record.
- Details: [F4 cumulative audit](research/reports/F4_PREFLIGHT_CUMULATIVE_CODE_AUDIT_2026-10-15.md), [F4 remediation](research/reports/F4_PREFLIGHT_REMEDIATION_IMPLEMENTATION_REPORT_2026-10-15.md), [shutdown closure](research/reports/FINAL_ASI_SHUTDOWN_LIFECYCLE_CLOSURE_2026-10-17.md).

## Live stability observation

- A user-confirmed game freeze occurred near the startup-toast-to-idle interval. The available ETW/PresentMon evidence records a roughly 12.95-second presentation gap and long `UserRequest` waits on the presenting thread (about 11.34 s and 1.48 s). The waited object and blocking stack/cause were not identified; the exact Overlay hide time was not captured. This remains an unresolved observation, not a proven Overlay defect and not a fixed issue.
- Do not attribute it to DComp, the toast, NVIDIA or the owner thread without new causal evidence. The recorded G1 ETW analysis is in [`OVERLAY_OWNED_HWND_G1E_BLACK_BACKGROUND_FOLLOWUP_2026-10-13.md`](research/reports/OVERLAY_OWNED_HWND_G1E_BLACK_BACKGROUND_FOLLOWUP_2026-10-13.md).

## Native runtime safety and diagnostics

- Zoom IN/OUT callbacks use `SafeMidHookEntry`; production-adapter tests and a local executable-hook fixture cover normal and selected exceptional paths while preserving continuation context.
- SafetyHook state distinguishes unpatched, patched and uncertain target bytes. Trampoline/stub storage is retained where patched code could still reference it. Fault-injection coverage is selected-path evidence, not proof of every teardown schedule.
- Startup initialization has an outer exception boundary and guarded cleanup; controlled fixtures cover selected early/partial failures. Game-specific installer behavior is not fully reproduced offline.
- The ordinary ASI contains passive diagnostics controlled by `[Diagnostics] Enabled`. OFF-path checks establish gating of diagnostic-only continuation traces, counters, predictions and selected reads; shared observations required by production behavior remain active. A local continuation-trace microbenchmark is not a whole-frame or FPS measurement.
- Repeated restoration-state logging was reduced to state changes, removing the observed duplicate-log flood in the tested scenario; it does not prove logging caused earlier system-wide stalls.

### SafetyHook hardening and follow-up findings

- R1–R3 hardening covers page-protection recovery, execute-only VEH admission and private trampoline/stub allocation that transitions from RW to RX. WC01 addressed delayed execute faults processed after RX restoration; WC02 gives successful `VirtualAlloc` regions RAII ownership before later throwing operations.
- The later WB01/WB02 review found two additional concrete paths: retry suppression crossing transaction generations, and a continuation exposed through VEH before a pre-write failure without adequate owner lifetime. Both were corrected in the current reviewed source. Retry identity now includes source/page/allocation/type/generation; route exposure controls retention of trampoline and MidHook stub storage.
- Targeted native scenarios exercise same-address faults across generations, active-vs-completed route selection/address reuse, and a worker held inside a real trampoline while a pre-write protection failure and owner destruction occur. They check actual RX/committed state and continuation completion. The correction report records **6/6 focused process runs with diagnostics OFF and ON**.
- The subsequent canonical `test.cmd -NonGraphics` run passed **62/62 registry invocations** (112 native cases; 218 child processes). Release compilation and PE checks also passed for that engineering pass. These are offline results; they do not replace manual game qualification of the exact resulting build or establish universal thread-schedule safety.
- WB01/WB02 are closed by their targeted offline evidence. The earlier behavioral-overlap audit is an audit-time snapshot; its two findings were not established as causes of Microsoft or Cynet classifications. The later correction report is the current status reference: [WB01/WB02 correction](research/reports/SAFETYHOOK_WB01_WB02_CORRECTION_2026-10-17.md); see also [behavioral-overlap audit and follow-up](research/reports/WACATAC_BEHAVIORAL_OVERLAP_SECURITY_AUDIT_2026-10-17.md).

### Overlay startup discovery and live observations

- Startup diagnostics established that the message queue, WinEvent registration and owner-thread refresh dispatch can all succeed while no eligible game HWND exists. Discovery rejects the Overlay's own window and unsuitable invisible/zero-client candidates; it does not initialize DirectComposition until a suitable game HWND is selected.
- The refresh path now keeps a coalesced pending request and uses the owner command event as the primary wake, with checked message-post fallback and bounded reason-coded diagnostics. A deterministic policy harness covers coalescing and requests arriving during consumption; it does not emulate game window creation or DComp presentation.
- In a post-reboot live run, the initial scans still found no eligible HWND; a later refresh selected the game window and DComp, first-frame, input binding and Delete open/close were observed successfully without restarting the Overlay worker. This confirms eventual discovery in that run, not why an earlier run failed or universal launch timing. See [Overlay startup refresh diagnostics](research/reports/OVERLAY_STARTUP_REFRESH_DIAGNOSTICS_2026-10-17.md).
- Startup toast visibility was positively observed before Delete for about ten seconds in one live run. Other runs had successful internal lifecycle logs without a user-visible toast. Keep physical presentation and cross-launch reliability unresolved; no readiness-gate or TTL change is justified by current evidence.

## Closed, rejected and deferred research

- **Weapon/viewmodel FOV:** reference `.wvf` and `MPC_FOV.TanFOV` behavior were causally investigated, but a safe, patch-resilient UE reflection/invocation bridge for this standalone ASI is not established; production repair remains deferred.
- **Subtitle centering:** reproduced as a vanilla UI issue. Tested alignments/fixed offsets were not robust; no production subtitle hook or heuristic offset was added.
- **Graphics compatibility:** the dedicated Overlay avoids the game's Present path and G5 has a user-accepted NVIDIA HUD coexistence observation. Universal compatibility with all drivers, wrappers, Frame Generation/VRR combinations, or HDR/scRGB is not guaranteed.
- **Runtime decomposition B1–B7:** complete as an architecture campaign. Further splitting is not pursued solely to reduce file size; runtime-owned adapters remain with the single lifecycle owner where their ABI/state contracts require it.

Historical investigations and evidence are indexed in [`research/README.md`](research/README.md); consult the dated report for exact procedure and provenance rather than repeating settled experiments.
