# N01–N03 Native Runtime Safety Remediation — Implementation Report

Date: 2026-10-10  
Scope: approved sequence N02 → N01 → N03  
Runtime/game launch and ASI installation: **not performed**

## Outcome

| Stage | Status | Result |
|---|---|---|
| N02 — Zoom exception containment | **Fixed** | Zoom IN/OUT now enter through `SafeMidHookEntry`; native continuation and context are covered by production-adapter and local executable-hook fixtures. |
| N01 — SafetyHook executable lifetime | **Fixed** | Patch state distinguishes unpatched, patched, and uncertain bytes; executable storage is retained after any possible target write until process termination. |
| N03 — Startup exception containment | **Partially mitigated** | The actual `InitializeThread` entry now contains exceptions, closes readiness/correction gates, and performs at most one guarded cleanup attempt. Fault-injection tests cover this boundary; the full real-game `Initialize`/installer path is not run in an offline process. |

No Gameplay/Cinematic/Dialogue algorithms, FOV/aspect semantics, configuration, Overlay UX/localization, B1–B4 decisions, or release package contents were intentionally changed.

## Implemented

### N02 — Zoom exception containment

- Registered both Zoom hooks through the existing `SafeMidHookEntry` wrapper.
- Kept the original mid-hook continuation/context and existing telemetry/publication order.
- Did not add rollback for partially published telemetry.
- Added actual-adapter normal, unreadable-input, and injected exception cases under Diagnostic OFF and ON.
- Added a local SafetyHook executable fixture that checks the original instruction/trampoline continuation and resulting native flags.

### N01 — SafetyHook executable lifetime

- Replaced the single enabled boolean with explicit `Unpatched`, `Patched`, and `Uncertain` patch states, plus an “ever patched” lifetime marker.
- Tracked whether target-byte writing was attempted separately from whether the write completed. This distinguishes a failed byte write from a failed page-protection restoration.
- Made both page-protection restoration attempts independent; the second attempt is not short-circuited when the first fails.
- Prevented uncertain hooks from being enabled again. Disable can still attempt restoration of the original target bytes.
- Retained trampoline/stub allocations and their allocator ownership whenever target code may reference executable storage. Move, reset, failed create, disable, and destruction preserve that ownership rather than returning memory to a reusable allocator.
- The retention policy intentionally keeps executable allocations resident until process termination. This prevents use-after-free/reuse, but it is **not** a successful resource teardown and can accumulate retained executable storage if hooks are repeatedly activated and discarded during one process lifetime.
- Added fault injection through a test-build-only `VirtualProtect` wrapper and allocation-release observation. Production builds do not compile this test hook.
- Fault fixtures exercise failed enable, failed disable, post-write protection restoration failure, destruction/reset, move ownership, and the local native hook continuation in isolated child processes.

Validation limitation: the fault fixtures now prove the post-change failure handling, but a pre-change baseline failure run was not captured before editing the vendor implementation. No claim is made that an original failure was independently reproduced first.

### N03 — Startup exception containment

- Added an outer `try/catch (...)` around the actual `plugin::InitializeThread` body, including startup diagnostics, initialization invocation, and Overlay core-ready notification.
- Fatal exceptions close `stopping`/readiness, Gameplay and Cinematic correction gates, hotkey-capture state, and Overlay startup success; workers are signaled to stop.
- Failure cleanup is guarded and marked attempted before invocation. Exceptions during cleanup are contained, and the TLS marker prevents a second reset attempt if later failure handling also throws.
- Existing recoverable hook-setup exceptions keep the previous readiness behavior when cleanup completes. If cleanup throws or workers remain owned, initialization returns a failure code and readiness remains closed.
- Cleanup remains on the initialization worker thread created by `CreateThread`, not under loader lock. No Overlay shutdown API or second `RuntimeState` owner was added.
- Added compile-time test-only substitutions for the initializer and failure-cleanup calls. The harness still executes the production `InitializeThread` entry and real `ResetAllRuntimeResources` where applicable; these substitutions are absent from production builds.
- Tests cover early and partial initialization exceptions, secondary cleanup exception, and a retained worker/self-join path followed by control-thread cleanup.

Validation limitation: the controlled initializer tests exercise the real thread-entry boundary but do not execute the complete game-specific signature scan/hook installation path. That path requires live-process validation; no production fault-injection API was introduced.

## Validation

### N02 stage gate

- Focused actual-adapter matrix: PASS, ordinary and diagnostic profiles; 94 cases × Diagnostic OFF/ON per profile.
- Full `test.cmd`: PASS, exit code 0.
- Production `build.cmd`: PASS, exit code 0.
- Diagnostic build: PASS, exit code 0.
- Zoom local native continuation fixture: PASS.

### N01 stage gate

- Focused actual-adapter/native matrix: PASS, 97 cases × Diagnostic OFF/ON per profile.
- Full `test.cmd`: PASS, exit code 0.
- Production and diagnostic builds: PASS, exit code 0.
- SafetyHook fault-injection and native continuation cases: PASS.

### N03 final gate

- Focused ordinary profile: PASS, 101 cases × Diagnostic OFF/ON = 202 fresh processes.
- Focused diagnostic profile: PASS, 101 cases × Diagnostic OFF/ON = 202 fresh processes.
- Full `test.cmd`: PASS, exit code 0. It ran the actual-adapter matrix in both profiles, 404 fresh matrix processes total, plus the other project harnesses and locale/resource checks.
- Final production `build.cmd`: PASS, exit code 0.
- Final diagnostic build: PASS, exit code 0.
- Changed-code warnings: none observed. The builds still emit the existing C4201 anonymous-struct warnings from unchanged `external/safetyhook/Zydis.h` lines 5146 and 6026.
- Locale/resource audit: PASS, all 18 catalogs and embedded resources.

## Final artifact hashes

These are validation artifacts under `build-artifacts/native-runtime-evidence/2026-10-09/`, not release package contents.

| Artifact | SHA-256 |
|---|---|
| Final production ASI | `E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81` |
| Final diagnostic ASI | `CD73C138AB35618182F90F8E2F7F56B51602D42773041A99CD23E0434A9CA75B` |
| Ordinary actual-adapter harness | `B82F89AE2DB7F6BD7DA945E5CAB073827B842423C1E71E73E2BC0E700921AD32` |
| Diagnostic actual-adapter harness | `F62B167F41BF40DBAC2DC19B732D099EB04082E01E5CC133E010C82822987B50` |

## Live-runtime validation still required

- N02: run gameplay Zoom IN/OUT transitions in the game and confirm the native continuation remains visually/behaviorally unchanged under the actual game hook sites.
- N01: validate hook install, disable/reset, and process shutdown in the game. Offline fixtures cover real local executable hooks and injected protection failures, not the game’s concurrent camera callback traffic.
- N03: validate normal startup/readiness, an initialization failure with real partially installed hooks, and shutdown in the game. The complete game-specific initializer was intentionally not run by the harness.

No stage establishes live-game correctness solely from compilation; those runtime checks remain outstanding.

## Workspace note

The first N02 builds used the build script’s default project-root ASI output paths before subsequent builds were redirected to this evidence directory. Those default development outputs may have been regenerated. The `release-assets` package was not targeted or installed. Final N03 build outputs and object files were isolated under the evidence directory.
