# Overlay D3D12 POC — Discovery/Lifecycle Batch

Date: 2026-09-21  
Scope: optional post-v1 overlay POC, lifecycle, presentation discovery and
experimental rendering  
Runtime: discovery and association sessions performed; rendering not launched

## Result

The isolated overlay POC contains bounded runtime discovery telemetry and a
separate experimental rendering build. The production camera ASI remains
unchanged; graphics submission is enabled only by the experimental POC artifact.

```yaml
rendering_poc_implemented: YES
graphics_submission_enabled: EXPERIMENTAL_ONLY
production_rendering: NO
imgui_version: v1.91.9b
renderer_deterministic_tests: PASS
rendering_runtime: NOT_PERFORMED
runtime_validation_required: YES
```

This is ready for a controlled runtime discovery session, not evidence that a
usable queue relationship has already been established in the game.

The first discovery session established the factory and presentation stages:

```yaml
factory_export_interception: PASS
factory_objects_observed: PASS
swapchain_creation_interception: PASS
swapchain_creation_path: CreateSwapChainForHwnd
present_interception: PASS
resize_buffers_interception: PASS
queue_association: AMBIGUOUS_AFTER_RESIZE
rendering: EXPERIMENTAL_ONLY
```

The repeat log observed `CreateSwapChainForHwnd` and a swapchain with a valid
device and `DIRECT` queue candidate:

```text
width=4320 height=1440 buffers=6
device=<non-null> queue=<non-null> queueType=DIRECT
```

It also observed `Present` and several successful `ResizeBuffers` cycles,
including window sizes `4320x1440`, `4552x1280` and `5120x1440`. The first
implementation reported the queue association `AMBIGUOUS` after each resize
because it invalidated the candidate at `ResizeBuffers`. This was a bounded
resize-lifecycle evidence gap, not a missing swapchain hook.

The association lifecycle has since been repaired statically: a successful
resize now retains the creation-time `S↔D↔Q` structural evidence and marks it
`RESIZE_REVALIDATION_REQUIRED`; the next valid Present revalidates it as
`SUPPORTED`. Failed resize clears the association. A new swapchain, changed
device or multiple candidates cannot inherit or silently resolve the old
association. Rendering remains disabled.

## Implementation

The lifecycle model remains independent and fail-closed:

```text
Unavailable → Discovering → Ready
                         ↘ Resizing → Ready
                                      ↘ Failed
Failed → Discovering  (fresh recreation attempt)
any non-disabled state → Disabled
```

The experimental discovery ASI hooks the DXGI factory exports and the relevant
factory/swapchain COM methods needed to observe:

- factory and swapchain creation;
- creation object/device identity;
- `IDXGISwapChain::GetDevice(ID3D12Device)`;
- command-queue candidate identity and `D3D12_COMMAND_QUEUE_DESC::Type`;
- first and bounded early `Present` observations;
- back-buffer count/index;
- `ResizeBuffers` begin/end and post-resize observation windows.

The factory method slots are mapped to the inherited `IDXGIFactory2` layout:
`CreateSwapChain` 10, `CreateSwapChainForHwnd` 15,
`CreateSwapChainForCoreWindow` 16 and `CreateSwapChainForComposition` 24.
Method-specific invocation telemetry records the method, HRESULT and returned
swapchain identity. Factory observation is identity-suppressed while retaining
distinct interface/object identities and method coverage.

Queue candidates are recorded only when the creation object exposes a command
queue and the swapchain exposes a D3D12 device. Duplicate candidates are
ignored. A single candidate with at least one correlated Present is reported as
supported by the deterministic evidence layer; multiple candidates, missing
identities and missing Present evidence remain ambiguous/unsupported. No
candidate is selected for rendering.

## Isolation

The discovery path is built only by `build-overlay-discovery.cmd` as:

```text
STALKER2CameraTweaksOverlayDiscovery.asi
```

The normal `build.cmd` production source list and production artifact are
unchanged. No discovery state is read by production camera code. The discovery
build does not add ImGui, input interception, command-list creation, queue
submission, RTV/resource creation or a settings UI; those exist only in the
separate rendering POC build.

## Deterministic coverage

`discovery_evidence_harness` covers:

- bounded initial windows and re-arm;
- candidate identity acceptance;
- duplicate suppression;
- Present evidence requirement;
- single-candidate association;
- multiple-candidate ambiguity;
- resize retention and Present revalidation;
- repeated resize revalidation;
- new swapchain non-inheritance;
- changed-device ambiguity;
- failed-resize invalidation;
- swapchain recreation invalidation;
- null/zero identity fail-closed behavior.

The existing lifecycle harness remains unchanged. Both are included in
`test.cmd`; the full deterministic runner currently reports 35/35 harnesses.

## Validation

```yaml
deterministic_tests: PASS
test_cmd: PASS
test_cmd_harnesses: 35/35
production_build: PASS
experimental_discovery_build: PASS
experimental_rendering_build: PASS
git_diff_check: PASS
runtime_discovery: SWAPCHAIN_PRESENT_RESIZE_OBSERVED; REVALIDATION_PASS
association_repair_runtime: PASS
```

The builds retain the repository's existing vendor Zydis warnings; no new
error or overlay-specific warning is expected after the continuation fix.

## Runtime gate

The next controlled session must use only the rebuilt experimental discovery artifact
and inspect its bounded log. It must establish, for the actual STALKER 2
process:

1. the game HWND and active swapchain;
2. a valid D3D12 device from the swapchain;
3. the command-queue candidate set and queue type;
4. evidence that the candidate is related to the observed Present sequence;
5. back-buffer and resize/recreation behavior.

The association repair itself still requires one controlled runtime session.
Until that evidence exists, queue association remains:

```yaml
command_queue_association: STATICALLY_UNKNOWN_RUNTIME_OBSERVATION_READY
graphics_submission_enabled: NO
imgui_render_path_implemented: NO
runtime_validation_required: YES
ready_for_runtime_discovery: YES
```

The follow-up runtime confirmed the repaired lifecycle on one persistent
swapchain and device across repeated successful resize cycles. Every following
first Present recorded the same DIRECT queue and
`associationRevalidated=true`; subsequent Presents remained `SUPPORTED`.
No new swapchain or device identity was observed. The diagnostic field named
`associationBeforeResize` is currently emitted after the pending state is set,
so it reports `RESIZE_REVALIDATION_REQUIRED` rather than the pre-call
`SUPPORTED` state; this is a telemetry-labeling issue only. `RESIZE_END` also
needs a line-separator cleanup before future log-format use. Neither affects
the validated association behavior or enables rendering.

The discovery POC must stop at association discovery. A remembered first/last
`DIRECT` queue is not accepted as proof, and failure or ambiguity must leave
rendering disabled.

## Rendering POC implementation

The first rendering implementation is isolated in the experimental
`STALKER2CameraTweaksOverlayPOC.asi` build. It vendors Dear ImGui v1.91.9b
under `external/imgui/` and is built only by `build-overlay-poc.cmd`; the
production `build.cmd` source list and artifact remain unchanged.

The renderer is enabled only after the validated swapchain/device/DIRECT-queue
association is observed. It owns RTV and shader-visible SRV heaps, backbuffer
references, per-buffer command allocators, a command list, fence and fence
event. It submits one non-interactive static ImGui status window and does not
implement settings, hotkeys or input interception.

Resize handling waits for GPU completion, invalidates ImGui device objects,
releases backbuffers before `ResizeBuffers`, and rebuilds only after the next
supported Present revalidates the association. Identity changes, invalid
indices, resource failures and synchronization failures disable only the
overlay. Dear ImGui is isolated from the production artifact and covered by
the vendored license and third-party notice.

`renderer_state_harness` covers initialization, association gating, resize
suppression, successful rebuild eligibility, failed resize and terminal
disable/recreation behavior. The deterministic runner now reports 34/34
harnesses.

```yaml
rendering_poc_implemented: YES
graphics_submission_enabled: EXPERIMENTAL_ONLY
production_rendering: NO
imgui_version: v1.91.9b
renderer_deterministic_tests: PASS
rendering_runtime: NOT_PERFORMED
runtime_validation_required: YES
```

## Rendering validation gate

The rendering runtime gate has not been performed. The next controlled session
must use only the rebuilt `STALKER2CameraTweaksOverlayPOC.asi` artifact,
without the discovery or production ASI for isolation. Verify a visible static
window, stable frames, one resize/rebuild cycle, recovery after resize and no
crash/device-removal behavior. Inspect:

```text
OVERLAY_RENDER_INIT_OK
OVERLAY_RESOURCES_CREATED
OVERLAY_FIRST_FRAME
OVERLAY_RESIZE_RELEASE
OVERLAY_RESIZE_REBUILD_OK
```

## Interactive shell implementation

The experimental POC now includes a minimal input shell. `Insert` toggles
overlay visibility. While visible, Win32 keyboard and mouse messages are
forwarded to the Dear ImGui Win32 backend and consumed by the overlay; while
hidden, those messages continue through the game's original window procedure.
The shell does not install a raw-input hook, change cursor clipping or alter
camera/settings state. WndProc installation failure leaves the renderer
available but non-interactive and does not affect the production camera path.

```yaml
interactive_shell_implemented: YES
toggle_key: INSERT
settings_integration: NO
raw_input_hook: NO
production_input_changes: NO
input_state_harness: PASS
runtime_input_validation: NOT_PERFORMED
```

## Input capture repair

The initial shell unconditionally consumed common keyboard and mouse messages
while visible. That was too broad for the static POC. Capture now depends on
Dear ImGui's `WantCaptureKeyboard` and `WantCaptureMouse` flags; unrelated game
input continues through the original WndProc. Insert remains consumed only for
the visibility toggle.

```yaml
input_capture_scope: IMGUI_REQUESTED_ONLY
unrelated_game_input_blocked: NO_INTENDED
input_capture_harness: PASS
runtime_retest: REQUIRED
```

## Input capture repair

The initial shell unconditionally consumed common keyboard and mouse messages
while visible. That was too broad for the static POC. Capture now depends on
Dear ImGui's `WantCaptureKeyboard` and `WantCaptureMouse` flags; unrelated game
input continues through the original WndProc. Insert remains consumed only for
the visibility toggle.

```yaml
input_capture_scope: IMGUI_REQUESTED_ONLY
unrelated_game_input_blocked: NO_INTENDED
input_capture_harness: PASS
runtime_retest: REQUIRED
```

## Shared runtime settings boundary

The production runtime now exposes one typed mutation boundary for Gameplay
mode, Cinematic aspect policy, Cinematic FOV mode and Dialogue zoom policy.
F9-F12 use this boundary; persistence remains a configuration frontend
responsibility. No ImGui controls are connected yet, and no overlay copy of
settings was introduced.

```yaml
runtime_settings_api: IMPLEMENTED
hotkeys_migrated: F9/F10/F11/F12
overlay_settings_integration: NOT_PERFORMED
runtime_hotkey_validation: NOT_PERFORMED
```

## Shared runtime settings boundary

The production runtime now exposes one typed mutation boundary for Gameplay
mode, Cinematic aspect policy, Cinematic FOV mode and Dialogue zoom policy.
F9-F12 use this boundary; persistence remains a configuration frontend
responsibility. No ImGui controls are connected yet, and no overlay copy of
settings was introduced.

```yaml
runtime_settings_api: IMPLEMENTED
hotkeys_migrated: F9/F10/F11/F12
overlay_settings_integration: NOT_PERFORMED
runtime_hotkey_validation: NOT_PERFORMED
```

## Interactive shell implementation

The experimental POC now includes a minimal input shell. `Insert` toggles
overlay visibility. While visible, Win32 keyboard and mouse messages are
forwarded to the Dear ImGui Win32 backend and consumed by the overlay; while
hidden, those messages continue through the game's original window procedure.
The shell does not install a raw-input hook, change cursor clipping or alter
camera/settings state. WndProc installation failure leaves the renderer
available but non-interactive and does not affect the production camera path.

```yaml
interactive_shell_implemented: YES
toggle_key: INSERT
settings_integration: NO
raw_input_hook: NO
production_input_changes: NO
input_state_harness: PASS
runtime_input_validation: NOT_PERFORMED
```
