# Production Safety Invariants

- Project-wide semantic FOV tolerance is ±1.0 degree, defined once as
  `camera::kSemanticFovToleranceDegrees` in `src/camera/fov_semantics.hpp`.
  Finite FOV values within that inclusive difference are semantically
  equivalent unless independent lifecycle/source evidence proves an intentional
  transition. Numeric proximity alone never proves source identity, provenance,
  lifecycle, ownership, or camera intent; source ownership must use explicit
  source/lifecycle evidence. A difference greater than 1.0 degree warrants
  investigation but does not itself prove a transition. This tolerance is not
  a floating-point, projection, interpolation, recovery-endpoint,
  sampling-stability, or output assertion epsilon; those retain their
  purpose-specific thresholds and must not be replaced mechanically.

- Ambiguous, incomplete or structurally invalid signature matches fail closed.
- Resolver decode, fixed look-ahead, relative-call and byte-window validation
  are bounded by the validated executable section span.
- The gameplay and cinematic resolvers validate match cardinality and
  instruction shape before installing hooks. The executable SHA-256 is logged
  as identity/support evidence; it is not currently an allowlist gate. An
  unknown hash still cannot install a hook without a unique structurally valid
  resolver result.
- Gameplay, cinematics and dialogue are independently configurable user-facing
  interventions, but a feature may require observation capabilities installed
  by another domain. Missing required observation capability fails the
  dependent intervention closed without invalidating unrelated features.
- Runtime diagnostics are persistent infrastructure: useful bounded camera,
  lifecycle, ownership, source and compatibility instrumentation remains in
  the ordinary production ASI under Diagnostic Mode after the motivating issue
  is resolved. Diagnostic Mode controls observation/log emission only; it
  cannot change camera output, aspect, lifecycle, ownership, associations,
  correction gates or state-machine timing. Prefer event/change-based output
  and skip diagnostic-only reads, snapshots and formatting while disabled.
  Remove or replace established instrumentation only for a documented reason:
  safety/invasiveness, material measured performance cost, a stale or unsafe
  runtime-layout dependency, or complete supersession by a safer/general
  observation contract. Preserve the diagnostic capability when replacing its
  implementation and validate that it remains behaviorally observational.
- A failed configuration persistence operation does not truncate or replace
  the last-known-good INI.
- Configuration staging uses exclusive file creation and never removes a
  pre-existing staging pathname after an open failure. Native log writers
  validate final-component reparse/link state before truncation, and INI reads
  and runtime/Overlay log writes have explicit size budgets.
- Newly generated native stubs/trampolines are cache-synchronized before
  publication. Patch/restoration synchronization failures are reported and
  uncertain executable ownership is retained. A failed VEH registration cannot
  proceed to the patch's execute-protection transition.
- Cinematic aspect-store failure skips the custom/native store path and
  resumes after the replaced instruction; this is intentional fail-closed
  behavior.
- Cinematic hooks are created disabled and are enabled only after the required
  component installation commit; observation-only lifecycle installation does
  not enable presentation intervention.
- Staged configuration is flushed and closed successfully before atomic
  replacement of the managed INI.
- Auto viewport aspect uses only usable client geometry; transient degenerate
  dimensions fall back to the existing display/native path.
- Auto overlay language is read on explicit selection and at the closed-to-open transition. Explicit selection requests synchronization through the game window procedure; the UI/render callback does not call the native reader directly. The
  native reader validates the expected game image layout and both code
  prologues before calling either address; returned FString data is bounded
  and copied before releasing its game-allocated buffer. Unknown or unavailable
  identifiers select English without changing persisted Auto mode. No polling,
  OS locale, storefront state, or game-setting writes are used.
- Overlay position migration is one-shot and non-destructive: legacy coordinates
  are copied into `[Overlay]` in the main INI, and the old file is left untouched
  but no longer read after the migration marker is committed.
- The startup overlay hint is presenter-owned, one-shot state. Eligibility
  requires completed camera-core/runtime initialization, a ready Overlay
  presenter with its GameHwnd input binding, and either a successful read of
  the game's validated Interface Language value or an explicit bounded
  fallback. The one-shot read runs through the existing GameHwnd callback and
  returns to the Overlay owner; it does not add a graphics hook. Auto mode
  applies the detected locale, while an explicit user locale remains
  authoritative. A failed/post-rejected probe falls back immediately; an
  accepted probe that receives no result resolves after one 3000 ms owner-wait
  deadline, not a polling loop. Auto mode uses confirmed English fallback in
  those failure cases. This gate does not claim that a game scene or frame is
  ready. Manual menu/Delete handling is independent of startup-hint readiness.
  A notification's visible lifetime begins only after its draw is committed on
  the visual's attached surface (for replacement generations, after
  SetContent + composition Commit + generation publication). Pending
  notifications cannot expire before that point; fade, expiry and the final
  clear commit remain presenter-scheduled.
- Controlled shutdown signals and joins workers before resetting hooks or
  restoring patched state. Loader-lock detach does not perform that teardown.
- Runtime callbacks that mutate ordinary telemetry fields are expected to run
  on the game/runtime owner thread. Atomic fields are used for cross-thread
  observations; `WorkerLifecycle` is an externally serialized lifecycle
  primitive, not a general concurrent thread-management API. Lifecycle calls
  must not overlap initialization or one another; DLL detach only signals the
  stop event.
- The player's selected FOV is preserved; aspect correction does not use a
  hard-coded FOV compensation multiplier.
- In Gameplay `RuntimeCombine`, the correction target is a validated current
  viewport aspect. Synchronize the Gameplay writer aspect only on a real
  mismatch, then confirm aspect and flags by read-back before applying HorPlus
  in that domain. Invalid viewport input or unconfirmed synchronization must
  fail closed without applying HorPlus in the unconfirmed domain. This mode
  does not synchronize native flags; existing `HorPlus` and
  `AspectRecalculation` semantics remain independent.
- For an owner- and writer-validated RuntimeCombine `CinematicExiting`
  recovery, preserve the confirmed transformed cinematic output only across
  the validated native interpolation segment, and resume normal RuntimeCombine
  after the native endpoint is confirmed. Do not infer this behavior for
  unrelated modes, sources or unvalidated samples.
- In Gameplay HorPlus mode, cinematic EXIT keeps the camera coordinator in
  `CinematicExiting` until a readable gameplay writer sample with valid aspect
  and gameplay flags matches the saved native EXIT target for a validated source.
  Retained native `GameplayBaseline` ownership permits recovery on the first
  post-EXIT sample without a prior departure; a replaced source requires the
  existing source-bound recovery evidence. Dialogue's depart-then-return
  exclusion semantics remain independent and unchanged. For the same validated
  gameplay source, readable native interpolation samples bounded by the cached
  transformed cinematic endpoint and saved native EXIT target are mapped
  continuously between those endpoints in HorPlus output space. This recovery
  interpolation cannot update `GameplayBaseline`. Ambiguous, invalid,
  out-of-range, or insufficiently source-validated samples pass through
  unchanged. HorPlus resumes normal gameplay transformation when native
  recovery is validated; later native gameplay changes remain eligible.
- Overlay is an optional independent DirectComposition consumer. It never
  patches DXGI exports or game swapchain methods and never owns game
  backbuffers, devices or queues. Its private D3D11 work targets only its own
  composition surface; camera core and INI configuration do not depend on
  window discovery, composition initialization, rendering, or input readiness.
- Presenter pacing is demand-driven: idle/static UI owns no repeating frame
  source; notification animation and coalesced dirty UI frames use the system
  DirectComposition clock when available. A pacing-only worker may signal the
  single ImGui/D3D11 owner thread but never accesses its UI/GPU state. The
  compatibility fallback is bounded to notification animation; input redraw
  remains event-driven. No busy-loop or game Present synchronization is used.
- A settled hidden Overlay with no notifications or pending startup hint has an
  empty composition target root. Its owner retains graphics resources and the
  Delete input path. Root transitions publish attachment only after SetRoot and
  Commit succeed; failure disables Overlay instead of reusing uncertain pending
  commands. Reattachment follows accepted surface preparation/publication and
  precedes notification lifetime publication. No compositor completion wait is
  introduced on either thread.
- The composition owner thread creates and updates surfaces. A generation is
  published only after a complete draw and successful composition commit; a
  failed replacement leaves the previous generation authoritative. Minimize
  suspends drawing and restore resumes it without losing generation ownership.
  Resource retirement occurs after the visual has accepted and committed its
  replacement, never in the game's resize/presentation callback.
- Device loss permits one explicit Overlay-owned recreation attempt. Failure
  disables the optional Overlay and releases its own resources; it does not
  alter or wait on the game's native presentation path. No per-frame GPU fence
  waits or game Present synchronization are used.
- Window/input subclassing is independent of renderer-generation state. The
  game HWND is the input boundary; a separate unowned Overlay HWND owns the
  DComp target and is created/destroyed on the Overlay owner thread. The
  original game window procedure is preserved for pass-through. The WndProc
  uses terminal admission, bounded
  command enqueue and try-only input-bridge operations; it does not acquire the
  Renderer mutex, apply or persist settings, publish notifications, snapshot
  runtime settings, or perform file logging. Accepted keyboard/control
  commands are processed in order on the composition owner thread. Queue
  saturation rejects the current command, passes that event through, and
  requests terminal fail-open shutdown without overwriting previously
  accepted commands. While the settings UI is closed, the game owns input.
  While open, mouse messages and raw relative deltas are copied into a bounded
  bridge and applied to ImGui only on the composition owner thread;
  zero-delta raw packets do not claim the relative-motion stream; a real
  relative movement does. Input ownership is released transactionally on
  close/focus loss. Input admission requires an accepted visible composition
  frame and fresh owner progress; focus loss, minimize, host geometry changes,
  rebind and terminal state revoke it before owner-side reconciliation. A
  stale owner heartbeat terminally fails open rather than allowing the game to
  remain without input. No
  game-thread ImGui access, per-frame OS cursor polling or ClipCursor lease is
  used. If a foreign subclass is above Camera Tweaks, its saved continuation
  is retained until that window is destroyed rather than forcibly rewriting
  the foreign chain. Terminal Overlay disable cannot reacquire input capture.
  Core Gameplay, Cinematics and Dialogue initialization and INI settings remain
  active if Overlay setup fails. Renderer disable/failure requests terminal
  owner cleanup; it never performs synchronous graphics teardown on the
  failing caller. The owner drains admitted callbacks and runs a one-shot
  guarded finalizer. A failed DComp detach/commit retains the composition
  references and display HWND. The log distinguishes `COMPLETE`, known
  `RETAINED` resources and `UNVERIFIED` outcomes, with `PENDING` emitted before
  cleanup; Renderer-owned reference emptiness alone is not proof of DWM/driver
  teardown.
- Build success is not runtime proof. Runtime compatibility claims require
  executable identity, log evidence and the named regression scenario.
- Runtime settings API noexcept forwarding contains exceptions from its owned
  callbacks and reports failure through existing result types. Optional cleanup
  callbacks that acquire mutexes are not noexcept; their outer native-boundary
  handler contains transition failures too. Neither is AV/driver isolation.
