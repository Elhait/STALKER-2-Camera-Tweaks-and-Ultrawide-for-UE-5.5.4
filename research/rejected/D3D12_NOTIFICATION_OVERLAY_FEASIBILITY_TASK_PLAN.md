# D3D12 Notification Overlay Feasibility — Task Plan

Status: `BATCH 1A BLOCKED — COM-VMT STRATEGY UNSAFE / SEPARATE FROM CAMERA CORE`.

## Objective

Prove that `STALKER2CameraTweaks.asi` can render short in-game
notifications through an optional D3D12/ImGui overlay without affecting
existing gameplay, cinematic or dialogue camera/FOV functionality.

## Established evidence and current state

- The Auto cinematic aspect cache hotfix is complete, runtime-validated at
  21:9 and 32:9, and released separately as v0.5.1.
- Existing F9/F10 runtime selectors already emit meaningful state changes, but
  users currently receive confirmation mainly through the log.
- The S2AE reference mod demonstrates a production D3D12/DXGI overlay in this
  game family, including Present, resize, swapchain recreation and device-loss
  handling. Its code and assets are not to be copied.
- The current Camera/FOV core must remain independent of any renderer/UI
  subsystem.

## Approved scope

### Level 1 feasibility only

- Locate or attach to the active game DXGI/D3D12 swapchain.
- Establish the matching D3D12 device.
- Establish the associated `ID3D12CommandQueue` with concrete provenance;
  guessed or global queue hunting is not allowed.
- Prove command submission with the smallest safe diagnostic render path.
- Add the ImGui backend only after swapchain, device and queue submission are
  independently established, unless it is demonstrably the shorter proof.
- Render one static notification:
  `Camera Tweaks overlay active`.
- Add logging for initialization, queue provenance, renderer failure,
  resize/recreation and teardown/reinitialization.
- Validate that renderer failure degrades to a warning and does not disable
  camera/FOV hooks.

## Explicit non-goals

- No interactive settings window.
- No mouse capture or input routing changes.
- No controller or Steam Input support.
- No INI editing from the overlay.
- No `winmm.dll` proxy or loader change.
- No replacement or modification of existing F9/F10 behavior.
- No camera/FOV logic controlled by the renderer.
- No requirement that the overlay initialize for the core mod to work.
- No production release integration before the feasibility gates pass.

## Files or areas expected to be touched

- A separate experimental overlay source file or module under the research/
  experimental area.
- A dedicated build script and test ASI output outside stable release assets.
- A research report under `research/reports/`.
- Existing camera/FOV source is out of scope unless a minimal notification
  event adapter is required after renderer feasibility is proven.

## Architecture contract

```text
Camera/FOV core
      ↓ emits notification event
Notification queue
      ↓
Optional D3D12 renderer
```

If the renderer is unavailable or fails:

```text
renderer failure → log warning → Camera/FOV core continues unchanged
```

## Batches

### Batch 1A — Minimum viable renderer

Current implementation sub-batch: blocked after runtime access violations from
the COM-VMT interception strategy. No renderer resources or visible rendering
were promoted.

Allowed work:

- Attach to the active swapchain.
- Establish the matching device and associated command queue.
- Prove command submission with a bounded diagnostic primitive or the minimal
  equivalent render path.
- Hook `Present`.
- Render the static three-second notification.
- Implement the minimum `ResizeBuffers` handling needed for resource teardown
  and recreation.

Validation:

- Normal gameplay with the overlay enabled.
- Overlay disabled or unavailable.
- At least one resize/window-mode/swapchain recreation case.
- Game log inspection for lifecycle transitions and failures.

Acceptance:

- Notification renders in-game.
- No crash during normal gameplay or the tested recreation event.
- Overlay can reinitialize after recreation.
- Camera/FOV behavior is identical with the overlay enabled and disabled.
- Renderer failure produces a warning only.

Stop conditions:

- Any camera/FOV regression.
- Renderer failure can terminate or block the core.
- Broad renderer search becomes necessary without a concrete current-build
  DXGI/D3D12 anchor.
- Implementation would require a `winmm.dll` proxy.

### Batch 1B — Encounter-driven lifecycle expansion

Allowed only if the actual current build presents the corresponding case:

- `Present1`.
- `ResizeBuffers1`.
- Swapchain replacement.
- Device loss/recreation.
- Nested presentation.
- Recursion protection.

This batch must not reproduce S2AE's full defensive stack in advance. Each
item requires a concrete current-build observation or failure signal. ReShade,
Frame Generation and other third-party compatibility are risks only here, not
Batch 1 acceptance gates.

Acceptance:

- The encountered lifecycle event is handled or safely disables the overlay.
- Camera/FOV core remains unaffected.

### Batch 2 — Event-driven notification integration

Allowed only after Batch 1 passes:

- Connect the existing notification queue to F9/F10 state changes.
- Render:
  - `Cinematic framing: <mode>`
  - `Dialogue zoom: <mode>`
- Preserve existing F9/F10 selectors and lifecycle behavior.

Validation:

- Each hotkey produces the correct notification.
- Notification timing and replacement behavior are bounded and stable.
- Camera/FOV behavior remains unchanged.

### Batch 2A — Core-only failure regression

Required before promotion:

- Deliberately disable or force failure of overlay initialization.
- Confirm gameplay, cinematic and dialogue behavior still pass.
- Confirm existing F9/F10 selectors continue to work unchanged.
- Confirm the failure is logged once and the overlay is disabled for the
  session rather than retried on every `Present`.

### Batch 3 — Feasibility decision

- Promote only if renderer lifecycle and core independence are proven.
- Otherwise archive the branch as a bounded negative/deferred result.
- Do not expand into an interactive menu in this task.

## Risks and rollback / safe failure

- DXGI/D3D12 hooks can conflict with Frame Generation, ReShade or other
  overlays.
- Swapchain recreation can invalidate command queues, descriptor heaps and
  render targets.
- Present recursion or nested presentation can cause stack exhaustion.
- ImGui or renderer initialization may increase Defender false-positive risk.
- All renderer hooks must be optional and removable on initialization failure.
- A terminal overlay failure must disable the subsystem for the current
  session, release owned resources/hooks safely and stop retrying every
  `Present`.
- Resize/recreation and device-loss paths are recoverable cases and may retry
  in a controlled manner after the new resources are valid.
- Experimental binaries must remain outside stable release assets.

## Stop conditions and phase gates

- Batch 1 must prove renderer lifecycle stability before F9/F10 integration.
- Batch 1A must establish swapchain, device and command-queue provenance
  before renderer promotion.
- `SafetyHook::VmtHook` is rejected for DXGI COM objects in this task until a
  COM-safe interception strategy is proven.
- Batch 1B is encounter-driven and must not become a preemptive compatibility
  project.
- No mouse/controller/input work in Level 1.
- No source production merge before runtime validation and core regression
  evidence.
- No broad renderer or matrix search without a new concrete anchor.
- Any core regression blocks promotion immediately.

## Expected final review

- Compare actual changed paths with this plan.
- Confirm stable camera/FOV source and v0.5.1 release assets were untouched.
- Verify experimental output is separate from production release files.
- Record runtime limits, completed items, remaining work and deferred menu
  functionality in the research report.
- Do not stage, commit, reset, checkout or otherwise modify Git state.
