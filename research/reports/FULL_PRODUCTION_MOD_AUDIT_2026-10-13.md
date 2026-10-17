# Full Production Mod Audit — 2026-10-13

**Project:** S.T.A.L.K.E.R. 2 Camera Tweaks  
**Phase:** F1 — audit for final release hardening and maintenance handoff  
**Method:** independent static review of the current working tree, with existing evidence reused within its recorded scope.  
**Disposition:** audit complete; remediation is proposed, not implemented.

## Executive assessment

The Overlay-owned HWND migration remains accepted at G0–G5. This audit finds no reason to discard that architecture, split `runtime.cpp` again, replace the camera algorithms, or restart the graphics experiments. The single runtime owner and single graphics owner are useful boundaries. The important remaining work is to make their failure, input and persistence contracts reliable.

The deduplicated matrix contains **15 findings: 10 source-confirmed defects and 5 evidence-backed risks**. There are **no confirmed Critical findings**. Three findings are High: two source-confirmed safety defects and the already-open native callback/reset risk, now attached to concrete current call paths. The other findings comprise nine Medium and three Low items. A source-confirmed defect means that the code supplies a concrete violating path or interleaving; it does not mean that a new crash or fault was reproduced during this read-only audit.

The highest priorities are:

1. Exceptions in optional Overlay work can still terminate the game because some throwing work occurs inside `noexcept` functions, outside their effective exception boundary.
2. The canonical test runner does not guarantee process-tree containment or verified cleanup on timeout/cancellation. This matters directly to the user's ability to keep using the PC.
3. Controlled runtime reset disarms entry flags and joins mod workers but does not establish quiescence of an already-admitted engine callback. This is the existing N05 boundary, not a new claim that ordinary process exit or G5 failed.
4. INI updates can overwrite a partially read file, and concurrent Overlay/hotkey changes can persist an older accepted value after a newer one.
5. Queued key rebinding uses modifier state at owner-processing time instead of event time.

No new deterministic defect was established in the HorPlus formula, the optical Dialogue formulas, the current Cinematic aspect-policy selection, native instruction signatures, or the ordinary DComp alpha path. No measured new performance regression was established. Existing startup pauses, driver completion, engine object lifetime, Arabic shaping and broad graphics compatibility limits remain explicit; none is silently converted into a PASS or into a newly proved defect.

The recommended release strategy is bounded remediation of the findings below, focused checks for each changed contract, then one consolidated final qualification run and a newly identity-checked archive. There is no recommendation for another general architecture campaign.

## 1. Baseline, provenance and execution limits

- Repository: `E:/Work/Slaker2 mods/01-Projects/STALKER-2-Ultrawide-Fix-for-UE-5.5.4`.
- Git HEAD: `e756442bcf9942923b4531fb513af893f6aecae7`.
- Subject: the **current working tree**, including the existing staged migration changes, not HEAD alone.
- Source inventory: **143 files in `src/`**. The explicit production registry selects **61 translation units: 54 project-owned and 7 vendor**. Research language watchers and conditionally excluded experimental branches are not treated as ordinary ASI inputs.
- Canonical tests: **59 harness targets / 62 ordered invocations**, 102 actual-adapter cases with Diagnostics OFF/ON = **204 child processes**, plus **four** N06-M01 Zoom invocations. Those are different counting units.
- The ordinary ASI contains passive diagnostics; `[Diagnostics] Enabled` is the user-facing switch. A diagnostic-named compatibility wrapper is not a second required user build.
- Input snapshot: 379 files from `src`, `tools/build`, `tools/generate_ini_documentation.ps1`, `tests`, `locales`, SafetyHook, ImGui and spdlog headers, plus `CMakeLists.txt`, `build.cmd`, `test.cmd`, `.gitattributes`. Sorted records are `relative-path|length|SHA-256`, joined with LF and hashed as UTF-8. Aggregate SHA-256: `5AD380CB16A09944A92615EC93303A109F078BBC05ACE4BFFFA45AD4B98B6413`.

| Evidence subject | SHA-256 / scope |
| --- | --- |
| `src/plugin/runtime.cpp` | `3E9C4EAF512F551C145432C90D6A83AC5D6708705D91EE59C8CA53AA27F1F7F1` |
| `src/overlay/composition/composition_runtime.cpp` | `C33E17D86D141D7421515C42BE30F1066DDF8ECB929F0938EEF23726D47A2BF3` |
| `src/overlay/presenter/renderer_runtime.cpp` | `D9C2A85F2FFAE18981892503102EDB77C41835CF55CFEF58693E75A1FFDC763D` |
| `CMakeLists.txt` | `4FFD76CC1BE026824F76ACF1C825C4CA1E163249E752DCF2B7D2248FD47C9EAF` |
| `tests/test_harnesses.json` | `0A2EC747412DD8D6497DC95F5DADA583FBD8E55795C02E1147ADE9870DE61522` |
| G2/G3 candidate used for G5 | `123D792A61A550892141DBEC12BB059A606BEA6A30B0C8B300491B6CAFC7DEF2` |
| Verified G5 rollback | `C7287C8C52233EC5B8F8E32A0DBC6C38209DDA15F9C07FE99C505CA4FDC75C7C` |
| Current loose `release-assets/STALKER2CameraTweaks.asi` | `3C6F95F75D36B04780740D82783D586E821C1B8AFFABCE4463F12CDEBFCDE4E8` |
| ASI inside existing v3.1.0 ZIP | `FFB03F35D0C04A5A609FCB47B3A9024E2DA268FA40BB431186903233498E545A` |
| Existing v3.1.0 ZIP itself | `5045B7B33C6492CE4FF1D316F47358A907B5DB9F932509199203EF68815516C2` |

The different release/staging identities are recorded for final packaging. Existing archives are historical artifacts; their difference from the new G5 candidate is not itself archive corruption. The future final archive must be made from its intended, qualified artifact, rather than assuming that the loose release-assets file or an old ZIP represents the current source.

No game, ETW/WPR/PresentMon recording, native/graphics fixture, test suite, compiler, CMake configure or build was launched. Work used finite local file reads, bounded searches and small-file hashing/archive-directory inspection. Inspection shells returned; no background analysis process or capture session was created. The installed ASI, rollback, source, configuration and existing documentation were not modified. This newly created report is the requested deliverable exception to the read-only restriction.

## 2. Actual production and ownership inventory

| Production group | Project TUs | Owner / boundary reviewed |
| --- | ---: | --- |
| Plugin integration | 5 | `RuntimeState`, DLL entry, settings API, worker handles, feature status |
| Configuration | 3 | configuration repository, managed template, parsers, serialized replacement |
| Diagnostics | 4 | runtime gate, continuation/provenance storage, performance observations |
| Hook infrastructure | 3 | signature scanner, instruction validation, hook-set integration |
| Gameplay | 4 | aspect policy, writer resolution, mode transitions, HorPlus/recovery policy |
| Cinematics | 4 | selection, aspect store, association/recovery helpers, transactional installation |
| Dialogue | 2 | optical transforms, candidate/active/exit/rearm state |
| Camera observations | 6 | FOV publication, baseline/restoration stores, snapshots and presentation |
| Win32 platform | 4 | memory checks, hashes, window selection, client viewport |
| Overlay presentation | 6 | typed status projection, read-only camera panel, selectors, layout/placement |
| Overlay localization | 7 | catalogs, placeholders, validation, fonts, keys, native Auto language reader |
| Overlay composition | 1 | display HWND, GameHwnd subclass, events, command queue, clock worker/finalizer |
| Overlay presenter | 4 | surface generations, notification state, Renderer, private D3D11 pipeline |
| Overlay input | 1 | terminal admission, bounded command/event queues, rebind/cursor state |
| Vendor integration | 7 | SafetyHook + Zydis, five ImGui core/platform TUs; local lifetime patch and ABI use |

The definitive file list is `tools/build/production_sources.json`. Inline production transactions and policies in headers were included in the review, particularly `camera_writer_transaction.hpp`, `cinematic_lifecycle_transaction.hpp`, `dialogue_boundary_transaction.hpp`, `camera_correction_gate.hpp`, `camera_state_publication.hpp`, the Cinematic recovery/EXIT helpers and the Overlay input/presenter/finalizer contracts. Vendor review covered the bundled hook relocation/lifetime integration, the local N01 patch, ImGui backend ownership and use; it does not certify every possible instruction understood by the bundled decoder or every upstream ImGui path.

Critical paths traced:

- DLL attach → initializer → config/log/hash → component installation → settings publication → workers → Overlay readiness, including both local feature failures and fatal cleanup.
- Game writer/ENTER/EXIT/aspect/Dialogue/Zoom boundary → correction gate/domain transaction → native register/store output → snapshot publication → optional visible Overlay wake.
- Game WndProc → terminal admission → bounded command/event handoff → owner application/persistence → committed display readiness → fail-open revocation.
- Resize/DPI/focus/rebind → surface generation and display placement → notification lifetime → idle root detach.
- Terminal request → admitted-callback drain → clock/timer/subclass cleanup → Renderer/COM release → owner-thread display destruction → retained/unverified accounting.
- Build registry → generated INI/RC inputs → isolated objects → ASI publication; test registry → build → serial process runner → adapter child matrix.

## 3. Deduplicated findings matrix

Severity reflects consequence, not probability. Confidence describes evidence for the stated mechanism, not proof that it caused an earlier incident.

| ID | Subsystem | Severity | Classification | Confidence | Finding | Release disposition |
| --- | --- | --- | --- | --- | --- | --- |
| FP01 | Overlay exception containment | High | Confirmed defect | High, source | Throwing optional work inside `noexcept` paths can call `std::terminate` | Fix before final release |
| FP02 | Test process ownership | High | Confirmed defect | High, source | Timeout/cancellation does not contain and verify the complete owned process tree | Fix before qualification |
| FP03 | Native reset/callback lifecycle | High | Evidence-backed risk | High for missing barrier; runtime schedule unverified | Reset can overlap a callback admitted before stopping | Resolve supported reset boundary; N05 remains open |
| FP04 | Startup/readiness | Medium | Confirmed defect | High, source | Inner failure paths can return success or reopen completion after cleanup | Fix |
| FP05 | INI data preservation | Medium | Confirmed defect | High, source | Partial read can be committed as a replacement INI | Fix |
| FP06 | Runtime settings/persistence | Medium | Confirmed defect | High, explicit interleaving | Apply order and persistence order can disagree across producers | Fix |
| FP07 | Input command semantics | Medium | Confirmed defect | High, event/consumer comparison | Rebind modifier state is sampled after the original key event | Fix |
| FP08 | CMake generated dependencies | Medium | Confirmed defect | High, target properties | Non-adapter harnesses include a hardcoded canonical generated directory | Fix |
| FP09 | Test artifact isolation | Medium | Evidence-backed risk | High for shared path | Distinct CMake trees publish harnesses to the same executable directory | Fix alongside FP08 |
| FP10 | Hashing resource ownership | Medium | Confirmed defect | High, exception path | Raw file/crypto handles leak if allocation throws | Fix with RAII |
| FP11 | Camera/Overlay HWND interaction | Medium | Evidence-backed risk | High for eligibility; visible error unverified | Camera viewport fallback can select the unowned Overlay display HWND | Narrow selector repair |
| FP14 | Native language reader concurrency | Medium | Evidence-backed risk | High for unsynchronized singleton | Overlapping old/new HWND threads can race initial reader publication | Narrow nonblocking publication repair |
| FP12 | Persistence UX | Low | Confirmed defect | High, source | Ordinary settings save failure is only logged, while UI reports acceptance | Polish or explicitly accept |
| FP15 | Placement persistence | Low | Confirmed defect | High, deterministic trace | Duplicate Overlay sections read last but placement writes first | Fix with focused config check |
| FP13 | Subclass continuation storage | Low | Evidence-backed risk | High for retention; growth impact bounded by rebind count | Successful bindings have no reclamation/budget after repeated rebinds | Bound/document residency; preserve continuation safety |

### FP01 — Optional Overlay exceptions can terminate the process

- **Location:** `src/overlay/composition/composition_runtime.cpp:1174`, `Worker`; startup calls at 1182–1194, command dispatch at 1241 and the unguarded log at 1312. `StartFrameClock` at 193 is also `noexcept` and calls allocating log expressions. `src/overlay/presenter/renderer_runtime.cpp:892`, `ApplyWindowDpi`, invokes font rebuild/log construction inside `noexcept`; `EnsureStartupNotification` at 1478 and `PrepareStartupNotification` at 1501 call `Log` from their catch handlers at 1497/1535.
- **Trigger:** an allocation, string construction or logger callback throws on one of these optional paths. This can happen during startup, DPI/font changes, a command or an error-reporting path.
- **Mechanism/consequence:** `RunOptionalOverlayWork` can contain a throwing operation only if the exception reaches it. Throwing through an intervening `noexcept` function instead calls `std::terminate`; the worker has no encompassing catch/finalizer boundary. An optional UI failure can therefore terminate the whole game. The catch-handler logs can themselves throw, defeating the original containment.
- **Coverage/limit:** policy/finalizer fixtures and the G1 successful rendering runs do not inject exceptions into the actual `Worker` entry or these leaf methods. G4 compilation and PASS do not establish this failure contract.
- **Minimal fix:** put one effective owner-thread exception boundary around worker work, route failure through terminal admission and the existing owner finalizer, make failure logging nonthrowing, and remove or locally contain inappropriate `noexcept` on throwing internals. Preserve the single graphics owner.
- **Regression/validation:** Medium integration risk. Inject a throwing logger using the existing Renderer logger seam and a controlled allocation failure in an isolated process; assert survival, terminal fail-open state and truthful cleanup outcome. A caller-side catch alone is insufficient.
- **Relationship:** independent of N02's fixed camera hook wrapper and N03's initializer boundary; it affects the separate Overlay worker. This is new concrete evidence outside G5's normal-path observations.

### FP02 — Canonical tests do not guarantee process-tree cleanup

- **Location:** `tests/runner/test_harness_runner.ps1:45`–64, process launch/wait/kill; `tests/runner/test_cmd_exit_code_contract.ps1:21`–43, `Invoke-BatchFixture` and its unbounded `WaitForExit`; its runner timeout also kills only its immediate process. Adapter descendants are created by `tests/plugin/runtime_native_adapter_harness.cpp:1201`–1224; termination waits there are not checked. `test.cmd` has no enclosing process-tree supervisor.
- **Trigger:** an adapter parent times out/crashes, the runner is cancelled, a launch/wait/kill raises an exception, or an exit-contract child stops making progress.
- **Mechanism/consequence:** PowerShell's parameterless `Process.Kill()` here targets the immediate process, not descendants. The 5-second post-kill wait result is discarded. There is no `finally` that owns and verifies the tree on every exit. A native child can outlive its controller, or an exit fixture can wait indefinitely. Such a leftover test can continue consuming resources; the audit does not claim it caused the user's earlier multi-day degradation.
- **Coverage/limit:** the status-contract fixture checks return codes and a sleeping leaf process. It does not spawn a stubborn grandchild and verify its disappearance on parent timeout, cancellation and supervisor exception. Adapter-local 10-second deadlines are useful while their parent remains alive, but do not replace external ownership.
- **Minimal fix:** supervise each owned tree with a Windows Job Object or equivalent tracked, bounded tree supervisor; use kill-on-close/cancellation cleanup, bounded graceful and forced steps, then check actual exit. Bound the exit-contract fixture's waits. Keep test execution serial and apply a finite build/test budget at the entry point.
- **Regression/validation:** Low runtime-mod risk, Medium tooling risk. A small parent/grandchild fixture should cover timeout, crash and cancellation; inspect owned PIDs after each case. Do not stop unrelated applications or recording sessions.
- **Relationship:** signed nonzero handling was already fixed by Hybrid H0 and remains correct. This is a different contract: returning FAIL is not proof of cleanup. It contradicts the current mandatory project containment rule.

### FP03 — Controlled reset has no engine-callback quiescence barrier

- **Location:** `src/plugin/runtime.cpp:751`–795, `ResetAllRuntimeResources`; `ReplayManualTransition:4537`–4539 and `TraceCinematicEnter:3090`–3092 check entry flags before acquiring `CameraCorrectionGate`; hook resets are at 778–789. `WorkerLifecycle::StopAndJoin`, `src/plugin/worker_lifecycle.cpp:63`, joins only owned worker handles.
- **Trigger:** supported failure cleanup or an explicit controlled reset overlaps an engine callback that already passed its flag check. Normal dynamic unload is not a supported trigger and is not being added to scope.
- **Mechanism/consequence:** a callback can pass the entry check, pause, then continue or acquire the correction gate after reset has closed flags and begun clearing state/removing hooks. Reset itself does not take that gate or wait for camera callback admission. It can therefore be followed by a late camera mutation or inconsistent lifecycle publication. Executable retention protects trampoline storage; it does not prove completion of active callbacks.
- **Coverage/limit:** worker join and serialized native fixtures do not exercise unconstrained concurrent engine callback teardown. Existing N05 explicitly records this gap. Ordinary process termination and G5 remain accepted within their scope; no live race was reproduced here.
- **Minimal fix:** define and enforce the supported reset admission/drain contract at the existing runtime owner. Recheck stopping/gates after acquiring the correction gate and serialize state reset against admitted mutation. Handle the self-callback case explicitly; do not join or tear down under loader lock.
- **Regression/validation:** High lifecycle risk. Use a deterministic callback latch: pass the entry check, pause before gate acquisition, request reset, then resume. Assert no late correction/republished readiness and preserved continuation. An isolated native hook fixture can test the bounded removal contract after the state test; it is not a license to unload the mod in game.
- **Relationship:** concrete current-path refinement of **N05 Open**, not a duplicate new gate or a reversal of N01 Fixed. Retained executable memory remains an accepted safety policy.

### FP04 — Startup failure and success/readiness are not consistently distinguished

- **Location:** `src/plugin/runtime.cpp:6359`–6369, logger setup catch returns `0`; inner initializer catches at 6599–6615 can finish cleanup and fall through to `runtimeInitializationComplete=true` and `return 0` at 6616–6617. `InitializeThread:6743`–6744 publishes Overlay camera-core readiness when Overlay startup succeeded, without testing the core outcome/stopping state.
- **Trigger:** logger setup fails, or an exception reaches an inner `Initialize` catch and cleanup succeeds. The outer fatal handler is not entered because the exception was already consumed.
- **Mechanism/consequence:** an actual initialization failure can look successful to the caller. In the inner-catch case, reset closed completion/stopping, then the initializer reopens completion and publishes the camera-core-ready event despite stopping remaining set and resources having been reset. Settings/UI availability and lifecycle evidence can consequently disagree with the real core state. Logger failure also silently skips functional initialization; narrow `logPath.string()` is an additional Unicode-path compatibility risk for that branch.
- **Coverage/limit:** startup adapter fault cases replace the initializer call with a throwing fixture (`runtime_native_adapter_harness.cpp:179`–187). They exercise the real outer entry handler, not these inner production catches. This is the documented N03/N04 evidence boundary.
- **Minimal fix:** return an explicit failure outcome from every failed inner path, keep readiness closed after failed initialization, and publish camera-core-ready only for the intended successful/available runtime outcome. Decide explicitly whether unavailable file logging should use a safe fallback or fail initialization; do not silently call it success. Use a wide-safe path if file logging is retained.
- **Regression/validation:** Low/Medium startup risk. Exercise inner cleanup-success and logger-unavailable branches, separately from outer throws; assert result, stopping/completion flags, worker state and absence of a false CAMERA_CORE_READY publication.
- **Relationship:** N03 remains **Partially mitigated**, N04 remains **Open**. The current outer exception boundary is useful and should stay.

### FP05 — A partially read INI can replace the complete original

- **Location:** `src/config/config_repository.cpp:410`–424, `PersistConfigValue`; `src/config/config_template.cpp:567`–580, `SynchronizeManagedConfigTemplateBody`; later replacement at repository 476/template 1058. The repository's separate `ReadLines` helper already returns `!input.bad()`.
- **Trigger:** input I/O fails after some lines have been read, before EOF.
- **Mechanism/consequence:** both update paths treat loop termination as a complete read, close the stream and build replacement content from the partial vector. Staging/flush/atomic rename can all succeed while the unknown tail is lost. Atomic writing does not protect the source from a failed read. This contradicts the last-known-good INI preservation invariant.
- **Coverage/limit:** `config_persistence_harness` covers staging and replacement failures and normal duplicate-key updates. It does not inject a mid-read failure before rewrite. The loader also lacks an end-of-read failure check, so partial configuration can be accepted.
- **Minimal fix:** check read completion before modifying/staging, reuse one checked line-read helper, and return failure without replacing the file on a read error. Distinguish EOF from I/O failure.
- **Regression/validation:** Low risk. Inject an input stream failure after a known prefix; assert original bytes and unmanaged tail remain unchanged for both template synchronization and value persistence. This needs a bounded I/O seam, not a graphics test.
- **Relationship:** previous atomic replacement fixes are retained; this is the missing earlier stage of the same preservation contract.

### FP06 — Concurrent Apply/Persist can save an older setting last

- **Location:** `src/overlay/presenter/renderer_runtime.cpp:663`–674, `ApplySetting`; `src/plugin/runtime.cpp:6203`–6217 and equivalent Dialogue/Cinematic hotkey paths; `PersistRuntimeSetting:6159`–6168; `src/config/config_repository.cpp:414`, per-write persistence mutex.
- **Trigger:** Hotkey worker and Overlay owner change the same setting with overlapping Apply/Persist operations.
- **Mechanism/consequence:** a valid schedule is H.Apply(A) → O.Apply(B) → O.Persist(B) → H.Persist(A). Camera application is serialized by the correction gate and file writes by the persistence mutex, but nothing connects the order of those two phases. Runtime ends at B and the INI at A; next startup restores the older value. This is not a file corruption race and is not fixed by making each replacement atomic.
- **Coverage/limit:** settings API tests cover type checks, publication and invocation; camera-setting tests cover the correction gate. Config tests serialize file mutations. None asserts one accepted mutation order across two producers and subsequent reload.
- **Minimal fix:** provide one runtime-owned mutation/persistence ordering contract, using a transaction/ticket or serialization across the complete logical action. Release the camera gate before file I/O. Every producer must use the same contract; avoid independent fixes in GUI and HotkeyLoop.
- **Regression/validation:** Medium API risk. A two-producer latch test must force the schedule above and verify that persisted/reloaded state matches the last accepted effective setting. Include a persistence failure without falsely acknowledging a saved value.
- **Relationship:** respects the B2 Remain decision and existing RuntimeState ownership. It does not require another source split or a second settings owner. FP12 concerns the visible failure report, not ordering.

### FP07 — Queued rebinding uses the wrong moment's modifier state

- **Location:** `src/overlay/composition/composition_runtime.cpp:901`–918, game callback checks modifiers and enqueues; `ProcessOverlayWindowMessage:543`–550 rechecks `GetAsyncKeyState` on the owner. `OwnerInputCommand`, `src/overlay/input/input_state.hpp:40`, carries no modifier snapshot.
- **Trigger:** Ctrl/Shift/Alt state changes between the key event and owner processing, for example while the owner is busy applying/persisting a previous command.
- **Mechanism/consequence:** Ctrl+F6 can be passed to the game at event time, then become an accepted plain-F6 rebind after Ctrl is released. The converse can consume a plain key at event time and later reject it because a modifier was pressed. The original event and the accepted action no longer agree.
- **Coverage/limit:** `input_state_harness.cpp:717`–723 verifies rejection when a supplied modifier flag is true. It does not delay the production owner command while modifier state changes. General keyboard pass-through is intentional and is not being reported as a defect.
- **Minimal fix:** capture the bounded modifier state needed for rebinding in the admitted Game WndProc event and carry it in the owner command. Use that snapshot for owner-side rebind decisions. Preserve native continuation and the existing accepted-command order.
- **Regression/validation:** Low/Medium input risk. Test both modifier-release and modifier-press schedules, including native consumption and captured key-up state. Avoid changing the agreed general keyboard forwarding policy.
- **Relationship:** a semantic gap in the otherwise accepted G1-A asynchronous command boundary; G1 normal-path PASS remains valid.

### FP08 — Alternate CMake trees can include a stale generated INI header

- **Location:** `CMakeLists.txt:96`–111 creates the header in the current binary tree; 273 computes the test-generated include path from the fixed registry directory; `configure_test_harness_target:310`–313 uses that path. `tests/test_harnesses.json` sets `generated_include_directory` to `build-artifacts/cm/test-suite/generated`.
- **Trigger:** build a non-adapter harness using `config_template.cpp`, such as `config_persistence_harness`, from a CMake binary directory other than the canonical test-suite tree.
- **Mechanism/consequence:** the target depends on the newly generated header in its own tree but searches the hardcoded canonical tree for `#include "canonical_ini_documentation.hpp"`. A clean alternative tree can fail; an existing canonical header can silently supply stale content. Actual-adapter dependencies separately include the current generated directory, which explains why their candidate CTest path can pass while this general harness contract is wrong.
- **Coverage/limit:** the canonical test tree happens to match the hardcoded path. The five CTest entries build/run only actual adapters and Zoom fixtures; they do not cover the non-adapter alternate-tree case.
- **Minimal fix:** use the current binary tree's `generated_directory` for every harness and keep the output dependency and include path aligned. Treat registry paths as a canonical entry-point default, not a target's actual generated location.
- **Regression/validation:** Low build risk. Compile the config harness in a fresh alternate directory with no canonical generated header; then alter the generator input and verify only the intended generated header is consumed.
- **Relationship:** does not invalidate the earlier `/showIncludes` and missing-output dependency fixes or the canonical G4 run.

### FP09 — Test executables are shared across otherwise distinct CMake trees

- **Location:** `CMakeLists.txt:272`, 307–309 and 376–379; registry `output_directory=build-artifacts/tests`. `tools/build/test-cmake-candidate.cmd` uses its own candidate binary tree but the same executable destination.
- **Trigger:** separate test/candidate trees or configurations build/use harnesses in succession or overlap, especially while one runner is executing a previously built target.
- **Mechanism/consequence:** object files are isolated, executable paths are not. A runner or CTest target can execute the binary last written by another tree; simultaneous linking can also fail against a running executable. A successful configure/build in one tree does not pin the bytes later run from the shared destination.
- **Coverage/limit:** current serial canonical use is normally unaffected. Registry inventory parity and source equality do not establish executable identity isolation.
- **Minimal fix:** put harness/support outputs inside their owning binary tree and pass that directory/manifest to the existing runner. Keep `test.cmd` canonical and execution serial. Until repaired, reject overlapping builds/runs sharing the directory.
- **Regression/validation:** Low/Medium runner path risk. Two distinct trees must have distinct executable paths; verify one tree's rebuild cannot change the other's executable hash or execution subject.
- **Relationship:** complementary to FP08; separately concerns final outputs, not header dependencies.

### FP10 — Hashing leaks raw handles on an allocation exception

- **Location:** `src/platform/win32/sha256.cpp:12`–55, `ComputeSha256`.
- **Trigger:** allocation throws after `CreateFileW`, during the 1 MiB buffer/hash-object allocation, or during result string reserve/append after crypto handles were created.
- **Mechanism/consequence:** file, algorithm and hash handles are cleaned only at the normal function tail. Stack unwinding skips it. Initialization can catch the exception elsewhere without owning these handles. This leak is within the game process; it does not establish a resource leak surviving game-process exit or explain the user's system-wide degradation.
- **Coverage/limit:** normal SHA identity reads work. There is no canonical allocation-fault ownership test for this helper.
- **Minimal fix:** acquire each raw handle into RAII immediately and keep cleanup independent of allocation/result construction.
- **Regression/validation:** Low risk. Inject failures at buffer allocation, hash-object resize and digest formatting in an isolated process; verify handle/resource accounting and normal hashes.
- **Relationship:** can trigger FP04's startup failure branch. It is separate from SafetyHook executable retention, which is intentional.

### FP11 — Camera viewport fallback can choose the display HWND

- **Location:** `src/platform/win32/window.cpp:5`–14 returns the first visible unowned process window; `viewport.cpp:16`–35 uses it when the foreground HWND is not in the game process. The new display is created unowned in `composition_runtime.cpp:289`–296. Camera callers include `runtime.cpp:4579` for RuntimeCombine and 6144 for the semantic viewport.
- **Trigger:** foreground switches outside the game while the display remains visible pending owner reconciliation, or a host resize/rebind overlaps that transition.
- **Mechanism/consequence:** the composition discovery code excludes the display class, but this separate platform selector does not. The display is a valid candidate, often above the game in z-order. Its client rectangle normally mirrors the host, so a wrong visible ratio is not established in ordinary steady state; during lagging geometry/rebind it can be an old ratio. Camera correction and its potential assessment can then use a display window rather than the authoritative game viewport.
- **Coverage/limit:** `tests/platform/viewport_harness.cpp` tests usable dimensions only. G3 discovery self-exclusion does not exercise this platform fallback.
- **Minimal fix:** exclude the dedicated display class/tool window from this selector and choose a usable host candidate consistently. Keep camera viewport discovery working with Overlay disabled; do not make camera correction depend on the graphics owner.
- **Regression/validation:** Low risk. A small HWND fixture with a game surrogate plus a larger/older unowned display should return the game candidate in foreground and fallback paths, including a transient aspect change.
- **Relationship:** cross-subsystem gap left outside the G3 composition discovery repair; it does not undo that repair.

### FP12 — Ordinary settings persistence failure is invisible to the user

- **Location:** `src/overlay/presenter/renderer_runtime.cpp:663`–674, `ApplySetting`; it logs `OVERLAY_SETTING_PERSIST_FAILED` and still returns true. Hotkey setting notices at `runtime.cpp:6208`–6222 are published before persistence completes. The rebind path at composition 590–599 already has a useful persisted/error distinction.
- **Trigger:** runtime mutation succeeds but INI replacement fails because of a locked/unwritable file or storage problem.
- **Mechanism/consequence:** active state changes correctly, but the user receives no ordinary UI warning that it is unsaved. Settings appear successfully changed under the documented automatic-save behavior and revert at restart.
- **Coverage/limit:** persistence failure preservation is tested; the ordinary user's visible feedback is not. A log marker alone is not an interaction-level assertion.
- **Minimal fix:** expose an accepted-but-unsaved result/toast using the existing notification infrastructure. Keep the effective setting active and distinguish it from persistence success; do not silently roll back it or claim a saved value.
- **Regression/validation:** Low risk. Inject persistence failure and verify the effective setting/readback, unchanged INI and one visible unsaved warning; then verify successful saves do not generate an error notice.
- **Relationship:** UX part of final polish; related to FP06 but independently repairable.

### FP13 — Continuation records have unbudgeted process-lifetime growth

- **Location:** `src/overlay/composition/composition_runtime.cpp:981` allocates `InputWindowBinding`; only failed-install branches at 989/997 delete it. `ForgetInputWindow:226`–231 removes the property but does not retire storage; successful removal/rebind at 1013–1051 likewise has no reclamation registry.
- **Trigger:** many successful host rebind/install/remove cycles in one process.
- **Mechanism/consequence:** each successful installation retains another immutable continuation record. This avoids dangling saved continuations and queued pointers, particularly under a foreign subclass, and is safer than immediate deletion. However, there is no explicit cap or eventual accounting when a window is destroyed. The per-record growth is small and no material memory impact was measured.
- **Coverage/limit:** stale/foreign continuation fixtures verify safety, not accumulated retained-record count after repeated rebinds. `COMPLETE` Renderer cleanup is not a statement that every process-resident continuation record was reclaimed.
- **Minimal fix:** either document and enforce a bounded resident record budget or implement deferred reclamation only after callback/command references and foreign chains are proven gone. Prefer a bounded stable pool to unsafe early deletion if reclamation cannot be established.
- **Regression/validation:** Medium risk if changing reclamation, Low if budgeting. Repeat bounded rebinds with stale queued commands and a foreign subclass; ensure stable continuation, budget behavior and no use-after-free.
- **Relationship:** preserves G1's accepted continuation-lifetime safety. This is a resource-budget risk, not a request to remove that safety policy.

### FP14 — Native Auto-language singleton publication can race during rebind

- **Location:** `src/overlay/localization/game_language_reader.cpp:35`–37, plain `g_reader`, `g_free`, `g_initialized`; `InitializeOnce:105`–117. Game callbacks execute the reader at composition 879–884. The command/input architecture expressly permits overlapping old/new HWND producers during rebind.
- **Trigger:** the first valid language synchronization overlaps callbacks on old and new GameHwnd threads. Same-thread steady-state use does not trigger it.
- **Mechanism/consequence:** two native reader calls can read/write the singleton fields without a common lock or atomic publication. The fact that both would normally assign the same validated function addresses does not remove the C++ data race. A partially published reader/free pair or undefined behavior is possible; neither was observed in the game.
- **Coverage/limit:** current locale synchronization evidence validates the identified game layout and ordinary locale sequence. It does not establish concurrent singleton initialization across rebind threads.
- **Minimal fix:** publish an immutable validated reader/free bundle once with a nonblocking initialization state. A callback seeing initialization in progress should fall back safely; do not make Game WndProc wait on the graphics owner or introduce a blocking `call_once` path indiscriminately. Keep the actual game calls on the game-window thread.
- **Regression/validation:** Low/Medium risk. Concurrent fake/native-reader publication fixture must observe either a complete pair or a safe fallback, never a half-published pair; existing game-layout validation must remain unchanged.
- **Relationship:** distinct from the already-reviewed bounded FString copy/free and startup Auto-locale delivery.

### FP15 — Duplicate Overlay sections defeat placement persistence

- **Location:** `src/config/config_repository.cpp:55`–76, `ReadValue` retains the last matching section/key; `SetValue:98`–144 stops at the end of the first matching section. `PersistOverlayPlacement:517`–526 uses that first-section writer.
- **Trigger:** an INI contains two `[Overlay]` sections with `PositionX`/`PositionY`, a case not rejected by the reader or template synchronizer.
- **Mechanism/consequence:** with first PositionX=30 and last PositionX=60, loading returns 60. Saving 100 updates the first occurrence but leaves the last at 60. Save returns true, then reload returns 60. The general `PersistConfigValue` path already updates repeated occurrences, so placement uses inconsistent duplicate semantics.
- **Coverage/limit:** `config_persistence_harness.cpp:392`–404 covers repeated Gameplay sections for the general writer. Placement tests exercise one Overlay section and migration/replacement failure, not repeated-section reload.
- **Minimal fix:** make placement's multi-key update converge all recognized occurrences, or normalize repeated managed sections in one atomic update while retaining unmanaged content. Keep migration one-shot and non-destructive.
- **Regression/validation:** Low risk. Two Overlay sections, duplicate coordinates, custom tail and migration marker should save/reload the new pair consistently and remain idempotent.
- **Relationship:** independent of FP05's read-failure protection; both can share the same checked configuration-update helper.

## 4. Cross-subsystem interaction review

| Interaction | Established useful boundary | Remaining issue / decision |
| --- | --- | --- |
| Camera callbacks ↔ runtime settings | CameraCorrectionGate serializes mutations; owned domain stores have locks or atomics | FP03 reset admission; FP06 accepted/persisted ordering occurs outside the camera gate |
| Game WndProc ↔ Overlay owner | Bounded command queue, try-only event bridge, terminal fail-open and heartbeat; native continuation retained | FP07 event-time modifier semantics; FP14 reader publication; FP13 retained record budget |
| Overlay HWND ↔ camera viewport | Overlay's own discovery excludes its display class | FP11 separate platform selector still includes that HWND |
| Optional UI ↔ camera core | No game DXGI/backbuffer/device ownership; core generally survives Overlay disable | FP01 process termination defeats optionality; FP04 readiness can falsely follow failed initialization |
| Accepted settings ↔ disk ↔ restart | Atomic staged writes preserve original on write/replacement failure | FP05 failed reads; FP06 reverse save order; FP15 placement duplicates; FP12 unsaved feedback |
| Diagnostics ↔ native callbacks | Runtime OFF gates, bounded trace buffers, semantic/change-based output; deferred logs run after correction-gate release | No new measured hot-path defect; ordinary lifecycle logging remains synchronous and can still cost time outside the gate |
| DComp owner ↔ device recovery/finalization | One owner, explicit surface generations, bounded recreation, retained state on uncertain detach | G5 does not prove driver completion; owner waits/driver calls cannot be assigned the earlier startup pause from current evidence |
| Build graph ↔ registry ↔ runner | Explicit 61-TU registry, shared actual-adapter objects, no duplicate runtime TU, canonical serial runner | FP08 generated include path; FP09 output identity; FP02 resource containment |
| Candidate ↔ staging ↔ final archive | Exact G5 candidate and rollback identified | Existing loose release asset, existing v3.1 ZIP and G5 candidate are different files; final qualification must pin one intended artifact |

No new reciprocal lock cycle was established among camera gate, Dialogue mutex, camera-state mutex, baseline/observation locks and the graphics owner. Game WndProc does not acquire `g_rendererMutex` or perform settings persistence. This is meaningful improvement from G1. It does not make throwing `noexcept` work safe or establish callback quiescence during controlled native reset.

The raw mouse path deliberately forwards only relative movement from `WM_INPUT`; zero-delta, absolute and non-mouse packets pass through. Legacy button/wheel messages provide the menu interaction in the observed session. A raw-only/no-legacy button-wheel configuration is not proved supported by these tests; no defect is assigned to the actual game's unobserved registration flags. Similarly, ordinary keyboard forwarding is intentional according to `input_state_harness` and is not treated as unwanted capture leakage.

## 5. Existing evidence, accepted limits and hypotheses

These records are not counted again as new confirmed defects:

| Record / topic | Current disposition retained | Final-release treatment |
| --- | --- | --- |
| G0–G5 | Migration complete; G5 user-accepted for `123D…DEF2` | Do not repeat without a concrete defect; changes in remediation need only their affected checks and final qualification |
| N01 | Fixed within fault-injection/retention evidence | Executable bundle/allocator retention is intentional; do not report it as successful native teardown or remove it to make leak counts look clean |
| N02 | Fixed; SafeMidHookEntry plus N06-M01 | Preserve normal/exception native continuation checks |
| N03 | Partially mitigated | FP04 supplies concrete inner-initializer follow-up; retain outer containment |
| N04 | Open, actual production startup/install/failure orchestration limits | Focus on named changed failure branches; helper PASS is not full installer proof |
| N05 | Open | FP03; normal dynamic unload remains unsupported |
| N06 / M01 | Parent Partially verified/Open; M01 complete within offline scope | Seven current-image hook sites and game-derived Zoom replay are useful evidence, not full engine-consumer/lifetime proof |
| Historical E349 identity | Pending in retained historical records | Do not require recreating/testing an obsolete E349 file for the new release. Qualify the final current artifact and keep historical identities distinct |
| Startup pause / graphics stack | Confirmed past pause, **Unverified hypothesis** as to mod/driver/engine cause | Preserve G1/ETW report limits; no new causal finding, extra capture or speculative DComp/mutex rewrite |
| Renderer shutdown logs | No finalizer record in the supplied G5 logs | Cleanup unverified by those logs; does not reopen user-accepted G5 |
| GPU/VRR compatibility | NVIDIA coexistence user-observed; indicator disappearance while menu open accepted | Neither the indicator nor logs prove actual VRR state; no universal driver/FG/wrapper guarantee |
| Weapon/viewmodel FOV after Cinematics | Known game behavior; standalone reflection/invocation repair deferred | Keep clear known-limit documentation; not a promised new subsystem for this final release |
| FOV source/identity research | Retained baselines, association and native consumer/lifetime evidence have named limits | Do not infer identity from matching numbers or revive exhausted searches as automatic release work |
| Arabic/RTL/accessibility | Glyph/resource coverage does not supply shaping/bidi layout or screen-reader support | Accept/document actual supported interaction; no claim of comprehensive accessibility |
| Custom AspectRecalculation sizes | Known game Auto resize behavior | Preserve described HorPlus/RuntimeCombine alternatives; no new camera policy |

Sources reused: [G4 regression](OVERLAY_OWNED_HWND_G4_REGRESSION_RESULT_2026-10-13.md), [G5 acceptance](OVERLAY_OWNED_HWND_G5_COMPATIBILITY_STATUS_2026-10-13.md), [G2/G3 gap result](OVERLAY_OWNED_HWND_G2_G3_GAP_RESULT_2026-10-13.md), G1-A/B/C/D/E stage reports and [finalization](OVERLAY_OWNED_HWND_G1_FINALIZATION_2026-10-13.md), [native evidence audit](../evidence/runtime-decomposition/NATIVE_RUNTIME_EVIDENCE_AUDIT_2026-10-09.md), [N01–N03 remediation](../evidence/runtime-decomposition/N01_N03_NATIVE_RUNTIME_SAFETY_REMEDIATION_IMPLEMENTATION_2026-10-09.md), [N06 consolidated evidence](../evidence/native-runtime/N06/N06_NATIVE_RUNTIME_EVIDENCE_CONSOLIDATED.md), [Diagnostics unification](DIAGNOSTICS_UNIFICATION_IMPLEMENTATION_REPORT.md), and [Hybrid test migration](TEST_INFRASTRUCTURE_HYBRID_MIGRATION_IMPLEMENTATION_REPORT.md). Earlier PASS results were not rerun or attributed to a different source/artifact.

## 6. Coverage transparency: reviewed areas without a new finding

“No finding” means no concrete additional problem was established in the named reviewed paths, not proof of the absence of every possible fault.

| Area | Reviewed paths and result |
| --- | --- |
| Architecture/dependencies | Explicit registry and one RuntimeState; domain helpers versus native orchestration; Overlay not a settings owner. No justification for further runtime splits/managers |
| HorPlus/FOV math | `camera/horplus.cpp`, `gameplay/horplus_gameplay.cpp`, recovery range/endpoint checks; finite/range/flag gates in production transactions. No new arithmetic or double-transform defect established |
| RuntimeCombine | Current viewport target, aspect write only on mismatch, readback/flags confirmation before HorPlus, recovery handoff and mode/module dispatch. Existing adapter failure cases remain relevant; FP11 concerns the window supplying that target |
| Cinematic lifecycle | Captured selection policy on ENTER, generation/association conflict handling, accepted/rejected/indexed EXIT routing, continuation, baseline retention and validated recovery. No new normal-path defect; native/lifetime gates remain |
| Dialogue | Adaptive/Reduced optical projection, Disabled behavior, Candidate/Active/Exiting/Rearm, source-change cancellation, Cinematic exclusion, Enabled/Mode transactions. No user-selectable Native mode reintroduced; no new numeric defect found |
| Native resolvers | Unique-match refusal, bounded Zydis validation for writer/aspect/ENTER/EXIT, shared call target, concrete Dialogue instruction bytes, exact-image Zoom gate. No new signature mismatch; scanning expects the valid loaded game module, not arbitrary hostile PE input |
| Native ABI/vendor | SafeMidHookEntry on ordinary camera/Zoom hooks, intended aspect-store RIP skip, register/store contract and relocation evidence; N01 patch distinguishes uncertain bytes and protects storage. No newly proved vendor relocation defect |
| Graphics ownership | Private D3D11 device/context, no game swapchain/Present hook or retained game queue/backbuffer; ComPtr storage, matched BeginDraw/EndDraw, clipped damage updates, premultiplied-alpha output, map failure unmap and ClearState/Flush shutdown. No ordinary COM leak identified; driver release remains distinct |
| Composition/geometry | Target on own unowned display HWND, root toggles without target churn, client placement/DPI cache, self-event exclusion, accepted show state, one bounded device-recovery attempt, surface publication after accepted replacement. FP01/FP11 are separate remaining boundaries |
| Input/focus | Admission gate closes terminally; owner heartbeat fail-open, display click-through, stale generation checks, queue saturation refusal, native continuation, cursor/escape/rebind logic. General keyboard forwarding accepted; FP07/FP14 add precise timing/publication concerns |
| Configuration parsing | Bounded font sizes, booleans/modes/locales/hotkeys, unique binding repair, module switches and legacy migration. Atomic write/flush/close/replace preserves original on staged write/replace failure. FP05/FP06/FP15 concern separate stages |
| Diagnostics/performance | OFF gates on continuation storage, provenance/counters/predictions; bounded buffers, capped notifications, change-based snapshot wake, deferred camera logging, demand-driven compositor clock/fallback animation. No new measured performance defect; prior leaf timings are not whole-game FPS evidence |
| Localization/Unicode | 18 registry entries/RC catalogs, strict UTF-8 and surrogate parsing, bounded JSON depth/resource sizes, unique flattened keys and placeholders, six font profiles, stable embedded font lifetime, locale font sizing. No catalog/resource mismatch established; shaping/accessibility limits retained |
| UI projection | Effective settings/status readback, Actual versus Potential, per-fact freshness/validity, layout clamping and selector tooltip identities. Technical panel code remains observational. FP12 is the missing ordinary unsaved warning |
| Build/test assertions | Explicit source roles, generated dependencies, localized /showIncludes remediation, separate production/test objects, resource dependencies and canonical error propagation. NDEBUG is explicit only for native-adapter targets, whose assertions use explicit checks; no global disabling of ordinary harness assertions found |
| Packaging/install | Existing v3.1 ZIP contains five flat entries: ASI, INI, README.txt, LICENSE.md, THIRD_PARTY_NOTICES.md. No Vortex JSON or duplicated `Stalker2/Binaries/Win64` nesting in that archive; notices/resources retain external attribution |

The current CTest registration is five entries, not the complete 59-target suite. The five-entry candidate wrapper must not be described as a full canonical PASS. The actual-adapter main loop's 204 fresh children and four separately invoked Zoom scenarios are explicitly distinguished from harness/runner counts. Localization policy regexes are source checks and are not graphical runtime assertions.

## 7. Prioritized, independently implementable remediation

These batches are **proposals awaiting implementation approval**. Each has concrete entry points and focused acceptance checks; no new broad discovery phase is required. Keep ASI identity/name, accepted camera semantics, one Runtime owner and one graphics owner. FOV/INI compatibility scope must not silently expand.

| Batch | Findings / change | Focused acceptance | Priority and dependencies |
| --- | --- | --- | --- |
| R1 — Test containment | FP02: bounded process-tree supervisor, cancellation/finally ownership, verified exit; bound exit-contract waits and entry-point load | Parent/grandchild timeout/crash/cancel leave no owned processes; nonzero propagation retained | First, before executing new native checks |
| R2a — Optional Overlay exception boundary | FP01: effective worker boundary, safe failure logging, correct leaf exception specifications, existing finalizer handoff | Throwing logger/controlled allocation does not terminate process; input opens to game; truthful cleanup state | Release blocker; independent of camera algorithm changes |
| R2b — Initialization and hash ownership | FP04/FP10: truthful inner outcomes/readiness, optional/wide-safe logging decision, RAII handles | Inner failure does not publish ready; logger-unavailable result explicit; allocation faults release owned handles | Release correctness; independent of R2a |
| R3 — Supported native reset | FP03: admission/recheck/state-reset serialization and documented self-callback behavior | Latch admitted callback against reset; no late correction/reopened readiness; isolated native continuation retained | High-risk focused lifecycle slice after R1; preserve N01 retention and unsupported dynamic unload |
| R4a — Config read/duplicate preservation | FP05/FP15: checked reads before replacement, consistent multi-key duplicate semantics | Mid-read failure preserves bytes; repeated Overlay sections save/reload position and unmanaged tail | Low-risk data-safety slice |
| R4b — Ordered settings commit and feedback | FP06/FP12: shared producer ordering and accepted-but-unsaved status | Forced two-producer reverse schedule reloads final accepted value; failed save is visible and non-destructive | Medium API slice; use R4a's safe persistence |
| R5 — Input event/publication semantics | FP07/FP14: event-time rebind modifiers, nonblocking immutable native-reader publication | Both delayed modifier schedules and concurrent reader-init complete/fallback pair | Focused boundary repair; no general keyboard-policy change |
| R6 — Test graph/artifact isolation | FP08/FP09: binary-tree-local generated includes and executables, pass actual output directory to runner | Fresh alternate config harness consumes its own header; two trees cannot replace each other's test subject | Needed before final qualification; independent of mod behavior |
| R7 — Authoritative camera viewport | FP11: exclude own display and select usable game candidate independently of Overlay availability | Surrogate host/display, foreground fallback and geometry mismatch select game rectangle | Small cross-subsystem repair |
| R8 — Resident continuation budget | FP13: choose explicit bounded storage/acceptance policy; reclaim only with proved safety | Repeat rebind/stale/foreign chains preserve continuation and enforce chosen budget | Low; may be consciously accepted instead of changing lifetime |

Run focused validation only for the changed contract in each approved batch. Do not run the entire suite after every mechanical edit. Use the required finite external timeouts, serial native/graphics fixtures and actual process/session cleanup checks. Do not start graphics fixtures while the user is gaming without the already-required separate agreement.

### Final qualification and maintenance handoff

After approved remediation/polish, F4 should perform one canonical regression run on the final source, then build the intended `STALKER2CameraTweaks.asi`, inspect compiler/resource inputs and warnings, and record source-input/toolchain/artifact identities. Qualify the final current artifact in the agreed normal gameplay/Overlay scenarios once; the prior G5 result remains evidence for its own artifact. Do not revive rejected target-release experiments or require historical E349 bytes.

Make a new archive from that intended artifact and compare its ASI hash, INI/defaults, installation layout, licenses/notices and user-facing notes. Keep the verified rollback and a clear restore procedure. The existing ZIP's flat layout and lack of Vortex override should be preserved unless the user explicitly chooses a change. Do not silently overwrite an old release package during qualification.

The release stop criterion is: no open confirmed Critical/High defects; the supported initialization/reset/persistence/input guarantees are either repaired and focused-validated or explicitly accepted with consequences; required final artifact/package checks pass; each remaining Medium/Low risk has a named acceptance/defer decision. Unsupported dynamic unload, universal graphics-stack compatibility, full Arabic shaping, driver completion and a new weapon/viewmodel repair are not automatically new release requirements.

F5 is publication and maintenance, separately authorized. Maintenance should be bounded: updated-game resolver/layout adaptation, reproducible confirmed bug fixes and small compatibility corrections. New research ideas do not automatically enter this final release or reopen completed G0–G5/B1–B7 work.

## 8. Completion and preservation

All fourteen requested subsystem areas were inventoried and reviewed at their production boundaries and critical failure/recovery paths. The matrix is deduplicated; each finding has location, trigger, mechanism, coverage limits, minimal repair and focused validation. Existing open evidence gates and accepted limitations are separated from new source defects. No fixes or tests were implemented or executed.

The final preservation check **matched** all 379 input records and aggregate SHA-256 in section 1. The installed ASI still hashes to `123D792A61A550892141DBEC12BB059A606BEA6A30B0C8B300491B6CAFC7DEF2`, and rollback still hashes to `C7287C8C52233EC5B8F8E32A0DBC6C38209DDA15F9C07FE99C505CA4FDC75C7C`. The report contains 15 unique detailed finding sections and 15 matrix rows; severity/classification totals agree with the executive assessment. Git reports only this new audit report as an untracked deliverable for this task. Existing staged/unstaged work and all reference binaries remain owned by the user. This audit stops at the report and remediation recommendation for review.

## 9. Follow-up addendum — FP16 / R9 (2026-10-13)

This addendum records a later, separately authorized finding and validation. It does not alter the original audit snapshot or its 15-finding count above.

| ID | Subsystem | Severity | Classification | Current disposition |
|---|---|---|---|---|
| FP16 / R9 | Native hook execution consistency | Medium | Confirmed test-fixture defect; no production hook defect established | Fixed in the evidence harness; focused OFF/ON and canonical regression PASS |

**Evidence and mechanism.** `tests/plugin/runtime_native_evidence_cases.hpp`, `EvidenceNativeTrampoline()`, exercised `SafeMidHookEntry` while the global `g_nativeCallbackAdmission` remained in its default closed state. Unlike neighboring evidence scenarios that enter through `Setup(Memory&)`, this scenario did not need the game-memory setup and had omitted the equivalent runtime-startup admission precondition. `SafeMidHookEntry` therefore correctly rejected callback admission and followed the native continuation, making the observer appear not to execute. The observed installed patch and target→trampoline/stub path were consistent with that rejection, not a failed patch write.

**Instruction-cache hypothesis.** SafetyHook's lack of an explicit `FlushInstructionCache` was investigated as a plausible risk, not presumed causal. A controlled test-only flush on the target, then target plus trampoline epilogue/stub, did not change the failure. No vendor/production change was justified by this evidence; the cache-flush hypothesis was not the cause of this reproduced failure.

**Minimal correction.** The evidence case now asserts that admission begins closed and opens `g_nativeCallbackAdmission` before invoking the hook. Production callback admission remains unchanged. No SafetyHook, native ABI, continuation lifetime, or camera behavior was modified. R9 is separate from R8, which addresses the lifetime/budget of native continuations.

**Validation.** `evidence-native-trampoline` passed with Diagnostics OFF and ON. The subsequent single canonical `test.cmd` completed with exit 0 and 62 runner invocations. The actual-adapter matrix passed 104 scenarios in each diagnostics mode (208 child processes total), and all four N06-M01 Zoom scenarios passed. The process-tree supervisor reported no timeout, successful cleanup and no surviving descendants; post-run inspection found no remaining build/test processes. No game, graphics fixture, ETW capture, production build or ASI installation was performed for R9.

**Scope and remaining gates.** FP16 is closed as a fixture defect, not evidence of a production hook-execution defect. This result does not change N03 (`Partially mitigated`) or N04–N06 (`Open`), and does not satisfy final release ASI identity, packaging or live-runtime qualification. The modified fixture SHA-256 recorded at completion is `398F492B0CBB9B7F994E6BC7BC7D60D7FC43367EABECD3A9AFFBA51D8AD4B1D5`.
