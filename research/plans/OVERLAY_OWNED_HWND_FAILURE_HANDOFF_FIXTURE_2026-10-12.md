# Overlay-owned HWND — isolated failure handoff fixture

Status: implementation authorized for this standalone fixture only. No mod or
Renderer integration. No ASI, game, canonical suite or production build.

## Purpose

Exercise a one-shot terminal/admission protocol and owner-only cleanup with real
Win32 threads, message queues, events and test resources. Prove that the failing
game-thread path does not wait on a deliberately held renderer lock or join the
owner; that already admitted work drains before teardown; that queue delivery is
not the cleanup authority; and that `Complete` matches zero remaining resources.

## Fixture contract

- One 64-bit atomic gate: terminal bit plus in-flight callback count; CAS admission
  closes monotonically. A pre-created manual-reset terminal event provides durable
  owner wake. A drain event tracks admitted-callback quiescence. The owner queue is
  created before readiness is published.
- Game-side callbacks acquire admission only before optional work. Exception
  containment unwinds callback-local locks, latches terminal, requests the durable
  wake and returns without waiting for owner completion. Stale callback tokens
  invoke their saved continuation and cannot touch Overlay state.
- The owner skips/halts work after terminal, drains admitted callbacks without
  holding a lock they require, then independently finalizes each initialized test
  resource on its creating owner thread. Partial init is valid. Secondary cleanup
  failure yields Retained/Unverified if any resource remains alive.
- Windows message posting is a hint. The test injects absent/failed hint delivery
  and an actually saturated thread queue while the event still requests cleanup.
- Each mode runs in a fresh subprocess because terminal is monotonic and owner
  lifecycle is one-shot.

## Scenarios and acceptance

`race`, `startup`, `queue-full`, `partial-0`, `partial-1`, `partial-2`,
`cleanup-failure`, `stopping`, `late`, and `stale-token` each return zero only
when their invariant assertions pass. All waits have bounded test deadlines.

Accepted: no callback waits/joins/graphics cleanup; terminal cannot reopen input
admission; owner cleanup starts only when in-flight count reaches zero; wake works
when the message hint cannot; each resource release uses its creating thread;
complete means zero live resources; failed release remains explicitly retained;
cleanup executes at most once; late/stale callbacks cannot mutate or resurrect the
old owner.

## Validation and limits

Compile the standalone harness with the existing MSVC toolchain and `/W4 /WX`.
Run each isolated scenario and preserve executable/source hashes, per-case exit
codes and output under ignored `build-artifacts/`, with durable copies in the
dated evidence folder. No canonical tests/builds or game launch. A pass validates
only the fixture protocol. It does not prove the production WndProc binding,
driver/DComp calls, a forcibly hung owner, instantaneous native mouse pass-through,
or safe Renderer integration. After results, stop for review before G1.

## Execution result

Implemented as a separate native fixture and PowerShell runner. Build passed with
`/W4 /WX`; all ten scenarios passed in two consecutive full runs (20 subprocesses,
all exit 0). The queue-full case filled the Win32 thread queue to 10,000 posts and
observed error 1816, while event-driven cleanup completed. A deliberately retained
real event HANDLE resulted in `Retained`, with other resource releases continuing.
Full details and hashes are in
[`OVERLAY_OWNED_HWND_FAILURE_HANDOFF_FIXTURE_RESULT_2026-10-13.md`](../reports/OVERLAY_OWNED_HWND_FAILURE_HANDOFF_FIXTURE_RESULT_2026-10-13.md).

Fixture stage is complete. G1 and Renderer integration remain unapproved.
