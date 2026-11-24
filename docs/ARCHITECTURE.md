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

## Camera State FOV publication

Pending Dialogue candidates are hypotheses, not confirmed camera ownership.
The primary writer can terminate a stationary candidate without another setter
callback when its validated receiver, Dialogue settings epoch and cinematic
generation match, its start/target and current native sample agree with a
still-current production Gameplay reference, and aspect/context gates agree.
This runs before writer classification so the hypothesis cannot invalidate that
same trusted reference first. No elapsed-time/sample-count expiration, foreign
baseline reuse or reconstruction of an already non-current baseline is involved.
Active Dialogue, genuine descent, exclusion and unknown ownership retain their
existing guards. This terminal boundary does not identify save/world lifetime.

Writer classification captured before a recovery transaction is provisional.
When that transaction completes source-owned native recovery and commits
Gameplay with exclusion cleared, its Gameplay observation uses GameplayEligible
rather than the retired RecoveryOwned tag. Provider handoffs and uncompleted or
foreign recovery cannot use this admission path. This ordering keeps the fresh
native reference usable even if a setter arrives immediately after completion,
before any further Gameplay writer. If cinematic recovery finishes before the
exclusion sees a validated departure and return, the returning writer uses the
newly released scope in the same transaction.

Camera State projects production evidence; it does not select or reconstruct FOV.
`gameplayFov` is the admitted Gameplay reference from Authority, independently of
`neutralGameplayFov`, which retains its zoom-neutral qualification. A confirmed
transition can retain a usable reference; receiver replacement or a production
invalidation cannot be repaired by UI history. Settings invalidate this reference
only through its production owner, not an invented overlay epoch.

`nativeFov` is an original capture at a named boundary (Gameplay writer, Dialogue
setter, authored Cinematic ENTER). A retained ENTER target is not a live native
blend trajectory. Prepared `writerInputFov` may already contain upstream mod
correction and cannot refresh an unknown original capture.

The legacy `currentOutputFov` member represents **Planned Output**. Effective FOV
comes only from existing diagnostic post-store/post-helper observations matched
by writer transaction, receiver, output object, thread and observation generation.
That generation is instrumentation identity, not a native receiver lifetime.
Both readback boundaries remain separately inspectable. Snapshot sequence and
numeric equality are not correlation. No readback with Diagnostics OFF means
Effective is Unavailable; it does not disable correction. No field claims final
rendered projection or global writer coverage.

## Cinematic writer ownership

Cinematic source ownership belongs to the accepted generation and its
confirmed aspect-store target. The validated native source-to-output copy
admits the destination for that invocation; output identity may change between
calls and is not a persistent camera identity. Initial establishment and
continuation use the same per-call admission evidence. EXIT may snapshot the
last admitted destination only when its generation and source still match;
that value is historical metadata, not proof that the destination remains
active or allocated. Gameplay recovery is authorized by generation/source and
uses the current writer invocation, never a cached cinematic destination.
A conflicting aspect-store target, stale generation, disabled correction or
invalid output fails closed without replacing the last admitted evidence.

Cinematic activation generation does not itself define a new Gameplay reference.
A generation with no writer cannot retire a still-usable Gameplay reference. Conversely, a carried activation is not recovery or writer authority.
The CinematicEnterActivation record retains one admitted writer decision as
start provenance, bound to receiver, generation, settings epochs, aspect, flags,
thread and observed component. EXIT checks association before carrying it;
successor capture checks its context and sample before using the prior corrected
output. Settings/aspect conflicts or lost association retire this evidence.
For a captured segment with an admitted Writer, a new armer may observe up to
0.6 degrees of progress toward the same corroborated target. The camera field
must confirm the new start, and neither arming nor empty generations move the
Writer anchor. The approach is directional for ascending and descending
segments. If this path is used, the first admitted Writer must confirm the
original projection and thread for the current generation. This also covers
the observed constraint bit transition from 0x5 to 0x4 during arming when the
Writer restores 0x5. The narrow endpoint and other FOV comparisons retain
their own tolerances.
Prepared endpoints remain prepared after the native blend finishes; they are
not transformed again solely because a new technical generation starts. Unknown
starts do not gain a fresh-native claim. No camera pointer cached here is
subsequently dereferenced, and no time window or scene-specific rule is used.

## Runtime ownership

The translation-unit-private `RuntimeState` owns the ASI lifetime, native
callback admission, physical engine adapters, feature availability, workers
and presentation delivery. Its `lifecycle_authority::Authority` owns committed
camera ownership, effective camera settings, Gameplay reference, Cinematic
activation and recovery, Dialogue and provider handoff. Native callbacks submit
captured facts through their ports under `CameraCorrectionGate`. Settings use
the same gate, and shutdown retires Authority after callback and worker drain.

Writer PRE prepares at most one FOV argument for its native store. Optional
post-store and post-helper callbacks submit named observations for the same
invocation. They have no correction permission. Overlay, zoom and telemetry
read committed projections. Zoom and transition observations retain their own
provenance and never establish lifecycle ownership.

An accepted native Cinematic ENTER ends previous Dialogue permissions and
resets their candidate and recovery data. An interleaved writer for a
different receiver cannot move this terminal boundary. When scope storage is
full, Authority can reclaim retired records independently of receiver order.
Active claims, the baseline, the current selection, the latest writer and
pending effects keep their scope records.

The previous adapter's writable stores and control paths compile only for test
characterization. The ordinary production build has one camera lifecycle owner.

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

Zoom observation uses paired unique signatures and decoded load/compare
operands in the executable section. Both hooks are created disabled and the
observer admission opens only after both activate. Callbacks serialize through
the existing camera correction gate and recheck admission; reset closes
admission before the existing native callback drain and resource cleanup.
Diagnostics selection and Zoom observer availability are logged independently.
Neutral Gameplay FOV presentation requires an available observer and a valid
inactive observation. Before the first valid Zoom OUT, this reference can remain
unavailable; a valid OUT starts stable qualification if no prior reference exists.
This observational reference is separate from the production GameplayBaselineStore.

Cinematic writer diagnostics distinguish source ownership from output destination.
Same-source RBX transitions accumulate in fixed-size Runtime-owned counters rather
than individual identity records. Full writer/BEFORE/AFTER samples follow semantic
decision changes (including the transaction's exact mutateFov result), source,
ownership and validity changes. Output-pointer cycling or numeric FOV drift alone
does not select a full sample. Summaries retain first/last/extrema numeric values
and counts, and publish on source/lifecycle changes, new generations, EXIT,
disable and existing controlled cleanup. The independent continuation trace stays
available. No extra process-termination hook is used; ExitProcess does not promise
a final cleanup summary, so lifecycle boundaries preserve the primary evidence.
With Diagnostic Mode enabled, the camera-writer store trace correlates up to eight
camera-writer invocations while Cinematic is active per accepted generation across
PRE, an adjacent post-store observation, and the immediately following same-thread
invocation for the same source and output. The post-store point is resolved from the validated writer instruction
sequence and installed only when its patch span cannot overlap the primary writer
hook. It confirms the output field immediately after that store; it does not prove
that the render path later consumes the field or that no same-value intervening
write occurred. If the validated site or non-overlap condition is unavailable,
only this optional trace is omitted; camera behavior is unchanged.

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
