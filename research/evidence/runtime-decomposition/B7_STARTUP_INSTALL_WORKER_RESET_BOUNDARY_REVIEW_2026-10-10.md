# B7 Startup, Installers, Workers & Reset — Boundary Review

Date: 2026-10-10  
Scope: B7 boundary review only; no implementation authorized  
Production/vendor changes, tests, builds, game launch, and ASI installation: **not performed**  
Decision status: **Closed — Complete with documented `Remain` decisions; approved 2026-10-10**

## Executive result

The B7 inventory contains two independently implemented boundaries already
outside `runtime.cpp`: `DllMain` in `src/plugin/dll_entry.cpp` and the
externally serialized `WorkerLifecycle` primitive in
`src/plugin/worker_lifecycle.*`. Resolver/transaction policy is also already
separated in domain helpers. The remaining B7 responsibilities compose these
parts around the sole resident `RuntimeState`, its `HookSet`, readiness and
correction gates, feature capability flags, and cross-batch callback
lifecycles.

The review found no additional B7 extraction slice with a demonstrated net
reduction in coupling or failure risk. Moving installer/reset/startup glue
would require a broader mutable owner API or split one initialization and
rollback transaction across translation units. Moving worker loops or the
hotkey loop would similarly export feature state and lock ownership while
leaving thread ownership in the runtime. Recommendation: close B7 with
documented `Remain` decisions, retain the already-separated components, and
end the decomposition campaign at B1–B7 if the user approves. This does not
close N03–N06 or exact-artifact runtime validation.

## B7 inventory and dependency/ownership map

| Responsibility / current symbols | Current owner and dependencies | Ordering / thread contract | Classification and rationale |
|---|---|---|---|
| DLL entry and process attach/detach: `DllMain`, `SetModuleHandle`, `NotifyProcessDetach` | `DllMain` is already in `src/plugin/dll_entry.cpp`; module handle, stopping flag, gates, and worker stop event belong to the process-resident runtime owner | Attach disables thread callbacks and starts `InitializeThread` asynchronously, closes its thread handle, and returns. Process-termination detach does nothing complex; normal detach only closes readiness/correction gates and signals workers. It does not join or reset hooks under loader lock. | `Already extracted` — DLL entry is already a separate thin boundary. `SetModuleHandle` and runtime facade operations `Remain`; splitting them would not improve ownership. |
| Initialization entry: `InitializeThread` | Runtime owner; logger, config, readiness flags, Overlay startup-success state, gates, worker lifecycle and `Initialize` call | Runs on the initialization thread created by DLL attach. N03 wraps this actual entry with an outer exception boundary; failure closes readiness, signals workers, and attempts guarded cleanup once. Overlay core-ready notification follows the initialization result path. | `Remain` — it coordinates runtime and Overlay readiness and owns N03 failure semantics; moving it requires exporting runtime state and callback ordering. |
| Main startup orchestration: `Initialize` | Sole `RuntimeState`, `HookSet`, config/settings, executable image, logger, module handle, callbacks, worker facade, correction/readiness/capability flags | Ordered sequence: create logger and worker stop event; load configuration/settings; optionally start Overlay discovery; publish settings API; install observations and transactional Cinematic components; install/enable Gameplay writer; establish readiness and start required/optional workers. Recoverable setup exceptions route through one guarded reset attempt. | `Remain` — this is the composition root and startup transaction. Its ordering spans B1–B6 and Overlay; splitting it would create orchestration across TUs without reducing the number of owners. |
| Exception readiness and failure cleanup: `CloseInitializationReadiness`, `TryInitializationFailureCleanup` | Runtime flags, `WorkerLifecycle`, `ResetAllRuntimeResources`, one-shot initialization-thread marker | Readiness/gates close before cleanup; only one cleanup attempt is allowed. Cleanup exceptions are contained. A nonzero worker count means cleanup is not reported as complete and readiness stays closed. | `Remain` — N03 boundary is already implemented here and must stay adjacent to its actual entry/reset path unless a separately approved design establishes a smaller owner boundary. N03 is only partially mitigated. |
| Global reset: `ResetAllRuntimeResources` | All RuntimeState hook handles, gates, worker lifecycle, recovery/exclusion state, diagnostic counters, patched cinematic aspect state and capability flags | Set stopping/readiness/capture/gates closed; signal and join workers; if join fails, return while retaining hook resources; otherwise clear lifecycle state, emit bounded summary, reset diagnostic and production hooks in their current order, restore patched aspect, then clear capability flags. | `Remain` — it is the sole cross-component teardown orchestrator. Any move must preserve exact order, failure return, and retained-resource semantics across B3–B6 plus vendor hooks. |
| Public shutdown and normal detach: `Shutdown`, `NotifyProcessDetach` | Runtime owner, logger, flags, `WorkerLifecycle` | Controlled `Shutdown` calls reset outside loader lock and then flushes logging. `NotifyProcessDetach` is signal-only for normal detach; process termination skips complex cleanup. Dynamic unload is not claimed supported. | `Remain` — thin facade around the owner’s lifetime contract; no independent lifecycle owner exists. |
| Worker creation/join primitive: `WorkerLifecycle` | Already implemented in `src/plugin/worker_lifecycle.hpp/.cpp`, stored as one member of `RuntimeState` | Explicitly externally serialized lifecycle operations; worker entry thunk contains exceptions; stop event is signaled, threads joined, handles closed only after successful joins. Self-join/failure retains ownership rather than claiming teardown. | `Already extracted` — this is the coherent B7 worker-mechanism slice already separated. Do not create another worker manager or state copy. |
| Runtime worker facade: `CreateWorkerStopEvent`, `StartWorker`, `WaitForWorkerStop`, `StopWorkers` | Thin wrappers over the owner’s single `WorkerLifecycle`; logger is used for start failure/success | Start/stop/reset calls must not overlap initialization or one another. `SignalStop` may be invoked by detach; joins occur on controlled paths. | `Remain` — wrappers bind owner lifetime and user-visible logging; no current benefit from another facade layer. |
| Worker entry loops: `HotkeyLoop`, `ResolutionMonitorLoop`, `DialogueDiscoveryLoop`, `OneShotTriggerLoop`, and enabled diagnostic loops | Runtime feature settings, owner atomics, hotkey/config state, diagnostic snapshots, Dialogue mutex, and stop event; Overlay discovery itself belongs to the Overlay subsystem | Loops use stop-event waits and may observe state created by initialization. Optional diagnostic workers must fail locally. Runtime worker set has fixed capacity and lifecycle is externally serialized. | `Remain` — loop bodies are subsystem behavior coupled to owner state, not the worker mechanism. Extracting a subset would create state APIs and could change polling/stop timing. Diagnostic loops remain profile-gated. |
| Native hook resolution and instruction/site validation: `VerifyExecutableAndInstruction`, `ValidateEnterBoundary`, `ValidateExitBoundary`, `ValidateIndexedExitBoundary`, `ResolveCinematicAspectStore`, `ResolveCinematicFovCallsites` | Runtime executable image and owner HookSet; calls pure Gameplay/Cinematic resolver helpers | Must validate executable span, signature cardinality and instruction shape before hook construction; callback targets are production functions and native context assumptions belong to their owners. | `Already extracted` (pure resolver/policy helpers); `Remain` (runtime image binding and hook-site adapters). Existing Gameplay/Cinematic helper extraction is not proof of production install orchestration. |
| Hook construction, component commit and rollback: `InstallCinematicAspect`, `RestoreCinematicAspect`, `InstallCinematicAspectComponent`, `InstallCinematicFovComponent`, `CommitCinematicComponents`, `RollbackCinematicAspectComponent`, `RollbackCinematicFovComponent`, `InstallDialogueBoundary`, `InstallZoomTransitionObservation`, Gameplay writer installation, and profile-gated diagnostic installers | Same owner HookSet, SafetyHook objects, component availability and correction gates; Cinematic/Dialogue/Gameplay state from B4–B6 | Mid-hooks are created in required disabled/active forms; interdependent Cinematic hooks enable as a transaction and publish the gate only after success. Failure resets only the affected component when possible; global failure routes to startup cleanup. Rollback/reset order must match current code. | `Remain` — installers bind callbacks and HookSet fields in the same transaction. `cinematics::InitializeTransactional`, Gameplay/Cinematic resolvers and other leaf policies are `Already extracted`; moving production install closures adds no independent ownership boundary. |
| `SafeMidHookEntry` native exception wrapper | Private runtime helper, used by callback registration | Catches exceptions at foreign native callback entries and preserves trampoline pass-through context; N02 uses it for both Zoom callbacks. | `Remain` — wrapper stays local while callback registration is local; moving only the template creates a header abstraction without a second consumer boundary. N02 remains `Fixed` per its report. |
| Hook executable storage / SafetyHook patch state | Owned by individual SafetyHook hook objects in the vendored SafetyHook implementation; referenced by runtime HookSet until safe lifetime ends | N01 distinguishes unpatched/patched/uncertain states and retains executable storage when target bytes may reference it. Retention until process termination prevents reuse but is not successful teardown. | `Already extracted` as the existing vendor-library ownership boundary; N01 is `Fixed` within its fault-injection/offline evidence. This review does not change vendor code or claim live game teardown proof. |
| Hotkey dispatch/action and Overlay notification facade: `HotkeyLoop`, `PublishOverlayNotification`, `DrainOverlayNotifications`, `SetHotkeyRebindCaptureActive`, `MarkHotkeyRebindKeyConsumed` | `HotkeyLoop` reads runtime settings, owner policy/epochs and Dialogue state; notification queue/capture state are RuntimeState-owned and shared with Overlay; uses the published settings API and persistence helpers | Input is sampled by its worker loop; setting mutations cross the existing correction gate and persistence path. Rebind capture/consumed-key flags are shared with Overlay. | `Remain` for loop, queue and state-bound action orchestration; `Already extracted` for pure dispatch predicate, hotkey mapping and persistence. A new hotkey module would still depend on the same state/locks. |
| Overlay startup and camera-core-ready coordination | Plugin owns configured-start result and call ordering; Overlay owns discovery/presenter thread and readiness state. `TryStartConfiguredOverlay` is already a plugin helper; `StartOverlayDiscovery` and `NotifyOverlayCameraCoreReady` are Overlay subsystem calls. | Optional Overlay starts from configuration before camera-core readiness. After initialization, the plugin sends readiness notification only through the existing success/result checks. Overlay failure must not disable camera corrections; Overlay resources stay with Overlay owner. | `Already extracted` (optional-start decision helper and Overlay subsystem); `Remain` (plugin-to-Overlay handshake/readiness fields). Do not move graphics/presenter lifecycle into `runtime.cpp` or make camera readiness depend on Overlay success. |

The move ledger contains finer-grained B7 candidate rows. This grouped map is
the final responsibility-level classification; it supersedes proposed
destinations there, not the approved B1–B6 decisions or their historical line
inventories.

## Failure paths and safety evidence

- **N01 — Fixed:** the remediation report records explicit patch states and
  executable lifetime retention when target patch state may be uncertain.
  The retention policy is deliberately process-resident and does not claim a
  successful teardown. Local fault-injection/native fixtures passed; game
  callback concurrency and final-artifact runtime behavior remain separate.
- **N02 — Fixed:** both Zoom callbacks use `SafeMidHookEntry`; production
  adapter and local executable-hook fixtures cover normal/exception cases.
  This does not expand into a claim about every hook's live instruction site.
- **N03 — Partially mitigated:** actual `InitializeThread` contains
  exceptions, closes readiness/gates and attempts guarded cleanup at most
  once. Offline tests do not execute the complete game-specific signature
  scan, partial installation, or real Initialize/reset path.
- Static path note for N03 validation: `Initialize` has recoverable exception
  handlers that call guarded cleanup and then follow its existing completion
  return path; `InitializeThread` separately gates Overlay-ready notification
  on `overlayStartupSucceeded`. The exact combined readiness meaning after a
  partially completed/cleaned startup is not established by this review and
  must not be labelled a defect or a PASS without matching runtime evidence.
- **N04–N06 remain open:** N04 production startup/install/partial rollback
  orchestration; N05 callback quiescence and patch-removal races; N06 current
  native replay provenance. B7 classification closes none of these gates.
- **Exact artifact live validation remains pending:** prior live smoke loaded
  ASI SHA-256 `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771`,
  not final remediation artifact
  `E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81`.
  The earlier logs cannot be transferred to that binary.

These findings distinguish source ownership and known helper evidence from
unexercised game-specific failure paths. No N01–N03 status or historical test
result was changed or reinterpreted.

## Architecture conclusion and recommendation

No new B7 extraction is recommended. `WorkerLifecycle` and DLL entry are
already separate; pure resolver, transaction, and Overlay-start policy are
already helpers; the remaining work is runtime orchestration over one state
owner and one hook set. A new `runtime_hook_installation.cpp` or
`runtime_startup.cpp` would need to import owner state, callback addresses,
feature/capability flags, and commit/rollback operations. Without a narrower
opaque transaction boundary, that move would relocate ordering rather than
reduce it and increase cross-TU coupling.

Recommendation for user approval: close B7 as **Complete — with documented
`Remain` decisions**, and end the decomposition campaign at B1–B7. This is an
architecture closure recommendation only. Do not implement any extraction
from this report. Keep N03 `Partially mitigated`, N04–N06 open, and final
remediation ASI live validation pending. If an actual maintenance need later
identifies a cohesive installer transaction API, review that narrow design
separately rather than reopening the entire B7 set.

No code, vendor files, tests, builds, release artifacts, or configuration
were changed. No tests, builds, or game were run.
