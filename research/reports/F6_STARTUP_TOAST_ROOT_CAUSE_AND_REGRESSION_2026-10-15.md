# F6 — Startup Toast Root Cause Investigation & Regression Closure

**Date:** 2026-10-15  
**Status:** Local lifecycle invariants verified; exact live presentation cause remains unlocalized. Event-based diagnostics added; no speculative rendering behavior change.

## Executive summary

The available evidence does **not** establish a production rendering defect or a single root cause for the reported startup welcome-toast behavior. It establishes a precise constraint that resolves part of the apparent contradiction:

- In normal reconciliation, the composition root is detached only when the menu is not requested, the owner-thread notification queue is empty, and startup is no longer pending.
- Once startup notification ID `0` has been removed, opening the menu does not recreate it. Startup is one-shot and `MarkCreated` cannot return to pending.
- The supplied logs record successful toast draw-list submission/commit and later root detach, but do not timestamp each event or record queue contents, TTL deadline, eviction, or detach reason. They cannot show why the queue became empty or whether pixels were visible before detach.

A deterministic regression now covers startup-only attachment demand, Auto-locale wait, draw/commit-gated TTL, menu transitions while a toast is queued, expiry/detach ordering, and no replay on later manual open. A mutation sentinel confirmed that the test fails if the pending-startup attachment term is removed. The canonical suite passed. No game, ASI installation, ETW, PresentMon, or graphics fixture was run.

The production changes are event-only diagnostics and extraction of the existing attachment predicate into a pure helper shared by production and tests. No startup delay, TTL extension, replay, polling, topmost change, or menu/render behavior change was introduced.

## Evidence and limits

Reviewed the supplied Overlay/Startup/runtime logs; F5 hardening report; invisible-overlay desktop report; G1-C/G1-E lifecycle reports; and current renderer, composition owner loop, notification state, redraw policy, and focused harness.

The user’s live clarification is retained as direct observation: the welcome toast did not disappear before the Overlay was manually opened, and it had previously been described as becoming noticeable at manual open. The exact visual sequence is not independently captured. Do not interpret this as proof the toast pixels were visible before opening or as proof the same notification was replayed after detach.

The installed log records `OVERLAY_STARTUP_HINT_CREATED`, `OVERLAY_NOTIFICATION_LIFETIME_STARTED id=0`, and `OVERLAY_STARTUP_HINT_DRAWN_AND_COMMITTED` before manual `OVERLAY_VISIBILITY=VISIBLE`, and later records `OVERLAY_DCOMP_ROOT_ATTACHMENT attached=0`. It has no per-event monotonic timestamp, queue snapshot, TTL deadline, eviction record, or detach reason. `DRAWN_AND_COMMITTED` means the notification draw list was submitted and the renderer’s surface/attachment commit path succeeded; it does not prove Windows composition displayed those pixels to the user.

The prior standalone desktop fixture captured toast pixels in a surrogate host. That is component evidence only, not integrated game-startup evidence. The earlier blackout incident is why no graphics fixture was repeated.

## Lifecycle reconstruction

### Automatic startup

1. `Renderer::InitializeImpl` validates both HWNDs, creates D3D/DComp objects, commits the initial surface, marks the initial root attached, then prepares the toast, builds the first ImGui frame and calls `StartPendingNotificationLifetimes`.
2. `StartupNotificationState::ShouldWakePresenter` requires presenter readiness and input binding. If Auto-locale synchronization was accepted but incomplete, startup remains pending. The accepted result is marshalled to the owner command queue; owner processing applies it and requests redraw. A rejected request uses the existing confirmed English fallback.
3. `EnsureStartupNotification` creates ID `0`, appends it to owner-owned `notifications_`, calls one-shot `MarkCreated`, and marks the surface dirty.
4. `DrawNotifications` produces the draw list. `RenderImpl` records the notification ID only when its ImGui draw list with vertices is submitted. `DrawImGuiSurface` must complete `EndDraw` and DComp `Commit` successfully.
5. `StartPendingNotificationLifetimes` reconciles attachment/display state, then starts TTL only for a submitted ID after successful draw/commit and when the display HWND is visible. Time is based on `GetTickCount64`.
6. `UpdateNotifications` removes expired entries or can evict the oldest entry at the three-item limit. It marks the surface dirty. Reconciliation then detaches only if root demand is false.

### Manual menu open

Delete changes requested menu state at the game input boundary and wakes the owner. Owner-side reconciliation sees `menuRequested=true`, attaches/commits if necessary, updates display geometry/visibility/z-order and renders the menu. This path does not call `EnsureStartupNotification`; it cannot recreate startup ID `0`. Closing the menu leaves the root attached while any notification remains queued.

## Root-detach contradiction

The normal predicate, factored as `CompositionRootRequired`, is:

`menu requested || active notification count != 0 || startup notification pending`

Therefore a normal `attached=0` from `LogCompositionAttachment` means reconciliation evaluated demand as false. After startup creation, pending is false; normal detach also requires a closed menu and empty notification queue. The queue may become empty through TTL expiry or capacity eviction. Old logs cannot distinguish these. `ResetComposition` is a separate lifecycle detach/reset; the new event stream records it as `reason=composition_reset`.

Exact source locations in the current tree: `renderer_runtime.cpp:814-875` (`InitializeImpl`); `1611-1636` (`EnsureStartupNotification`); `1638-1677` (Auto-locale preparation); `1576-1608` (queue additions, capacity eviction and expiry); `1691-1735` (normal demand predicate and committed root transition); `1738-1769` (TTL start); `1772-1840` (toast draw construction); `2650-2685` (submitted ImGui draw-list evidence); `1522-1560` (reset detach); and `1434-1460` (display show/hide). `renderer_state.cpp:5-12, 19-43` contains one-shot creation and TTL state. `composition_runtime.cpp:811-850` processes owner commands; the game input boundary requests menu visibility at `composition_runtime.cpp:685`.

`OVERLAY_DCOMP_ROOT_ATTACHMENT attached=0` is emitted by `LogCompositionAttachment` at `renderer_runtime.cpp:1681-1687`, called by normal reconciliation at `1732`. `ResetComposition` does not emit that legacy string; it now emits the separate lifecycle event. Thus the supplied legacy `attached=0` specifically came from normal reconciliation, which means the queue was empty and the menu closed at that point. The missing evidence is why ID `0` left the queue (expiry versus capacity eviction) and what pixels the user saw before that transition.

After actual expiry/eviction, the current one-shot state machine cannot replay the same ID through manual open. If a future trace appears to show it after an empty-queue detach, that would contradict the reviewed call graph and should be preserved as new evidence. The old logs cannot prove such a sequence.

This does not invalidate the user observation. It shows that the old logs cannot map it to a specific event. The specific source path “normal reconciliation detaches while an entry remains queued” is ruled out under the current owner-thread serialization.

## Hypothesis matrix

| Hypothesis | Finding | Basis / limit |
|---|---|---|
| Normal reconcile detaches while toast remains queued | Ruled out for current owner-thread path | Predicate includes queue count; owner thread mutates the vector. Mutation-sensitive test covers it. |
| TTL begins before submitted draw and successful commit | Ruled out by code/test | Lifetime start requires submitted ID and accepted commit; pending lifetime cannot expire. |
| Auto-locale wait drops toast or consumes TTL | Not supported | Pending demand remains; accepted result wakes owner; focused tests cover wait. |
| Queue eviction removed ID `0` | Possible, not evidenced in supplied log | Three-entry cap exists; old log lacks queue/eviction events. |
| TTL expired before user noticed toast | Possible, not evidenced | TTL is 10 seconds after accepted visible-frame conditions; user says it did not disappear before manual open. Do not assert this explanation. |
| Manual Delete recreates startup toast | Ruled out by source | One creation call site; `MarkCreated` is one-shot. |
| DComp commit proves pixels reached the user | False evidence claim | No compositor/pixel acknowledgement; surrogate capture is not game capture. |
| Short INI tag change caused this symptom | Not established | Prior desktop report reproduced related visibility/input mismatch with preserved G5/F4 references too. |

## Prior fixes and coverage gap

G1-C/G1-E harnesses cover owner lifecycle, host state, input admission, Auto-locale gating, pending lifetime, expiry boundaries and submitted-draw/commit ordering. The 2026-10-15 desktop investigation reproduced and corrected a host z-order issue using a surrogate pixel oracle. Neither scope proves actual integrated game pixels during startup.

The relevant F2-F5 work is present as staged/unstaged working-tree changes and date-stamped reports rather than a clean sequence of commits for this path; the available path history reaches the v3.1.0 release commit. Therefore no commit boundary can be identified as the toast regression trigger. The source comparison and reports show the existing z-order correction and one-shot/commit-gated TTL contracts, while the missing combined startup-only attachment/presentation coverage explains why offline PASS did not establish the live symptom.

The missing deterministic coverage was the full root-demand rule with the startup toast as the sole attachment reason, through menu open/close and subsequent expiry. This task adds that coverage; it does not establish that an earlier fix regressed.

## Deterministic regression matrix

`TestStartupToastAttachmentLifecycleWithoutMenu` uses production pure-state helpers and a fake monotonic clock.

| Case | Assertion |
|---|---|
| T01 | Pending/queued startup toast alone keeps root required with menu closed. |
| T02 | Accepted Auto-locale wait preserves pending demand; creation follows resolution. |
| T03 | Toast-only attachment demand persists while queued. |
| T04 | Manual open does not create a second startup toast. |
| T05 | Closing menu cannot detach while toast remains queued. |
| T06 | TTL starts after submitted draw+commit; expiry permits detach at exact deadline. |
| T07 | Later reattachment does not replay the expired one-shot toast. |
| T08 | Existing host activity/visibility/placement tests cover reconciliation state, not game pixels. |
| T09 | Failed/delayed commit leaves TTL pending and non-expiring. |
| T10 | Repeated open/close after expiry returns to settled detached state without replay. |

### Mutation sensitivity

Temporarily removed the pending-startup term from `CompositionRootRequired`. The focused harness failed at `pending startup toast alone keeps the composition root attached` (exit `1`; process cleanup verified). Restored the predicate, rebuilt the target, and reran successfully (exit `0`; cleanup verified). No test mutation remains.

## Diagnostics added

When `[Diagnostics] Enabled=true`, owner-thread `OVERLAY_TOAST_LIFECYCLE` records include monotonic timestamp, thread ID, event/reason, notification ID, queue count and each entry’s started/pending state, age and deadline, startup pending, menu request, root attachment, display/HWND visibility, host activity/minimize state, surface generation and z-order sample.

Events are transition-based, not per-frame: startup locale wait/creation; queue add/evict/expiry; toast draw-list submission and TTL start; initial/rebind/recovery attach; normal reconcile attach/detach with demand reason; composition-reset detach; display show/hide transitions.

With diagnostics disabled, the logger returns at the atomic diagnostics gate before clock reads, queue traversal, window queries or formatting. `Renderer::SetVisible` does not call it because that path may execute outside the renderer owner thread.

## Validation

- `test.cmd`: **PASS**, exit `0`.
- Registry: **59/59** repository targets; **62** serial runner invocations; **105** adapter cases / **210** fresh child processes; **3** resource targets; no missing inputs.
- CMake configure and harness build: **PASS**; changed renderer translation unit and presenter harness compiled.
- Runner exit-code/cleanup contract: **PASS**, including negative NTSTATUS, crashes, timeouts, descendants and CTest-wrapper containment.
- `composition_presenter_state_harness`: **PASS** after restoring the tested invariant.
- N06-M01: **4/4** scenarios passed in the canonical run.
- Runner verified process-tree cleanup. Its configured expected-helper cleanup handled residual `vctip.exe`; a subsequent process check found no active CMake/Ninja/compiler/linker/WPR/PresentMon/game process.
- No game, graphics fixture, GPU stress, ETW, PresentMon or ASI installation.

`git diff --check` could not inspect unstaged changes: this checkout reports `fatal: this operation must be run in a work tree` for that command even though `git status` and `git rev-parse --is-inside-work-tree` work. `git diff --cached --check` reports trailing whitespace in unrelated already-staged research reports; those files were not modified. No staging/index operation was performed.

During validation, an initial compile caught a duplicate local test definition introduced while editing; it was removed before the passing canonical run. An initial standalone mutation build lacked the VS developer environment; the subsequent correctly initialized build produced the expected mutation failure. All temporary mutation was restored and the focused test passed.

## Changed files

- `src/overlay/presenter/renderer_runtime.cpp` — event diagnostics and attachment/display reasons.
- `src/overlay/presenter/renderer_runtime.hpp` — diagnostic declaration/state; no cross-thread logging from `SetVisible`.
- `src/overlay/presenter/renderer_state.hpp` — pure attachment-demand helper and visible-time accessor.
- `tests/overlay/composition_presenter_state_harness.cpp` — startup-toast attachment/lifetime regression.
- `research/reports/F6_STARTUP_TOAST_ROOT_CAUSE_AND_REGRESSION_2026-10-15.md` — this report.

Existing staged and unrelated uncommitted changes were preserved. Nothing was staged, unstaged, reset or reverted.

## Root cause and production-fix status

**Exact integrated live root cause: not proven.** The source-level path “normal reconciliation detaches while toast remains queued” is ruled out and protected by a mutation-sensitive regression. Supplied logs cannot distinguish expiry, capacity eviction, lifecycle reset, or a presentation discrepancy at the time of the observation.

**Production behavior fix: none justified.** Replay, TTL extension, startup delay, forced topmost, or schedule changes would be speculative. The bounded event stream is the minimal next-evidence change.

## One bounded live protocol

The currently installed ASI does not contain the new event records. If the user later approves using a diagnostic candidate, build it separately, preserve the installed/rollback ASI, and have the user install it for one ordinary game run with `[Diagnostics] Enabled=true`. Retain the Overlay log from startup through one manual Delete open/close. This task did not build or install that candidate. Correlate:

`startup_created → toast_draw_submitted → root_attach/display_show → ttl_start → queue_expire or queue_evict → root_detach`

Compare timestamps, ID/queue state and the first menu/root-attach event with the user’s observation. Preserve any record that shows the same ID after an empty-queue detach. A screen capture is optional during the user’s normal manual test only; no automated graphics fixture is warranted.

This protocol distinguishes the remaining live presentation hypotheses; it does not reopen completed G1 architecture or lifecycle tests.
