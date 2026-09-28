# DXGI startup safety repair

Authorized by the user's “Виправляємо все” following the release-code analysis.

## Contract and scope

Optional Overlay must not alter native DXGI creation arguments/results or keep
an old flip swapchain alive across native replacement for the same window.
Callbacks must have published originals before activation, hold registry leases,
and synchronize registry mutation. Both ResizeBuffers variants must release
Overlay backbuffers before native resize. Overlay startup, render and teardown
failures must remain local to Overlay; no GPU fence wait may block the game
thread indefinitely. Camera features continue through the normal runtime/INI
path. Camera semantics, settings, UI and localization remain unchanged. No game
launch, package update or Git publication.

## Batches

1. Correct CoreWindow ABI; introduce bounded COM entry publication and
   synchronized table-owned callback leases; publish disabled export hooks before
   activation. Validate SDK signatures and synthetic COM publication/address reuse.
2. Release the matching renderer before native replacement; serialize renderer
   access without holding a lock across native creation; handle ResizeBuffers1
   with matching-swapchain ownership and updated buffer/format configuration;
   unsupported queue reassignment disables Overlay, not native resize. Bound
   GPU fence waits and fail closed for Overlay on device loss or timeout without
   waiting again during teardown. Validate deterministic timeout/failure and
   replacement/resize contracts.
3. Run full test.cmd and production build to a separate ignored artifact path;
   review complete changed-path diff and whitespace.

Expected files: overlay discovery/renderer, a concrete DXGI hook ownership helper,
focused offline harness and its runner registration, architecture/safety docs.

## Evidence and risks

Confirmed source defects: CoreWindow signature mismatch, active-before-published
hooks, unsynchronized registry access, early owning swapchain reference with late
replacement teardown, missing ResizeBuffers1 interception. Runtime attribution
of the reported E_ACCESSDENIED/null-read crashes remains unconfirmed. Offline
fixtures do not prove real driver/wrapper/game compatibility. Never hold the
registry lock across a native callback; never dereference a COM object after its
final Release. Teardown and registry leases must handle reentrant callbacks.
Implementation refinement: patch only selected entries in the existing COM
table, rather than clone its public SDK prefix. Foreign implementations may
append private virtual methods. Table-owned registry records retain no COM
reference and cannot become stale when an object address is reused. Allocation
in evidence stores is allowed to reach the optional C++ exception boundary,
rather than terminate in an incorrectly declared noexcept method.

### Follow-up evidence: discovery timing and missing Overlay activation (2026-09-27)

The factory caller trace candidate ran without a startup crash, but Overlay did
not activate. The captured IDXGIFactory7 vtable slots 10/15/16/24 exactly
matched this module's CreateSwapChain hook addresses, ruling out a later slot
overwrite for those observed factory objects. No `FACTORY_METHOD_INVOKED`,
`SWAPCHAIN_DISCOVERED`, Present observation, input-hook installation or startup
hint appeared. CreateDXGIFactory2 calls originated from a distinct loaded
module also named `dxgi.dll`, consistent with a proxy/wrapper path; its exact
file identity was not present in the log. This establishes a discovery gap,
not yet runtime proof that the primary swapchain predates factory-hook setup.

Bounded follow-up: synchronously arm optional factory observation before
camera-core initialization, while explicitly gating renderer/input activation
until core readiness and the existing stable-target evidence. The diagnostic
candidate tests whether the observed gap was caused by late discovery. The
user-provided candidate runtime logs confirmed the sequence: factory hooks
armed, the expected D3D12 HWND swapchain and direct queue were observed, the
target passed device/queue/window checks, two successful Presents activated
renderer/input, and visibility toggled through Delete. No startup crash was
observed in that run. This confirms the candidate lifecycle in that environment;
it does not establish compatibility with every wrapper/driver combination.

## Status

Implementation and offline validation completed on 2026-09-27.

- All five confirmed source findings repaired; native callback arguments/results
  remain unchanged and optional Overlay failures retain defined teardown.
- GPU fences no longer use infinite waits on the render or teardown path. A
  timeout/device-removed fence fails only Overlay and prevents duplicate waits
  during cleanup; core camera runtime remains separately initialized.
- Nested matching resize callbacks defer recovery/failure disposition until the
  outermost native call returns, rather than rebuild during native execution.
- Latest test.cmd: 51/51 harnesses passed, including hooks-before-core startup
  ordering, core-readiness and stable-presentation activation gates, DXGI callback
  pass-through, Overlay failure isolation and bounded fence-wait policy fixtures.
- Localization audit: 18 catalogs / 182 keys passed; embedded resource/glyph/font
  coverage passed for supported font sizes 12–24.
- Production build passed to the separate ignored artifact
  build-artifacts/production-lifecycle/STALKER2CameraTweaks.asi. Only existing
  third-party Zydis/ImGui compiler warnings were observed. Factory caller/module
  and vtable-slot traces remain diagnostic-profile-only; verbose discovery and
  resize observations are gated by `Diagnostics.Enabled`.
- Changed-path review and git diff --check passed. Existing staged user images
  were preserved; released binaries and archives were not overwritten.
- No game was launched during the offline validation pass. Driver/wrapper
  compatibility beyond the candidate scenario and attribution/resolution of
  the reported startup crashes still require additional runtime evidence.
- Follow-up user runtime evidence confirmed the early-armed candidate's Overlay
  reached `OVERLAY_RENDER_TARGET_ACTIVATED`, renderer/input initialization,
  startup hint and VISIBLE/HIDDEN transitions. The production artifact still
  requires its own runtime confirmation; this repair step runs offline checks.
- Overlay and the game share the game's D3D12 device/queue. Bounded waits prevent
  Overlay code from waiting indefinitely on a hung fence, but a device-wide GPU
  hang can still make the game's native Present fail; offline isolation cannot
  guarantee recovery from that shared-device failure.
