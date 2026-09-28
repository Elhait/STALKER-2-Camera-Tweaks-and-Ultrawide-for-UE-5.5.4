# Production Safety Invariants

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
- A failed configuration persistence operation does not truncate or replace
  the last-known-good INI.
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
- The startup overlay hint is presenter-owned state, created once after
  composition readiness and runtime settings availability, before the first
  frame build. In Auto locale mode, an accepted asynchronous game-language
  synchronization completes before hint creation; if posting that request
  fails, a confirmed English fallback is used. Pending creation itself keeps
  the presenter scheduler awake until synchronization is accepted; thereafter
  the language-sync message is the wake event. Visibility, Delete and
  game-frame activity are not prerequisites. A notification's visible lifetime begins only after its draw
  is committed on the visual's attached surface (for replacement generations,
  after SetContent + composition Commit + generation publication). Pending
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
  game HWND is the input boundary and the original window procedure is
  preserved for pass-through. While the settings UI is closed, the game owns
  input. While open, mouse messages and raw relative deltas are copied into a
  bounded bridge and applied to ImGui only on the composition owner thread;
  zero-delta raw packets do not claim the relative-motion stream; a real
  relative movement does. Input ownership is released transactionally on
  close/focus loss. No
  game-thread ImGui access, per-frame OS cursor polling or ClipCursor lease is
  used. If a foreign subclass is above Camera Tweaks, its saved continuation
  is retained until that window is destroyed rather than forcibly rewriting
  the foreign chain. Terminal Overlay disable cannot reacquire input capture.
  Core Gameplay, Cinematics and Dialogue initialization and INI settings remain
  active if Overlay setup fails.
- Build success is not runtime proof. Runtime compatibility claims require
  executable identity, log evidence and the named regression scenario.
- Runtime settings API noexcept forwarding contains exceptions from its owned
  callbacks and reports failure through existing result types. Optional cleanup
  callbacks that acquire mutexes are not noexcept; their outer native-boundary
  handler contains transition failures too. Neither is AV/driver isolation.
