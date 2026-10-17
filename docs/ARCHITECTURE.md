# Production Architecture

## Camera FOV semantics

Runtime FOV values can differ by up to ±1.0 degree through native recalculation
and interpolation while representing the same camera intent. The canonical
inclusive tolerance is `camera::kSemanticFovToleranceDegrees` in
`src/camera/fov_semantics.hpp`. Use it only when deciding whether two FOV
values express the same camera intent. Never infer camera source/provenance
from FOV proximity. Do not substitute it for stricter endpoint evidence
required by an active recovery/interpolation lifecycle. Keep numerical
precision, projection singularity, interpolation range, sample stability, and
test-output tolerances separate; a difference larger than 1.0 degree is a
reason to investigate, not proof of a transition.

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

## Persistent runtime diagnostics

Runtime diagnostics are persistent project infrastructure, not disposable
task-specific scaffolding. Instrumentation that has demonstrated practical
value for investigating camera, lifecycle, ownership, source or compatibility
behavior should remain available in the ordinary production ASI under the
standard Diagnostic Mode after the triggering issue is resolved. A future
investigation should reuse and extend these observation points instead of
recreating them by default.

Instrumentation and emission are separate concerns. Diagnostic Mode may
enable bounded observation snapshots, classification and logging; it must not
change camera output, aspect, lifecycle, ownership, association, correction
gates or state-machine timing. Emission should be event/change based and
semantically deduplicated where useful, not per-frame by default. When
Diagnostic Mode is off, diagnostic-only reads, snapshots, formatting and log
emission should be skipped; shared observation required by production
ownership/correction remains active independently of that mode.

Persistent runtime diagnostic marker names, symbols, types and filenames name
the runtime concept they observe, not a temporary finding number, remediation
phase or build-profile label. Finding IDs belong in remediation documentation
and historical evidence only. When an established marker is renamed, preserve
the old spelling only as historical evidence and document the current marker.

Remove or replace established instrumentation only with a documented,
evidence-backed reason: it is unsafe or invasive, has a material measured
performance cost, depends on a stale/unsafe runtime layout or signature, or
is fully superseded by a safer and more generally useful observation contract.
Keep the diagnostic capability when replacing its implementation, and update
the relevant tests and this contract. A temporary build profile may still be
used for instrumentation that cannot safely ship in the ordinary artifact;
that exception must be explicit and must not become the default for reusable,
bounded diagnostics.

User-facing feature configuration and lifecycle capability are separate
concerns. A domain may install an observation-only dependency for another
domain without enabling its presentation intervention. A dependent
corrected Dialogue lifecycle is available only when its cinematic lifecycle
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
The main INI is user-owned runtime data, not a release or Vortex-deployed file.
The mod creates it from the canonical template when absent and rebuilds known
legacy layouts while preserving compatible settings. Structurally malformed or
unrecognized INI content is backed up before recovery; safely readable section
assignments are retained, and a failed backup or atomic replacement leaves the
original file untouched. A canonical no-op startup does not rewrite the file.
INI transport accepts at most 4 MiB, rejects final-component reparse points,
and creates the staging file exclusively. An already occupied `.tmp` name is
not overwritten or deleted; persistence reports failure and preserves the INI.
Log writers validate regular single-link disk files before truncation. Runtime
and Overlay logs retain up to 16 MiB per opened session, emit a limit marker and
suppress subsequent records. A failed record or limit-marker write also suppresses
further writes for that session, because a failed operation may have written a
prefix whose length is not reflected by the complete-record counter.
The startup journal remains independently bounded
by its fixed event capacity. Parent-directory junctions remain supported; this
is not a security boundary against an attacker controlling the game directory.

The production overlay lifecycle log is
`STALKER2CameraTweaksOverlay.log`. The Overlay has no DXGI factory, game
swapchain, Present, Present1, ResizeBuffers or ResizeBuffers1 interception.
It discovers the process's visible top-level game window using Win32 window
enumeration and out-of-context window events, and waits for camera-core
readiness before creating any presentation resources.
The owner thread creates its message queue before publishing its thread ID.
Window-event refreshes are coalesced in an atomic pending signal and wake the
owner through its command event; a thread-message post is only a fallback when
the owner event cannot be signaled. The owner rescans the process windows when
that pending signal is consumed, so message coalescing does not discard the
refresh request. Discovery and refresh diagnostics are transition-based and
bounded under `[Diagnostics] Enabled=true`.

A private D3D11 device owns the ImGui renderer and draws into
`IDCompositionSurface` updates attached to an `IDCompositionTarget` for a
separate unowned top-level Overlay display HWND. The target never binds to the
game HWND. The display HWND is created and destroyed on the existing Overlay
owner thread, is non-activating and mouse-transparent, and follows the game
client rectangle, activation, minimization and rebind lifecycle. While shown for
the active game, the non-activating display uses the topmost z-band even when
the game HWND is not topmost: an accepted HWND_TOP placement is not sufficient
to keep an unowned display above the foreground host after restore. Inactive or
minimized UI remains hidden. Placement reuse and input admission also check the
actual relative game/display z-order; WM_WINDOWPOSCHANGED revokes capture and
queues owner-side reconciliation without retaining its borrowed WINDOWPOS.
Overlay frames never acquire or retain game backbuffers, devices, queues or
swapchains and do not execute on the game's native presentation path. The ImGui
UI/context, D3D11 immediate context, and composition objects are serialized on
one Overlay owner thread. The game window procedure is the
nonblocking input boundary. It admits callbacks through the terminal gate,
copies mouse and relative raw-mouse events into the bounded input bridge, and
queues keyboard/control events for the Overlay owner. Settings Apply/Persist/
Snapshot, rebind decisions, notification publication, Renderer access and file
logging run on the owner thread. The bounded multi-producer command queue
preserves accepted commands in reservation order; when admission fails or it
fills, the current event passes to the game and the optional Overlay enters
terminal fail-open shutdown rather than overwriting an accepted command.
Native game-language reading remains on the GameHwnd thread; its bounded result
is queued back for owner-thread application. The owner-aware message pump waits
on terminal and command events independently of message-queue capacity, then
drains admitted callbacks before owner-thread finalization. Composition
resources remain resident while hidden. The
owner detaches the target root after a successful final clear when the UI is
closed, no notification remains, and startup hint creation is complete. It
reattaches the prepared surface for UI or notification display before starting
notification lifetimes. Attachment state is published only after an accepted
SetRoot + Commit; failure disables the optional Overlay. The game window subclass
and Delete handling remain active while detached.
A presenter-owned frame scheduler is event-driven while idle and
uses the DirectComposition compositor clock for active notification animation
and dirty interactive frames where the OS exposes that API. A clean static
panel does not render continuously; input/settings changes coalesce into
owner-thread frames. On systems without the compositor clock, interactive
changes remain event-driven and only notification animation uses a bounded
timer fallback. Startup guidance never depends on settings visibility, game
frames, or presentation callbacks.
Startup guidance is eligible only after runtime initialization, presenter and
input-binding readiness, plus a successful game Interface Language read or the
documented bounded fallback. The read uses the existing game-window callback;
its one-shot 3000 ms response deadline is integrated into the owner wait and
does not add a polling worker. This is evidence that the game-owned setting was
read, not a claim that a stable scene or game frame has been presented.

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

Input capture is admitted only after the requested UI has a committed,
attached surface and the display HWND is visible for the active, non-minimized
game window. Opening requests remain fail-open until that frame is accepted.
Focus loss, minimize, geometry/DPI changes, host destruction/rebind and
terminal state immediately revoke capture; the owner re-establishes it after
the display is ready again. While an interactive display is admitted, the
owner publishes a 250 ms progress heartbeat. If it becomes stale for 750 ms,
the callback closes admission and returns input through the saved native
continuation. Delete continues through the game WndProc when capture is
unavailable. When open and ready, mouse ownership is transferred through the
bounded event bridge and returned on close. Esc dismisses the UI. If another
window subclass is installed above Camera Tweaks, its continuation record is
retained until that HWND is destroyed rather than overwriting the foreign
subclass. Renderer failures request terminal state instead of synchronously
tearing down graphics resources. The owner closes admission, drains admitted
callbacks, and then runs one guarded finalizer on the owning thread.
Independent cleanup steps continue after a step fails; DComp references and
the display HWND are retained when detach/commit or Renderer release is not
confirmed. Finalization is reported as `COMPLETE`, `RETAINED`, or
`UNVERIFIED` from observed cleanup state, with a `PENDING` marker before
teardown begins. This implementation has focused offline evidence; live
Renderer/driver teardown remains unverified.

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
The ordinary ASI built by `build.cmd` contains the approved passive runtime
diagnostics. `[Diagnostics] Enabled` in the INI is the runtime switch; enabling
it does not require a separate ASI. `tools/build/build-diagnostic.cmd` is an
optional compatibility wrapper for isolated diagnostic-profile work, not a
second normal user build. The standalone POC/discovery and research watcher
scripts are research entry points, not contents of the ordinary ASI.
Production and test targets compile ImGui without its default shell/file helpers
and optional XInput/gamepad backend. Clipboard and IME behavior remain available.
The hotkey worker observes the four configured action keys only when hotkeys
are enabled and the game process is foreground. Held keys are consumed when
returning to the game so they are not dispatched as fresh presses.

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

The initialization worker pins the ASI with GetModuleHandleEx(FROM_ADDRESS|PIN)
before installing hooks or starting Overlay. Failure aborts initialization.
After pinning, FreeLibrary cannot physically unmap its callbacks or resident
owners. Racing unload against the original asynchronous attach/initialization
handoff is still unsupported; loaders must allow initialization to run.

`plugin::Shutdown()` is a camera-core reset helper; production currently has
no caller for it and it is not a combined ASI-unload API. Initialization failure
can reset camera-core resources. Overlay terminal disable separately closes
input admission, drains admitted callbacks and finalizes its resources on the
Overlay owner thread. Destruction of GameHwnd permits host rebind and does not
by itself request terminal shutdown of the mod.

Ordinary process exit has no guaranteed pre-exit handoff to either owner. It
can terminate those threads before DLL detach, so explicit worker joins,
hook restoration, COM release and Overlay finalization are not promised for
that scenario. Process-owned handles, windows and address space are reclaimed
by Windows; this is not evidence that DWM or the graphics driver completed
their asynchronous work. Forced termination may omit DLL detach entirely.

The existing `OVERLAY_OWNER_FINALIZATION=STARTED outcome=PENDING`,
`OVERLAY_OWNER_FINALIZATION=COMPLETE|RETAINED|UNVERIFIED` and
`OVERLAY_RENDERER_OWNED_REFS_RELEASED` records describe a running Overlay owner
finalizer. They are not mandatory end-of-game records. The startup journal has
no detach writer. There is no production `MOD_SHUTDOWN_COMPLETE` guarantee.

The identified logging/configuration paths use Win32 file handles rather than
CRT FILE streams. The main synchronous spdlog logger is owned directly by
Runtime and never registered in spdlog's static registry. The Overlay log and
its mutex have process-lifetime storage; its owner can still explicitly close
the handle. INI transport uses the same Win32 primitive with staged flush/close
and the existing atomic replacement, without changing parsing/migration rules.
ImGui disk settings are disabled because all mod windows use NoSavedSettings
and placement belongs to the main INI.

The pending-notification queue/counter/mutex belong to the resident Runtime.
SafetyHook retains its mutable allocator weak-reference state and TrapManager
instead of registering their destructors with CRT. Its existing VEH checks
Windows process-shutdown state before accessing its mutex/map; the function
address is cached outside those locks before installing traps. No new shutdown
hook is introduced. Bounded process-lifetime retention is intentional; it does
not make manual unload a supported operation or promise driver completion.
The /MT CRT and host still have their own termination behavior outside these
identified paths. The bounded audit, exact repair scope and qualification
limits are recorded in
[`FINAL_ASI_SHUTDOWN_LIFECYCLE_CLOSURE_2026-10-17.md`](../research/reports/FINAL_ASI_SHUTDOWN_LIFECYCLE_CLOSURE_2026-10-17.md).
