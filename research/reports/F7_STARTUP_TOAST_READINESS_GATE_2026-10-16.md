# F7 — Evidence-Based Startup Toast Readiness Gate

**Date:** 2026-10-16  
**Scope:** Startup notification readiness only. No game, graphics fixture, ETW,
PresentMon, ASI installation, or user configuration changes.

## Outcome

The startup toast now has an explicit gate separating runtime initialization,
Overlay readiness, observed game data, and eligibility. It is created only after
runtime and Overlay readiness plus either a successful validated game-data read
or the documented bounded fallback. The 10-second visible TTL and its
draw/attachment/commit start condition are unchanged.

The available live logs are historical evidence for the preceding artifact,
not validation of this F7 implementation. They show that the earlier toast's
TTL was 10 seconds, so F7 does not change or attempt to extend that lifetime.

## Available signals

| Signal | Availability and provenance | Limitations | F7 disposition |
| --- | --- | --- | --- |
| Runtime core initialization | `NotifyOverlayCameraCoreReady` publishes the runtime-ready state; `RefreshPresenter` refuses initialization until this state is set. | Mod readiness only; contains no game-owned data. | Required prerequisite, not game-data evidence. |
| Game window discovery / foreground / geometry | A process top-level HWND and its client geometry are discovered before presenter initialization. | Describes window state; does not establish that game data was read or a frame/scene is active. | Not accepted as game-data evidence. |
| Game WndProc subclass installation | Installation is recorded after a valid game HWND is selected. | Hook installation proves only that the callback was installed; it does not prove it executed or that a game read succeeded. | Required to deliver the probe, not accepted by itself. |
| Camera observers / camera snapshots | Can publish observed camera state when their relevant feature and game path are active. | Optional feature/path dependent; may never fire before a menu or gameplay scene and is unrelated to toast readiness. | Not used. |
| Auto locale synchronization | Existing reader invokes a validated native game interface-language reader. A successful returned locale is game-owned data; read failure/fallback is distinguishable through `readSucceeded`. | Delivery waits for the game WndProc to process a posted message. `overlayLocaleAutoSynchronized` alone is not proof because fallback can also mark locale synchronized. | F7 sends a one-shot startup probe using this same reader; only `readSucceeded=true` counts as observed game data. |
| Configured game/mod settings | Runtime settings snapshot is available to the Overlay. | Mod configuration, not an observation of the game's runtime. | Not accepted as game-data evidence. |

### Native reader safety and meaning

`ReadGameLanguageLocale` validates the expected game image and AMD64 PE layout,
the `.text` section shape, and the reader/free-function prologues before calling
the native reader. It bounds and copies the returned FString and frees the game
buffer under SEH. The probe runs through the existing game-window callback;
only a fixed-size result and explicit success bit cross to the Overlay owner
command queue. The owner consumes the result under its existing serialized
Renderer path.

This is a genuine successful read of a game-owned interface-language value.
It is **not** evidence that the main menu, a save, gameplay, or a stable
presented frame is ready. F7 intentionally does not require those states.

## Readiness contract

The owner-thread `StartupToastReadinessGate` keeps these facts separate:

1. `RuntimeInitialized`: the runtime-core readiness notification has enabled
   presenter discovery; the Renderer initialization path is reached only after
   this notification.
2. `OverlayReady`: the presenter is ready and the GameHwnd input binding has
   been installed. This represents Overlay availability, not game-data
   observation.
3. `GameDataObserved`: the dedicated native interface-language read returned
   successfully. Fallback values never set this fact.
4. `StartupToastEligible`: runtime and Overlay are ready, and either game data
   was observed or the explicit fallback was resolved.

The gate is mutated on the Overlay owner thread while the existing renderer
serialization is held. Game WndProc only performs the already-established
bounded read and submits a generation-tagged owner command; it does not create
the notification or manipulate Renderer state. Duplicate readiness and data
signals are idempotent. The notification remains one-shot (ID 0).

No gameplay camera state, optional camera module, save-load completion, main
menu, graphics hook, swapchain hook, Present hook, or new polling worker is a
prerequisite. Manual Delete/menu input remains independent of startup-toast
eligibility.

## Missing-signal policy

- If posting the probe fails, the gate resolves immediately through explicit
  fallback.
- If the game callback runs but the validated native read fails, the result is
  explicitly fallback, not `GameDataObserved`.
- If no response arrives, a one-shot **3,000 ms response deadline** is included
  in the existing owner wait. It does not add a worker or periodic polling.
  The value is a bounded response policy; the available preceding run showed
  about 1.156 seconds between Auto-locale request acceptance and resolution,
  but that is one observation, not a latency distribution or guarantee.
- In Auto locale mode, fallback selects English. An explicit user-selected
  locale is not overwritten. A valid late result may still update Auto locale
  and is logged as late; it does not create another startup toast.

The deadline is not a fixed startup sleep: a successful read makes the toast
eligible immediately, while a failed/missing response takes the explicit
fallback path. The toast still begins its 10-second TTL only after its own draw
was submitted and the attached composition commit succeeded.

## Diagnostics

With `[Diagnostics] Enabled=true`, the gate emits transition markers
`STARTUP_GATE_WAITING`, `STARTUP_GATE_RUNTIME_READY`,
`STARTUP_GATE_GAME_DATA_OBSERVED`, `STARTUP_GATE_ELIGIBLE`, and
`STARTUP_GATE_FALLBACK`. Each includes `GetTickCount64()` milliseconds and a
reason. Diagnostics disabled means these formatted messages are not produced.
The markers report the gate's facts; they do not claim a stable scene or frame.

## Implementation files

- `src/overlay/presenter/renderer_state.hpp` — explicit gate state, deadline,
  and eligibility contract.
- `src/overlay/presenter/renderer_runtime.cpp/.hpp` — gate lifecycle,
  notification creation, Auto-locale fallback and diagnostic transitions.
- `src/overlay/localization/game_language_reader.cpp/.hpp` — dedicated startup
  probe message using the validated existing game-language reader.
- `src/overlay/input/input_state.hpp` — startup-probe identity carried in the
  owner command result.
- `src/overlay/composition/composition_runtime.cpp` — game callback read,
  owner-command completion, and deadline integrated into the existing wait.
- `tests/overlay/composition_presenter_state_harness.cpp` — deterministic
  event ordering, fallback, late data, idempotence and manual-menu independence.
- `tests/runner/localization_catalog_audit.ps1` — source-contract checks.
- `docs/ARCHITECTURE.md` and `docs/SAFETY_INVARIANTS.md` — readiness and
  evidence boundaries.

No INI, installed ASI, release artifact, hook behavior, camera algorithm,
configuration default, or F6 diagnostic behavior was intentionally changed.

## Validation

| Check | Result | Evidence boundary |
| --- | --- | --- |
| Focused `composition_presenter_state_harness` build | PASS, exit 0; rebuilt the changed harness object and relinked the executable. | The external process supervisor verified build-tree cleanup; transient `vctip.exe` was gone on the follow-up check. |
| Focused `composition_presenter_state_harness.exe` | PASS, exit 0: `composition presenter lifecycle: PASS`; supervisor verified child cleanup. | Deterministic offline state/lifecycle contracts only. |
| `localization_catalog_audit.ps1` | PASS, exit 0; 18 locales and all listed catalog/resource checks passed; supervised child cleanup verified. | Static/source and catalog checks, not runtime display. The wrapper's extra residual-process scan initially matched its own PowerShell host; a follow-up scan excluding the current host found no other relevant build/game/capture process. |
| Canonical `test.cmd` | PASS, exit 0; registry 59/59, 62/62 runner invocations. | 105 native-adapter cases each in Diagnostics OFF/ON (210 fresh child processes); 4/4 N06-M01 Zoom scenarios. Process-tree cleanup verified. |
| Production Release build | Compile/link completed (65/65 Ninja steps) and published the F7 candidate. The first wrapper invocation reported timeout after publication because a `vctip.exe` descendant outlived the build command; cleanup was verified and a later canonical incremental `build.cmd` returned exit 0 with `ASI is already current`. | A configure warning noted a long object directory for a test target; the production target compiled/linked, and the canonical test suite separately built and passed all harness targets. No process remained after verification. |
| F7 candidate artifact | SHA-256 `72F373AB015D93D1662E2D62DCAB67B4FF681C3202D7C08F343748790BFD02E5`, 3,599,872 bytes. | `build-artifacts/f7-candidate-20261016/STALKER2CameraTweaks.asi`; not installed. Candidate hash remained stable on the successful incremental verification. |
| Repository-root ASI | Unchanged: SHA-256 `B265DF72FF94A9BC05B8B9E57DA1C6B9F328530B19678D86098A4C6DDC64815F`. | Build output was directed to a separate ignored candidate directory; no install or rollback artifact was modified. |
| Live startup-toast readiness path | PASS for the F7 gate sequence; user-visible result **FAIL (not seen)**. | Fresh logs identify the exact F7 candidate SHA. Runtime readiness, successful game-data read, eligibility, toast draw submission, visible HWND state and 10-second TTL are recorded. The user reports that no toast was actually seen; these state/log markers do not prove human-visible pixels. |

No prior PASS is transferred to the F7 candidate. The currently accessible
`STALKER2CameraTweaksOverlay.log` is dated 2026-10-15 and identifies
`modSha256=91BC52C8754F43D43A2ED4FEFAF7767C2C454BF6215CE81F2191260934A31253`.
That earlier run records `ttl_start` at monotonic `1173421` and
`queue_expire` at `1183421`, exactly 10,000 ms later, then hides/detaches the
Overlay. This validates the old run's TTL timing only; it neither validates
F7 nor proves how long a person perceived the toast on the newer run.

## Live run result — F7 candidate, 2026-10-16

The subsequently supplied live logs identify
`modSha256=72F373AB015D93D1662E2D62DCAB67B4FF681C3202D7C08F343748790BFD02E5`,
matching the F7 candidate. The recorded sequence is:

| Event | Monotonic ms | Evidence |
| --- | ---: | --- |
| Runtime core ready | 11,592,203 | `STARTUP_GATE_RUNTIME_READY` |
| Overlay and input binding ready | 11,598,015 | `STARTUP_GATE_OVERLAY_READY` |
| Validated game Interface Language read | 11,599,734 | `STARTUP_GATE_GAME_DATA_OBSERVED` |
| Gate eligible and toast created | 11,599,734 | `STARTUP_GATE_ELIGIBLE`, `OVERLAY_STARTUP_HINT_CREATED` |
| Toast draw submitted and TTL started | 11,599,750 | Draw-list vertices recorded; HWND/display visible and z-order above host recorded |
| TTL expiry and display hide/root detach | 11,609,750 | `queue_expire`, `display_hide`, `root_detach` |

The TTL interval is exactly 10,000 ms. The main log records the successful
Auto-locale read at 19:03:03.093, placing toast creation at roughly 19:03:03
and expiry roughly 10 seconds later. Manual menu opens appear only after that
expiry. There is no startup-probe fallback in this run; actual game data was
observed.

The user reports that the toast was not seen at all. Therefore the gate and
notification lifecycle executed as designed, but the user-visible acceptance
criterion failed. `draw_submitted`, successful commit, `hwnd_visible=1`, and
`z_above_host=1` are internal state observations, not proof that displayed
pixels were perceptible. The evidence does not identify whether the remaining
gap is composition/pixel visibility or that the toast was not noticed during
the loading interval. No new graphics experiment or code change is justified
by these logs alone.

## Remaining limitations

- Actual on-screen visibility remains unverified and conflicts with the user's
  observation; this is distinct from the confirmed F7 readiness/TTL sequence.
- If the game WndProc does not process the probe within 3 seconds, fallback
  makes the toast eligible; a later valid result can correct Auto locale.
- `GameDataObserved` proves a validated interface-language read only, not
  gameplay/scene readiness.
- The canonical regression and production artifact identity are verified;
  visual presentation remains unverified despite the user's report that it
  was not seen.

## Live-test procedure and result

1. Preserve the currently installed ASI as rollback. Install only the newly
   built F7 candidate after its SHA is reported; do not replace or edit the
   user's INI except to set `[Diagnostics] Enabled=true` if it is not already
   enabled. Restore the user's prior Diagnostics value after the test.
2. Launch the game once and do not open the module menu before observing the
   startup toast. Let the game remain in its normal loading flow; do not wait
   for the main menu or a save as a prerequisite.
3. Record whether the toast appears during loading and its approximate visible
   time. Then use Delete once to confirm the regular menu still opens; this is
   a separate check and must not be mistaken for automatic toast display.
4. Preserve the fresh `STALKER2CameraTweaks.log`,
   `STALKER2CameraTweaksOverlay.log`, and `STALKER2CameraTweaksStartup.log`.
   Correlate `STARTUP_GATE_*`, `OVERLAY_STARTUP_HINT_CREATED`,
   `OVERLAY_STARTUP_HINT_DRAWN_AND_COMMITTED`, and the toast TTL markers with
   the exact candidate SHA. Do not infer perceived visibility from `Commit`
   alone.

The above one-run procedure was completed for the matching candidate SHA. No
automatic installation was performed. The user-visible result was negative;
no further game run is requested by this report.
