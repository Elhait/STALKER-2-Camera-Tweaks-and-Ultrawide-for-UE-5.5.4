# F4 Preflight Remediation — Implementation Report

Date: 2026-10-15. Scope: approved R1–R7, PF01–PF09.

**Result: implementation and offline regression PASS. Live qualification pending.**
The original preflight audit remains the historical record of the blocked tree;
this report records the subsequent fixes, not a reinterpretation of its evidence.
No game, ETW or PresentMon was launched. No ASI was installed or release packaged.

## 1. Baseline and boundaries

Baseline HEAD: `e756442bcf9942923b4531fb513af893f6aecae7` plus the user's existing
staged integration work. The staged index was preserved byte-for-byte:

`72FAC39B216993275F82BCB64A7032F1098FA30F8B3D09F105F97AED6404CD19`.

Changes in this task are unstaged. Previous G1–G5 and F2 R1–R9 decisions are not
reopened. Camera algorithms, native camera hooks/ABI, vendor code, release
artifacts, configuration defaults and installed INI are untouched. Diagnostics
remains an INI switch in the same ordinary ASI, not a second required build.
N03 stays **Partially mitigated**; N04–N06 stay **Open**.

## 2. Changes and focused evidence

| Batch / findings | Change and source | Focused result and limits |
| --- | --- | --- |
| R1 / PF01 | `src/platform/win32/sha256.cpp:59`: ScopedHashHandle owns its backing vector. Its destructor destroys the BCrypt hash before the member buffer is destroyed; the earlier algorithm owner still outlives the hash. | Existing actual-adapter SHA allocation checkpoints 1/2/3 PASS with Diagnostics OFF/ON. This exercises failures including final digest formatting and unchanged result/file-handle behavior; lifetime ordering is established by the owning type, not inferred from a green run alone. |
| R2 / PF05, PF06 | `src/config/config_template.cpp:724,1146`: migrate Features.GameplayAspectFix before defaults, with valid canonical Gameplay.Enabled authoritative; combine repeated managed sections into one logical body before filling missing keys. Preserve assignment order, unknown assignments and compatible values. | New persistence regression PASS for split Overlay/Gameplay sections, legacy false, canonical false versus legacy true in either order, and last-assignment precedence. Repeated sync preserves bytes, timestamp and file identity. All existing config persistence cases also PASS. |
| R3 / PF03, PF08 | `src/overlay/input/game_thread_subclass.hpp:11`; `src/overlay/composition/composition_runtime.cpp:1020,1101`; `src/overlay/presenter/renderer_runtime.hpp:49`; `renderer_runtime.cpp:1556`; `renderer_state.hpp:21`. Commit the binding on the HWND thread, publish the actual returned predecessor before further normal dispatch, acknowledge on the owner, then allow startup language preparation. | A real Win32 interleaving first demonstrates the stale-observed predecessor skipping an intervening foreign subclass. The shared production installer then preserves that chain and rejects off-thread installation without mutation. Existing Win32 stale-generation/foreign-chain/WM_NCDESTROY/drain cases PASS. Startup state regression rejects preparation before binding commit. Real game message-pump/startup-language delivery remains a live qualification item. |
| R4 / PF02 | `renderer_runtime.cpp:1338,1413,1605`: reconcile activity from current valid foreground GameHwnd and minimize state at binding completion, activation processing and attachment reconciliation. Queued event payloads do not override a newer foreground snapshot. | Presenter policy cases PASS for missed activation recovery and refusal for inactive, minimized or invalid hosts. Existing keyboard-focus-loss preservation cases PASS. This does not establish live focus behavior for every external overlay. |
| R5 / PF04 | `presenter_runtime_policy.hpp:16`; `renderer_runtime.hpp:74,89`; `renderer_runtime.cpp:1644,2611`: owner-local menu-frame admission is independent of requested visibility and toast attachment. Accept only a completed menu draw followed by accepted visible attachment. Revoke on close/reopen, host invalidation, rebind, composition reset and terminal state. Force damage for a changed menu request. | Focused cases PASS: toast-only display cannot admit input, draw without accepted display cannot admit input, successful draw/display can, and reopen/invalidation cannot revive old acceptance. Ordinary redraw dirtiness alone does not revoke a valid existing menu. Predicate/state checks plus actual call-order review are not a live input/render test. |
| R6 / PF07 | `renderer_runtime.cpp:1495`: DetachImGui always shuts down the D3D backend when a context exists, even if Initialize failed before imguiReady became true. Backend shutdown remains before context destruction. | New actual-adapter `evidence-partial-imgui-cleanup` PASS OFF/ON. Software WARP at feature level 9_1 refuses the production vs_4_0 pipeline after real device/context ownership. The test verifies partial ownership existed, then actual DetachImGui empties backend references and destroys its context. No display HWND/DComp target is created. Empty local references do not prove driver/DWM completion. |
| R7 / PF09 | `tests/runner/ctest_harness_runner.ps1`; `CMakeLists.txt:389`; `tests/runner/test_cmd_exit_code_contract.ps1`: direct CTest calls use the existing Job Object supervisor, bounded output, inner timeout and checked cleanup. CTest outer timeout is longer (330s versus 300s); entries are serial. | Wrapper returns 0, 37 and -1073741819 exactly; descendant timeout returns 124 with all three marked processes stopped. Four real N06-M01 CTest executions PASS. Canonical regression includes the wrapper contract. The entire native matrix was not redundantly rerun through CTest. |

### R3 bounded lifecycle decision

PF08 was not addressed merely by changing the preliminary WNDPROC snapshot or
adding a mutex. The real Win32 reproduction justifies changing the installation
boundary. A temporary **thread-specific WH_GETMESSAGE** hook delivers the install
operation on GameHwnd's existing thread. The request only accepts a nonzero
thread in this process; it cannot silently become a global hook. The GameHwnd
callback does not wait on the owner, enter the Renderer mutex, persist settings
or tear down graphics. It publishes completion and wakes the existing owner.

The owner removes the bootstrap hook on completion and during finalization.
Installation/post/unhook failures enter terminal fail-open handling; an unremoved
observer contributes to Retained, not a false Complete. Stable records remain
resident for native continuation. A host replacement while binding is unfinished
terminates the optional Overlay rather than reusing the pending binding. No
second graphics owner or new lock ordering is introduced. The game must process
its message queue before asynchronous installation can complete; the owner does
not claim input/language readiness merely because PostMessage succeeded.

### R5 ordering review

Opening while a toast is visible now invalidates menu acceptance immediately.
The existing owner can publish requested visibility, but InputPresentationReady
cannot promote capture until RenderImpl completes the menu draw/Commit and
StartPendingNotificationLifetimes accepts its attached, visible display. Failure
or terminal state keeps input fail-open. Notification visibility and menu
admission remain distinct; camera/publication state is not used as UI readiness.

## 3. Validation results

All checks were sequential and externally bounded by the existing process-tree
supervisor. Each final check below returned **exit 0**, **TimedOut=false** and
**CleanupVerified=true**. Logs and compact metadata are retained locally under
`build-artifacts/f4-preflight/` (ignored; not release content).

| Evidence label | Actual check |
| --- | --- |
| `r1-build`, `r1-off`, `r1-on` | actual-adapter build and SHA failure ownership cases |
| `r2-build`, `r2-config` | config persistence build and full existing config harness plus new cases |
| `r3-proof-build`, `r3-proof`, `r3-build`, `r3-input` | deterministic native chain reproduction, shared installer and existing Win32 input lifecycle |
| `r4-build`, `r4-state` | activity snapshot policy and existing presenter lifecycle |
| `r5-build`, `r5-state` | menu admission state and existing presenter lifecycle |
| `r6-build`, `r6-off`, `r6-on` | actual partial D3D backend failure/cleanup |
| `r7-configure`, `r7-contract-fixed` | CMake regeneration and wrapper status/timeout/descendant contract |
| `r7-ctest-zoom` | four N06-M01 scenarios through direct CTest |
| `final-registry` | strict canonical registry audit |
| `canonical-once` | one full test.cmd after all production changes |
| `final-asi-build` | normal production-profile build.cmd, isolated output, serial compilation |

The canonical run rebuilt the final changed production adapters/config/presenter
sources and completed all **59/59 harnesses**, **62/62 runner invocations**,
**105 adapter cases × OFF/ON = 210 fresh processes**, and **4/4 Zoom scenarios**.
The actual starting tree had 104 cases/208 processes, not the historical
102/204 count. The added partial-backend case accounts for the increase; no
previous scenario or assertion was removed. Registry count guards were updated
to the exact new numbers, not relaxed.

Localization/resource preflight: **18 locales / 213 keys PASS**, including
parity, placeholders, UTF-8, embedded resources and font profiles. As before,
offline glyph coverage does not establish Arabic shaping/bidi or visual QA.
The final canonical compile and ASI build emitted no compiler warnings/errors.
Earlier focused builds exposed a shadowed local name and unused parameter;
both were corrected before the canonical run. Git's expected LF/CRLF conversion
notices are distinct from compiler warnings. `git diff-files --check` passed.

### Failed attempts retained, not relabelled

- `r7-contract`: exit 1, no timeout, cleanup verified. The ad-hoc focused launcher
  inherited a sandbox TEMP path unsuitable for the batch fixture. The ignored
  launcher was corrected to use the same project-local ignored temp convention
  as canonical test.cmd; no product fix was attributed to this environment issue.
- `r7-contract-valid-temp`: exit 1, no timeout, cleanup verified. The new wrapper's
  JSON array was nested by Windows PowerShell pipeline enumeration, so the
  descendant probe received one malformed argument and returned 65 instead of
  timing out. Explicit string-array conversion fixed the wrapper. The subsequent
  signed-status/descendant regression and actual CTest Zoom runs passed. No
  failure check or assertion was weakened.

## 4. Cumulative review and preserved invariants

Reviewed the complete task-local source/test/CMake delta against the preserved
index with `git diff-files -p`, including the two new implementation files.
Normal `git diff` in this environment reports a work-tree error; the plumbing
comparison and index hash were used instead, without changing Git configuration.

- SHA buffer/handle ordering is repaired without changing digest semantics.
- Config migration resolves old values before defaults; repeated sections do
  not inject later default overrides. Atomic persistence and no-op identity
  regressions remain green. No schema key/default/reset policy was added.
- Game callbacks remain nonblocking relative to settings/Renderer/owner cleanup.
  Native forwarding uses stable records and the actual predecessor. Bootstrap
  observers participate in terminal admission/drain and finalization accounting.
- Foreground activity, menu request and committed interactive display are
  separate authorities. Delete/request handling is not conditioned on capture.
- DComp target remains on the Overlay HWND with its established lifetime; no
  target destroy/recreate experiment, new wait or graphics mutex was introduced.
- Partial ImGui resources are released through the existing owner cleanup.
  Uncertain DComp teardown retention and truthful finalizer outcomes remain.
- CTest uses the existing containment mechanism rather than an untracked child
  launcher. Native matrix, resources and compiler contracts remain explicit.
- No installed configuration/binary, vendor, packaging, camera formula, native
  patch/trampoline implementation or pre-existing staged content was changed.

## 5. Artifact identity and cleanup

Normal ASI candidate (same required filename):

`build-artifacts/f4-preflight/candidate/STALKER2CameraTweaks.asi`

Size: **3,590,144 bytes**. SHA-256:

`24A50EAF166FD52E68AD90A5712C1415E5DC6CC4BA6D4157CC4BD53605A43E86`

Build used build.cmd with a candidate output override and parallel level 1.
All passive diagnostics remain compiled into this ASI and controlled through INI.
The candidate is **not installed and not live-tested**.

The repository-root ASI and installed ASI both remain at:

`B265DF72FF94A9BC05B8B9E57DA1C6B9F328530B19678D86098A4C6DDC64815F`.

No existing rollback artifact was overwritten. Build intermediates and focused
logs stayed under ignored build directories; no root .obj was created by this
task. Job Object results verified exit of owned trees after every run, including
both failed wrapper attempts and the timeout fixtures. Recognized residual
vctip.exe compiler helpers were terminated by their supervising job and recorded
as expected cleanup, not ignored. No new recording session was started. A CIM
cross-process inventory query was denied by the sandbox; cleanup claims rely
on the launched trees' Job Object verification, not on a guessed global process
inventory. This does not claim that unrelated system/driver resources are clean.

## 6. Final gate

PF01–PF09 have implemented fixes and the described focused/offline evidence.
The implementation stage is complete; **F4 release/live qualification is still
pending**, not a general runtime-correctness PASS. In particular, the new
bootstrap binding/language sequence, focus recovery and toast-to-menu admission
need observation on this candidate in the real game. Earlier live evidence is
not transferred to this SHA. No new game session was requested or started as
part of implementation. Historical N03–N06 statuses are unchanged.
