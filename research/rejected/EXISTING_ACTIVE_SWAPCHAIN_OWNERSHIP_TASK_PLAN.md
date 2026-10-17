# Existing Active Swapchain Ownership — Task Plan

Status: `PLANNED / BLOCKED PENDING GATE A`.

## Objective

Determine whether an independently identifiable, already-existing
`IDXGISwapChain*` can be acquired safely and tied to the active S.T.A.L.K.E.R.
2 presentation path without extending factory interception or modifying COM
vtables.

## Established evidence and current state

- DXGI factory exports are intercepted safely.
- A per-instance `IDXGIFactory2` clone with the documented 25-entry ABI was
  installed without a crash.
- Correctly instrumented `CreateSwapChain*` methods produced no callback.
- The observed factory creation path is exhausted; this does not prove that
  all possible swapchain acquisition paths are absent.
- No active swapchain pointer, device provenance or game presentation
  ownership has been established.

## Approved scope

### Gate A — acquisition anchor

Find one concrete current-build or runtime ownership path that yields an
already-existing `IDXGISwapChain*` without guessing, memory scanning or
modifying COM vtables. The source must be independently identifiable, such as
an engine/window object retaining the pointer or another documented object
identity bridge. Allowed evidence classes are explicitly:

- current-build engine/window ownership chain;
- documented OS/DXGI runtime ownership API that returns or identifies the
  existing object;
- independently proven retained COM-object bridge.

### Gate B — ownership proof

Only after Gate A passes, verify the acquired object through:

```text
existing swapchain
    ↓
game window/presentation association
    ↓
IDXGISwapChain::GetDevice(ID3D12Device)
    ↓
active game ownership
    ↓
STOP
```

Dimensions, format and a successful `QI` are corroborating evidence only; they
do not establish ownership alone.

## Explicit non-goals

- No new `CreateDXGIFactory*` interception.
- No `Present` or `Present1` observation.
- No COM vtable modification or guessed vtable/object layout.
- No global pointer or memory scan for swapchains.
- No broad renderer/DXGI archaeology.
- No queue provenance beyond what is needed for Gate B device verification.
- No command submission, render resources, ImGui or notifications.
- No camera/FOV source changes and no stable-release changes.

## Files or areas expected to be touched

- A bounded research report under `research/reports/`.
- A separate diagnostic source only if Gate A has a concrete safe anchor.
- A dedicated test artifact only if a safe diagnostic implementation is
  justified.
- Existing camera/FOV source, production ASI, release assets and the closed
  factory-interception evidence remain untouched.

## Batches and validation

### Batch 1 — Gate A acquisition-anchor inventory

- Enumerate only concrete candidate sources already identified by current-build
  evidence, a documented OS/DXGI runtime API or an independently proven
  retained COM-object bridge.
- For each candidate, record pointer provenance, owner/object identity and the
  exact reason it can yield an existing `IDXGISwapChain*`.
- Reject candidates based only on dimensions, arbitrary COM pointers, guessed
  globals, displacement matches or generic renderer math.

Validation: candidate source is reproducible, bounded and does not require
factory expansion, `Present`, vtable patching or broad scanning.

### Batch 2 — Gate B ownership proof

Run only after Gate A PASS:

- verify game HWND/presentation association where applicable;
- call `GetDevice` on the acquired swapchain;
- record pointer identity and descriptor corroboration;
- classify `ACTIVE_GAME`, `AMBIGUOUS` or `NON_GAME`;
- stop immediately after ownership proof.

## Risks and rollback / safe failure

- An arbitrary or stale COM pointer can look valid temporarily.
- Multiple presentation objects may exist for game, helper or frame
  generation layers.
- Any candidate requiring guessed offsets, object layouts or shared-table
  changes is disabled/rejected.
- If a diagnostic implementation is added, failure must disable only that
  subsystem and leave the production camera/FOV core untouched.

## Stop conditions and phase gates

- Gate B is blocked until Gate A provides a concrete swapchain pointer source.
- Stop and mark `BLOCKED / DEFERRED` if no independent acquisition anchor can
  be identified without `Present`, factory expansion, broad renderer scan or
  guessed global pointers.
- Stop on ambiguous ownership, invalid COM identity or any crash/access
  violation.
- Stop after Gate B ownership PASS; renderer lifecycle is a separate task.

## Expected final review

- Record Gate A and Gate B separately as `PASS`, `PARTIAL`, `BLOCKED` or
  `REJECTED`.
- Record exact evidence and validation limits in a research report.
- Confirm stable camera/FOV source and release assets were untouched.
- Do not stage, commit, reset, checkout or otherwise modify Git state.
