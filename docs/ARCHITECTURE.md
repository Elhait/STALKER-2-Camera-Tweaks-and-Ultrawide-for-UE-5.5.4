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
