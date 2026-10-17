# DXGI COM-Safe Swapchain Interception — Task Plan

Status: `PLANNED / BLOCKER FOR D3D12 NOTIFICATION OVERLAY`.

## Objective

Obtain a safe, current-build `IDXGISwapChain*` interception point and prove
that it belongs to the active S.T.A.L.K.E.R. 2 game presentation path, with
explicit provenance for the associated D3D12 device and command queue.

## Established evidence and current state

- DXGI factory export interception loads safely and observes repeated
  `CreateDXGIFactory*` calls.
- Direct use of `SafetyHook::VmtHook` on DXGI COM objects caused an access
  violation in `dxgi`; that strategy is rejected for this task.
- The safe bootstrap intentionally does not modify COM object vtables and has
  not reached swapchain, device or queue provenance.
- No ImGui, Present rendering, notification queue or F9/F10 integration is
  allowed until this ownership task passes.

## Approved scope

Investigate bounded COM-safe acquisition mechanisms:

1. Per-instance vtable clone with no RTTI assumptions and validated vtable
   length.
2. Concrete DXGI function or IAT interception.
3. Factory interception before COM object use.
4. Another current-build swapchain provenance anchor, if independently
   identified.

Preferred identity proof for a D3D12 creation path:

```text
CreateSwapChain*(pDevice, ...)
    ↓ QI
ID3D12CommandQueue = Q
    ↓ Q->GetDevice()
queueDevice = Dq

returned IDXGISwapChain = S
    ↓ S->GetDevice()
swapchainDevice = Ds

Dq == Ds  →  COM identity match
```

This causal chain is preferred over treating an arbitrary creation argument
or queue pointer as merely plausible. For `CreateSwapChainForHwnd`, the HWND
must also be associated with the S.T.A.L.K.E.R. 2 game window; dimensions are
corroborating evidence, not ownership proof by themselves.

If a per-instance clone is used, it may clone only an explicitly identified
DXGI interface using the exact method span required by that interface ABI.
There must be no guessed extra entries, RTTI-derived prefix assumptions or
"copy enough vtable slots" logic. The private cloned table may replace only
that object's `lpVtbl`; shared/read-only DXGI tables must never be modified.

Required lifecycle order:

```text
safe factory interception
    ↓
observe CreateSwapChain* calls
    ↓
identify returned swapchain
    ↓
preserve creation arguments
    ↓
classify device/queue relationship
    ↓
prove active game presentation ownership
    ↓
STOP
```

## Explicit non-goals

- No `Present` hook in this task.
- No `ResizeBuffers` hook in this task.
- No command submission or render resources.
- No ImGui or notification rendering.
- No F9/F10 integration.
- No mouse/controller input.
- No camera/FOV source changes.
- No `winmm.dll` proxy.
- No broad renderer or matrix scan.
- No global/shared read-only DXGI vtable patching.

## Files or areas expected to be touched

- A separate experimental source under `src/`.
- A dedicated build script under `build-artifacts/test-scripts/`.
- A test ASI under `build-artifacts/test-asi/`.
- A research report under `research/reports/`.
- Existing camera/FOV source and stable release assets must remain untouched.

## Batches

### Batch 1 — COM-safe factory/swapchain interception

Allowed:

- Implement one bounded COM-safe interception mechanism.
- Observe each relevant `CreateSwapChain*` call.
- Preserve the creation device/queue argument and swapchain result.
- Query the creation argument for `ID3D12CommandQueue`, when applicable, and
  call `queue->GetDevice()`.
- Query the returned swapchain for its D3D12 device.
- Compare queue-device and swapchain-device COM identity.
- Prove the HWND/window association for `CreateSwapChainForHwnd`.
- Log factory interface/version, API variant, creation argument, queue/device
  addresses, swapchain, HWND, dimensions, format, buffer count, swap effect,
  flags, COM identity match and final classification.

Acceptance requires all of:

- Safe `IDXGISwapChain*` acquisition.
- Provenance to a game-created presentation path, not launcher/helper or an
  unrelated Frame Generation chain.
- D3D12 device provenance from that same swapchain.
- Explicit queue provenance from the same `CreateSwapChain*` argument or an
  equivalent object-identity bridge.
- `queueDevice == swapchainDevice` by COM identity, or a documented equivalent
  proof when the API variant does not expose the same argument shape.
- Final classification is `ACTIVE_GAME`, `AMBIGUOUS` or `NON_GAME`; only
  `ACTIVE_GAME` is a PASS.
- No crash and no modification of shared/read-only COM vtables.

STOP immediately after this provenance PASS. Do not add Present handling in
the same batch.

### Batch 1.1 — Factory path versus timing classification

If factory creation is observed but no swapchain callback occurs, perform one
additional bounded observation pass only:

- log the exact requested factory IID and returned interface pointer for each
  `CreateDXGIFactory*` export;
- record the safely queried `IDXGIFactory2` pointer and whether it was already
  cloned;
- identify the concrete ABI used by each clone (the current implementation
  uses the explicitly queried `IDXGIFactory2`, 25 methods; relevant slots are
  `10=CreateSwapChain`, `15=CreateSwapChainForHwnd`,
  `16=CreateSwapChainForCoreWindow` and
  `24=CreateSwapChainForComposition`);
- distinguish a new factory/interface path from an already-existing swapchain
  that predates ASI initialization.

If all observed factory instances are safely cloned and no `CreateSwapChain*`
callback occurs, classify the factory-hook branch as a bounded
likely-pre-existing-swapchain result and stop. Do not add a generic DXGI scan
or renderer hook.

### Batch 2 — Rejected/partial decision

- If the concrete DXGI interface or its required ABI method span is uncertain,
  reject the clone implementation; never infer a span from memory scanning.
- If only a non-game or ambiguous swapchain is observed, record partial/blocked
  evidence and stop.
- If safe interception requires patching shared DXGI implementation without
  instance ownership, reject the approach.
- If no COM-safe path is found, archive this task as bounded blocked and keep
  the renderer feasibility task deferred.

## Validation

- Build the experimental ASI with the dedicated script.
- Load it only in the controlled test setup.
- Capture the dedicated log through game startup and renderer initialization.
- Confirm the game remains stable.
- Confirm the observed swapchain corresponds to the active game presentation
  path using creation arguments, dimensions/window association and lifecycle
  identity.
- Do not treat a successful build or any arbitrary swapchain pointer as PASS.

## Risks and rollback / safe failure

- COM interface vtables may differ by requested interface and may not have a
  standard RTTI prefix.
- Multiple swapchains may exist for the game, helper components or overlays.
- An incorrect hook signature or slot can corrupt the caller and crash DXGI.
- Any ambiguous ownership, uncertain vtable length or invalid pointer must
  disable the experimental subsystem for the session.
- The experimental ASI must not contain camera/FOV functionality and must not
  become a dependency of the production ASI.

## Stop conditions and phase gates

- Stop on executable identity uncertainty if a current-build code anchor is
  needed.
- Stop on any crash or access violation.
- Stop if the concrete DXGI interface or exact ABI method span for a
  per-instance clone cannot be established.
- Stop if the swapchain cannot be tied to the game presentation path.
- Stop if queue/device COM identity cannot be established without guessed
  global queue hunting.
- Stop after device and queue provenance PASS; renderer lifecycle is a later
  task.

## Expected final review

- Compare changed paths with this plan.
- Confirm stable camera/FOV source and release assets were untouched.
- Record whether the result is PASS, PARTIAL, BLOCKED or REJECTED.
- Record runtime validation limits and the exact swapchain/device/queue
  evidence.
- Do not stage, commit, reset, checkout or otherwise modify Git state.
