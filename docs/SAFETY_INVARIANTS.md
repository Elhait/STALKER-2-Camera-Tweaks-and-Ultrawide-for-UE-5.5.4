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
- The startup overlay hint is queued once on the first valid Present after
  renderer/resource readiness and runtime settings availability; it does not
  depend on settings-panel visibility. Its displayed key comes from the
  effective hotkey snapshot. Its ten-second visible lifetime starts when the
  renderer drains it for display, not while it waits in the notification queue.
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
- Build success is not runtime proof. Runtime compatibility claims require
  executable identity, log evidence and the named regression scenario.
