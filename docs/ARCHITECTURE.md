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

The production ASI includes the combined DirectComposition/D3D11 ImGui settings overlay and its
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
`STALKER2CameraTweaksOverlay.log`. The Overlay has no DXGI factory, game
swapchain, Present, Present1, ResizeBuffers or ResizeBuffers1 interception.
It discovers the process's visible top-level game window using Win32 window
enumeration and out-of-context window events, and waits for camera-core
readiness before creating any presentation resources.

A private D3D11 device owns the ImGui renderer and draws into
`IDCompositionSurface` updates attached to an `IDCompositionTarget` for the
game HWND. Overlay frames never acquire or retain game backbuffers, devices,
queues or swapchains and do not execute on the game's native presentation
path. The ImGui UI/context, D3D11 immediate context, and composition objects
are serialized on one Overlay owner thread. The game window procedure is the
input boundary: while the settings UI is open it copies keyboard, mouse and
relative raw-mouse events into a bounded bridge; only the Overlay owner thread
applies them to ImGui. The independent composition surface remains attached
while hidden. A presenter-owned frame scheduler is event-driven while idle and
uses the DirectComposition compositor clock for active notification animation
and dirty interactive frames where the OS exposes that API. A clean static
panel does not render continuously; input/settings changes coalesce into
owner-thread frames. On systems without the compositor clock, interactive
changes remain event-driven and only notification animation uses a bounded
timer fallback. Startup guidance never depends on settings visibility, game
frames, or presentation callbacks.

Surface changes use a generation transaction: the replacement surface is
created, fully drawn and committed before it is attached to the visual and
published as the active generation. A failed draw/commit does not publish a
partial generation; the optional Overlay disables itself on unrecoverable
composition errors. Minimize suspends rendering without discarding the active
generation; restore and size changes resume through the owner thread. Device
loss permits one bounded recreation attempt, after which only the Overlay is
disabled. No GPU fence wait or synchronous wait on game presentation is used.
The camera runtime and its INI configuration remain independent of presenter
readiness or failure.

Input capture is activated independently from renderer generations after a
committed composition surface and game HWND are ready. When closed, input
passes to the game; when open, mouse ownership is transferred through the
bounded event bridge and returned on close. Delete toggles the settings UI and
Esc dismisses it. Terminal Overlay failure returns messages through the saved
window-procedure continuation. If another window subclass is installed above
Camera Tweaks, its continuation record is retained until that HWND is
destroyed rather than overwriting the foreign subclass.

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
journal containing process/worker startup, camera-core readiness, game-window
discovery, composition device/surface commits, generation changes, input
activation, and the first Overlay frame. It records no module/export archaeology,
instruction/jump decoding, or per-Present telemetry. Camera/runtime forensic
tracing remains separate and opt-in with `CAMERA_TWEAKS_STARTUP_TIMELINE=1`.

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
