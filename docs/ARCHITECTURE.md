# Production Architecture

## Runtime ownership

The translation-unit-private `RuntimeState` in `src/plugin/runtime.cpp` is the
primary integration and lifetime owner for production runtime state: module identity,
configuration, workers, installed hooks, feature availability,
cinematic/gameplay transition state, dialogue state and runtime telemetry. It
is intentionally not a public `plugin::RuntimeState` type. Callback functions
use local `g_*` aliases to fields in that owner; those aliases are not
callbacks or separate ownership boundaries. Lifecycle fields are changed through one
externally serialized Runtime control path; worker code only observes the stop
event.

Domain modules own reusable calculations, policies and resolver primitives.
The translation-unit-private `RuntimeState` owns their integration, lifecycle
and cross-domain coordination. A separate coordinator type is not required
unless it can own an independent state machine without receiving the entire
runtime as callbacks.

User-facing feature configuration and lifecycle capability are separate
concerns. A domain may install an observation-only dependency for another
domain without enabling its presentation intervention. A dependent
non-Native Dialogue lifecycle is available only when its cinematic lifecycle
observation and Gameplay recovery observation capabilities are available;
otherwise it fails closed while unrelated feature status remains independent.

The production ASI includes the combined D3D12/ImGui settings overlay and its
embedded localization catalogs/font profiles. Locale configuration is either
Auto or an explicit registry identity. Auto reads the game's current
Interface Language when selected and at the overlay closed-to-open transition, normalizes
it through the locale registry, and updates the effective catalog and font
profile; unknown/unavailable values use English without changing Auto mode.
Manual locale selection remains authoritative until the user selects Auto.
There is no per-frame language polling or separate localization state owner.
The overlay toggle and window position are stored in the main
`STALKER2CameraTweaks.ini` under `[Overlay]`. Existing `Hotkeys.OverlayToggle`
and the former `STALKER2CameraTweaksOverlay.ini` position are migrated once;
the old placement file is then ignored and is not deleted.

The production overlay lifecycle log is
`STALKER2CameraTweaksOverlay.log`. Repeated DXGI factory/PRESENT observations
and detailed association telemetry are emitted only while `Diagnostics.Enabled`
is true; normal logging focuses on initialization, swapchain selection,
visibility, actual resize/rebuild events, font changes and failures.
Caller-module and factory-table slot traces are compiled only into the
diagnostic build profile; they are not present in the production artifact.

DXGI export hooks are created disabled, assigned to their process-resident owner,
then enabled. COM method hooks capture SDK-bounded original entries and publish
their table registry record before atomically patching selected entries in the
original table. They do not truncate foreign vtables or retain COM objects in
the registry. Registry access is synchronized; callbacks keep shared record
leases. Distinct queried interface tables are covered independently.

The renderer releases its owning swapchain/backbuffer references before native
replacement for the same HWND. Resource recreation is not terminal disable.
Both `ResizeBuffers` and `ResizeBuffers1` release matching renderer resources
before native resize, without holding registry/renderer locks across the native
call. Nested native resize calls defer recovery until the outermost call returns.
A changed/multiple present-queue assignment fails closed for Overlay only;
the native HRESULT and arguments remain unchanged. Renderer/ImGui access is
serialized independently of the log/evidence registry. Failed DXGI creation
results are logged even when optional diagnostics are disabled.
GPU fence waits, including the locally adapted ImGui font-upload backend, are
bounded to one second and validate actual fence completion. A timeout or removed
device disables Overlay and skips a second blocking teardown wait. If submitted
work has no completion proof, one terminal set of GPU COM refs/event is deliberately
retained until process exit, while CPU UI state is detached; forcing release to
make native resize/replacement succeed would risk GPU use-after-free. Such native
operations can still fail with retained buffers. Backend vertex/index uploads
use the same fence-waited backbuffer slot as the renderer allocator, rather than
an independent draw-call ring. TEST/nonblocking Present calls are native-only.
The core camera runtime has no dependency on renderer readiness.
Export hook owners are process-resident: CRT detach must not invoke SafetyHook
thread suspension/unpatching under loader lock. Terminal-disabled input remains
native pass-through; cursor restoration is posted to the window thread. WndProc
chain snapshots/publication are serialized with renderer/input state.
Combined startup synchronously arms optional DXGI factory observation before
core camera initialization so it can see the game's initial presentation
factory. A bounded seed `IDXGIFactory2` is created only to install observation
on a widest-supported, image-backed shared factory table; its COM reference is
released after setup and it is never a renderer target. Factory export hooks
remain supplemental discovery, not readiness evidence. If the seed table or
its method owners cannot be pinned safely, discovery stays unavailable rather
than claiming coverage. Only real successful `CreateSwapChain*` callbacks
provide the swapchain and queue/device association. Renderer and input
activation remain gated on core readiness, validated target ownership and two
successful Presents. Nested proxy/native creation of the same canonical COM
identity is observed once per callback chain. A C++ failure arming discovery is
contained and logged, and does not block camera initialization.

## Build and test policy

`build.cmd` is the supported production build entry point. It discovers the
documented Visual Studio toolchain, uses C++23-era MSVC facilities and applies
`/W4` to the production compilation. Warnings are reviewed but `/WX` is not
currently required because vendor sources are part of the same bounded build
and do not have the project's warning policy. The production output
`STALKER2CameraTweaks.asi` includes the settings overlay, embedded locale
resources, and bounded Auto language reader. `tools/build/build-overlay-settings.cmd`
is retained as a compatibility alias to the same production build.

The production build selects overlay integration with `OVERLAY_PRODUCTION`;
`OVERLAY_SETTINGS_FRONTEND` enables the settings UI, and `OVERLAY_COMBINED`
connects discovery with the plugin runtime. The old standalone
`tools/build/build-overlay-poc.cmd` entry point retains `OVERLAY_RENDERING_POC` as a
legacy source gate; production does not define that POC-named macro.
`tools/build/build-diagnostic.cmd` creates a separate supported diagnostic ASI. The
standalone POC/discovery and research watcher scripts are retired research
entry points, not supported ASI profiles or contents of the production
package.

Production builds emit a bounded `STALKER2CameraTweaksStartup.log` startup
journal containing process/worker startup, factory-bootstrap and DXGI coverage
status, camera-core readiness, first factory/swapchain observations, pending
target acceptance, the two successful activation Presents, renderer/input
activation, and first Overlay frame. It records no module/export archaeology,
instruction/jump decoding, or per-Present telemetry. The heavyweight forensic
trace-v2 is separate and opt-in with `CAMERA_TWEAKS_STARTUP_TIMELINE=1`; it
replaces the production journal for that diagnostic build.

`test.cmd` is the single repository test entry point. It builds and runs all
current Windows harnesses with the same compiler family and returns failure if
any harness fails. Test outputs are generated only under the ignored
`build-artifacts/` directory.

## ASI lifetime contract

The supported production model is **loaded until process termination**.
Normal dynamic `FreeLibrary`/manual ASI unload is not claimed as supported.

- `DllMain(DLL_PROCESS_ATTACH)` starts initialization asynchronously and does
  not retain a joinable initialization handle.
- Runtime workers and installed hooks are owned by the runtime state.
- Controlled shutdown must occur outside the loader-lock boundary: signal the
  workers, join them, then reset hooks and restore state.
- `DLL_PROCESS_DETACH` on process termination performs no complex teardown.
- Normal detach only sends the stop notification and must not perform joins,
  hook reset or memory restoration under loader lock.
- The runtime owner is deliberately process-resident and is not destroyed by
  DLL static teardown. Normal dynamic unload remains unsupported.

Loaders and integrations must keep the ASI resident until process termination
unless a future implementation establishes a non-loader-lock owner for a
complete controlled unload.
