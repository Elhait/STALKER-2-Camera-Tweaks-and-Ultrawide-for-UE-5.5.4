# Overlay-owned HWND — cleanup handoff review

Date: 2026-10-12. Mode: source/platform-contract review only.
**Recommendation: feasible bounded protocol, but not ready for Renderer integration
or a G1 runtime claim without isolated failure-path validation.**

G0 is accepted as local feasibility PASS. Its two-window experiment did not test
the production exception boundary, worker exit, input bridge or cleanup.
[Design](OVERLAY_OWNED_HWND_BOUNDED_DESIGN_REVIEW_2026-10-12.md),
[G0 evidence](OVERLAY_OWNED_HWND_G0_FEASIBILITY_RESULT_2026-10-12.md).

## 1. Actual current path

| Boundary | Current source | Consequence for the proposed owned HWND |
| --- | --- | --- |
| Game WndProc | composition_runtime.cpp:578–603 | Takes g_rendererMutex before original-procedure lookup and input decisions, and again for FinishWindowMessage |
| Failure callback | composition_runtime.cpp:359–367 | Reacquires renderer mutex, calls Renderer::Disable, then bridge Deactivate and RemoveInputHook; only afterward posts WakeMessage |
| Disable / Shutdown | renderer_runtime.cpp:2426–2468 | Input Close, logging/journal, ImGui teardown and ResetComposition happen synchronously on the caller |
| Graphics reset | renderer_runtime.cpp:1274–1289 | SetRoot(nullptr), Commit and COM releases; this is graphics work, not atomic readiness publication |
| Bridge Deactivate | input_state.cpp:49–57 | Stores active=false but then locks its queue mutex and resets queued events |
| Input Close | input_state.cpp:173–182 | Atomic visible/focused stores precede a rebindMutex acquisition |
| Saved continuation | composition_runtime.cpp:174–186, 606–660 | Lookup/forget use a mutex and vector; foreign-subclass continuation is retained |
| Owner exit | composition_runtime.cpp:777–889 | GetMessage loop; final tail stops clock, removes hook and deactivates bridge, but does not explicitly call Renderer::Shutdown |
| Owner identity | composition_runtime.cpp:898–912 | Thread handle is closed after creation; successful normal exit does not clear uiThreadId or workerStarted |

**Important correction:** an ordinary C++ exception in the first WndProc work
lambda unwinds its scoped renderer lock before RunOptionalOverlayWork invokes
DisableAfterOverlayException. The reviewed path is not evidence that this exact
lock remains held across that catch. The failure callback nevertheless reacquires
it and synchronously tears down graphics on the game thread. Nested Renderer
failure handling can still run under the surrounding owner's renderer lock.

RunOptionalOverlayWork contains C++ work/transition exceptions, not access
violations under /EHsc. Deactivate is noexcept despite acquiring a mutex: it is
neither a nonblocking primitive nor protected against termination if its own
lock operation throws. Merely retaining its name in a new handler is insufficient.

No game crash/stall is attributed to these source paths by this review.

## 2. Findings

| ID | Classification | Finding / required disposition |
| --- | --- | --- |
| CH01 | Confirmed integration incompatibility | Current game-thread Disable performs ImGui/DComp cleanup. Do not reuse it as the owned-HWND failure callback. |
| CH02 | Confirmed blocking boundary | Close, Deactivate, RemoveInputHook, Log and Renderer access are unsuitable for the immediate lock-free revocation phase. Split revocation from reconciliation. |
| CH03 | Confirmed delivery gap | The posted wake is unchecked in this handler and is not authoritative cleanup state. A queue post alone cannot cover a stopped/not-yet-ready owner. |
| CH04 | Confirmed lifecycle gap | Existing owner epilogue has no unconditional Renderer cleanup or published cleanup outcome; startup work before the loop is not enclosed by an outer C++ failure/finalization boundary. |
| CH05 | Owned-window design limitation | Atomic revocation stops mod decisions, but does not instantly change native hit-testing of a still-visible interactive Overlay HWND. Owner-thread hide is a separate operation. |
| CH06 | Concurrency requirement | A terminal check before g_rendererMutex does not cancel a game callback already waiting on that mutex. In-flight callbacks must be accounted for before owner teardown. |

PostThreadMessage can fail before queue creation or when the queue is full; success
does not mean execution, and thread messages are not reliably dispatched by modal
loops. A published thread ID is not a cleanup guarantee.
[PostThreadMessageW contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postthreadmessagew).

## 3. Proposed authoritative contract — not implemented

Keep the existing Overlay owner. Add a small private, process-resident terminal
control record in composition runtime, not another Renderer/Runtime owner or a
generic manager. It owns only admission/terminal status and cleanup delivery:

- A monotonic terminal bit plus in-flight game-callback admission count. Admission
  and terminal closure must use one atomic CAS protocol, not independently sampled
  flags that can allow a late admission after cleanup begins.
- Owner phase: Starting, Running, Stopping, Stopped. Only owner advances running
  lifecycle; thread-creation failure publishes Stopped/no-resources.
- Cleanup outcome: Pending, Complete, or Retained/Unverified. Stopped is not a
  synonym for successful cleanup; resource ownership must determine the outcome.
- A persistent terminal-wake event and callback-quiescence notification if needed
  by the selected admission protocol. No new mutex. Handles must exist before
  hook admission and remain valid for late callers; no CloseHandle race.

For the process-resident ASI contract, bounded control handles can remain resident
until process termination. This is deliberate control-resource retention, not a
successful graphics teardown or support for dynamic unload. A proposed restart
would require a separate contract; terminal state is never reset in this slice.

### Game-thread phase

1. Atomically latch terminal failure, closing all new Overlay callback admission.
   This is the revocation linearization point and happens before logging/cleanup.
2. Use nonlocking revocation helpers only: effective InputState ownership and
   bridge activity must include the same terminal bit. Clear popup publication and
   the existing atomic hotkey-rebind capture/consumed-key flags. The two plugin
   setters currently perform only atomic stores (runtime.cpp:6663–6670).
3. Do not call Close, CancelRebind, Deactivate, Renderer::Disable/Shutdown,
   RemoveInputHook, allocations, stream logging or synchronous cross-thread UI.
   An in-flight Activate may not republish effective active=true after revocation;
   owner-side event application also checks terminal state before applying input.
4. Set the persistent terminal-wake event without waiting. A queue message may
   accompany it as a hint, never as the sole record. Store signal failure as a
   bounded atomic diagnostic status; do not retry indefinitely or clear terminal.
5. Reject a partially produced WndProc decision when work failed: no stale handled,
   Escape-consume, cursor-hide or finish-stage Renderer work. Return the native
   continuation for the current message, with default raw-input handling intact.
6. Release this callback's admission before returning. Never wait for owner
   completion, join a worker or acquire the renderer mutex in the failure callback.

Terminal pass-through must run **before** the normal renderer mutex. The existing
mutable vector lookup is not itself a nonblocking continuation contract. A future
implementation must safely publish a stable per-window continuation record before
installing its subclass and retain it while callbacks/foreign subclasses can use
it; no writes to the game's GWLP_USERDATA or freeing a captured record. HWND reuse
requires binding identity/generation, not a stale global last-original pointer.
This narrow binding-lifetime change is part of the proposal and needs review.

The original foreign/game WndProc must remain outside the optional Overlay catch.
Do not catch its exceptions or silently replace its continuation with DefWindowProc.
Cursor restoration runs on the game thread outside renderer locks; if requested
from owner, post a generation-checked game-window message. It must use the stable
continuation, with no allocation/logging before revocation is secured.

### Existing-owner phase

Use an event-aware message pump instead of relying solely on blocking GetMessage.
An event plus queue wait can wake without a successful thread message; check the
terminal latch before admitting initialization/render/refresh work and between
bounded dispatch batches so queued messages cannot starve terminal handling.
[MsgWaitForMultipleObjectsEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex).

After terminal latch, finish/unwind the currently executing owner operation,
release the normal renderer lock, and stop admitting graphics work. Hide the
owned window on its creating thread. Drain previously admitted game callbacks
without holding a lock those callbacks need. Only the owner may wait for callback
quiescence; wake plus authoritative count must prevent lost wakeups. Do not start
graphics teardown while a game callback remains blocked on that same mutex.

Then perform one guarded finalization path:

1. Withdraw accepted host presentation, disarm timer/demand and remove observers.
2. Reconcile bridge queue/rebind state; detach input subclass with the existing
   foreign-chain preservation policy. Binding locks are released before graphics
   release; terminal reentry uses the stable pass-through record.
3. Stop/join the pacing worker from the owner only, preserving current dependency
   ordering. If its termination cannot be established, retain resources it can
   reference; do not close handles under a live worker or report Complete.
4. Run ImGui/DComp teardown and then DestroyWindow for the owned window on owner.
   Handle partial initialization and already-destroyed windows. No completion wait
   is added to DComp; DestroyWindow is constrained to its creating thread.
   [DestroyWindow contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroywindow).
5. Publish actual cleanup outcome, clear the externally published owner thread ID,
   and publish Stopped only after finalization. Any restart marker must remain
   terminal; clearing workerStarted alone must not authorize a new owner.

An outer C++ boundary/finalization guard must cover setup, initial RefreshPresenter,
the pump and normal exit, not just individual loop branches. Guard independent
cleanup steps against secondary C++ exceptions. Existing noexcept methods still
need inspection: terminate, access violation and blocked driver calls are not
recoverable merely by surrounding them with another catch.

## 4. Who handles a request during exit?

| Owner state | Required disposition |
| --- | --- |
| Starting / no queue yet | Terminal bit and pre-created event persist. Owner skips further resource creation and executes its finalizer. No game wait for queue readiness. |
| Running / queued post fails | Event-aware owner consumes durable terminal state; queue delivery is optional. |
| Stopping / concurrent request | Finalizer already owns unconditional cleanup; request is idempotent and cannot reopen admission or schedule a second teardown. |
| Stopped after Complete | No new graphics work possible; late request remains terminal and does not need a live consumer. |
| Stopped with Retained/Unverified | No surrogate replacement owner or game-thread cleanup. Preserve resource status; do not claim the request was serviced successfully. |
| Owner hung, forcibly terminated, or process terminating | No unconditional completion guarantee. Logical input gate remains closed; physical hide/resource release may be unverified. No waits/retries/loader-lock cleanup are introduced to conceal this limit. |

**CH05 remains explicit:** a terminal Overlay WndProc returning DefWindowProc does
not forward a native mouse click to the game underneath. A separate visible HWND
can still hit-test until the owner hides it. G0 proves passive styles work, not
instant pass-through when the owner cannot run. Do not promise immediate restoration
of every OS mouse route from an atomic latch alone. Any stronger cross-thread
hide/style-change recovery would be a separate design requiring evidence.

## 5. Required validation before integration — not run

First validate the narrow handoff in an isolated surrogate fixture, not the game:

- C++ failure before/after WndProc decision, while bridge/rebind/renderer locks
  are occupied: game failure transition takes none and cannot invoke GPU cleanup.
- Queued stale toggle, Activate and event-drain races: terminal never reopens
  effective ownership or emits a stale handled decision.
- Last in-flight callback versus owner finalizer: no lock inversion, lost wake,
  premature release, binding use-after-free or duplicate Shutdown.
- Queue absent/full/post failure; terminal during Starting, Stopping and after
  Stopped: finalization is independent of posted-message acceptance.
- Partial resources, secondary cleanup exception, retained pacing resources,
  foreign subclass, HWND identity reuse and delayed cursor restoration.
- Assert owner thread IDs for ImGui/DComp release and HWND destruction; record
  actual outcome rather than only observing a cleanup request or closed thread.

These are proposed handoff checks, not executed G1–G5 tests. Immediate logical
revocation and eventual physical hide must be observed separately. The existing
normal game-WndProc renderer mutex remains a normal-path blocking boundary;
this protocol must not be sold as removal of all game-thread locking or stalls.

## 6. Disposition

The current DisableAfterOverlayException cannot satisfy the requested handoff
unchanged. A bounded split is credible, with explicit terminal admission,
nonlocking revocation, durable wake and one owner finalizer. It also requires
stable continuation lifetime and an in-flight callback drain contract; simply
posting Renderer::Shutdown to the owner would leave those races unresolved.

Recommend review/approval of this narrow protocol and its isolated failure fixture
before G1 or Renderer integration. No new ASI is needed for that step. No production
code, tests, configuration, build inputs, vendor or release assets were changed;
no builds, tests or game launches were run in this review.

Reviewed source SHA-256:

- composition_runtime.cpp: `75071D3C1A08DDD831C7C788EF206A95EE18A4436F3459372B81387D10E037B5`.
- input_state.cpp: `FAECEBCC5D0E89344FC893B20A13B350C0B695B88C1D2392C54E9FBE690E3AA4`.
- renderer_runtime.cpp: `C18840B951FC88713CAB1277FD2728697896EC96BFA62943AA948FD3F12F9D07`.

Production root detach, the pinned `78357…624E`, all target-release experiments,
and established N03–N06 evidence statuses remain unchanged.

## Subsequent authorized fixture result

The user separately authorized implementation and local tests for the isolated
failure-path fixture. Its ten subprocess scenarios passed twice, including the
mutex-held callback race, real full message queue, partial startup, late/stale
callbacks and truthful retained-resource outcome. See
[fixture result](OVERLAY_OWNED_HWND_FAILURE_HANDOFF_FIXTURE_RESULT_2026-10-13.md).
The source review above remains historical; production handoff is not implemented
or verified by the fixture. G1/Renderer integration remain unapproved.
