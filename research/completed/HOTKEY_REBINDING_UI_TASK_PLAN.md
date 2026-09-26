# Hotkey Rebinding UI Task Plan

## Objective

Add a Hotkeys section after Dialogue in the overlay's left settings column. Let users enable/disable hotkey actions and rebind the four existing actions at runtime, with immediate persistence and deterministic, unambiguous input handling.

## Established evidence and current state

- The overlay now separates configuration on the left from Runtime, Camera Integration, and Camera State on the right.
- `config::FeatureConfig` already stores `hotkeysEnabled` and the four key codes; defaults are F9–F12. The INI parser and template already read/write these settings, and `ParseHotkey` supports F1–F12, 0–9, and A–Z.
- `HotkeyLoop` currently polls those four configured codes and calls the existing runtime setting mutation paths. Hotkey action toasts already come from that existing path.
- `WorkerLifecycle` starts workers once and joins them during plugin reset/shutdown. `HotkeyLoop` already checks `hotkeysEnabled` as an action gate on each iteration; its loop does not need to stop/restart for checkbox changes.
- Startup also uses `hotkeysEnabled` to request cinematic and dialogue hook installation. Those gates must be decoupled from the checkbox so actions enabled later have their required capabilities. Initialize the existing components once and preserve selected runtime policies; do not dynamically install hooks on checkbox changes.
- `WorkerLifecycle` has a fixed capacity of three workers. Optional diagnostic builds can start the hotkey worker plus up to three diagnostic workers, so verify build-profile combinations and raise capacity only as needed for the always-present hotkey worker.
- `overlay::InputState` currently handles Insert and mouse capture; ordinary keyboard input passes through. A UI capture state must consume only the key used for binding and preserve ordinary passthrough for other keys.
- `RuntimeSettingsApi` currently covers gameplay/cinematic/dialogue settings, not hotkey configuration. Avoid an overlay-only settings store; extend the authoritative configuration/persistence path.

## Approved scope and UX contract

### Placement and display

- Place `Hotkeys` after `Dialogue` in the left configuration column.
- Initial display:

  ```text
  Hotkeys
  [✓] Enabled
  Gameplay Mode       [ F9  ]
  Cinematic Aspect    [ F10 ]
  Cinematic FOV       [ F11 ]
  Dialogue Zoom       [ F12 ]
  ```

- Each binding field is interactive. Clicking it starts capture for that action only; show `Press key` while capture is active.
- Use the existing standardized tooltip renderer for Enabled and binding help.

### Capture and input ownership

- At most one `RebindCapture { active, action, previousKey }` exists. This is overlay interaction state and does not belong in `CameraStateSnapshot`.
- Capture has priority over normal mod hotkeys. A supported captured key is consumed by the overlay, updates that binding, and never invokes the action on that same press.
- The following press of the new key invokes the existing action path.
- Esc cancels and is never bindable. Insert remains reserved for overlay toggle and is never bindable.
- Clicking a different binding while capturing cancels the first capture and starts the new one.
- Closing the overlay with Insert, losing focus, or tearing down the overlay cancels capture and preserves the previous binding.
- Unsupported keys are ignored while capture remains active. Supported keys are F1–F12, 0–9, and A–Z. Do not add modifiers, mouse buttons, numpad, or arbitrary virtual keys.
- Outside capture, preserve existing keyboard passthrough and do not trigger ordinary mod hotkeys from keys consumed by active overlay UI controls.
- Input priority: Insert/reserved overlay handling, active rebind capture, normal mod hotkeys, then game input.

### Conflicts, actions, and persistence

- A physical key may be assigned to only one of the four actions. Do not swap or silently steal duplicate bindings.
- On conflict, keep the previous binding, show an Error toast such as `Hotkey conflict` / `F10 is already assigned to Cinematic Aspect`, and keep capture active for another attempt.
- Successful rebinding updates runtime state, persists through the existing production configuration path, immediately updates the displayed binding, and takes effect without restart.
- On restart, the persisted binding is restored.
- Preserve the existing F9–F12 action/mutation implementations. Change only the key-to-action lookup. Existing action toasts continue through their current path.
- A captured key must not emit an action toast. Successful rebinding emits an Applied toast such as `Gameplay Mode Hotkey` / `F9 -> F6`.
- `Hotkeys.Enabled=false` disables all four actions but keeps the overlay and rebinding available. Do not add per-action None bindings in this first version.

## Explicit non-goals

- Do not change Gameplay/Cinematics/Dialogue cycling semantics or their mutation path.
- Do not add modifier combinations, mouse/numpad bindings, arbitrary VK support, per-action disable/None, or automatic key swapping.
- Do not change camera logic or `CameraStateSnapshot`.
- Do not add a second hotkey configuration store.

## Expected files or areas

- `src/config/feature_config.*`, `src/config/config_repository.cpp`, and configuration template paths: validate/extend supported binding updates and persistence only as needed.
- `src/plugin/runtime_settings.*` and `src/plugin/runtime.cpp`: expose authoritative hotkey configuration mutation/snapshot and safely update running hotkey behavior while preserving existing actions.
- `src/overlay/input_state.*`, `src/overlay/discovery_runtime.cpp`, and `src/overlay/renderer_runtime.cpp`: capture lifecycle, keyboard routing, Hotkeys UI, tooltip invocation, and success/conflict notifications.
- `tests/overlay`, `tests/config`, and relevant runtime-settings harnesses: deterministic behavior and persistence coverage.
- `test.cmd`, `build.cmd`, and `build-overlay-settings.cmd`: run existing validation; edit scripts only if a test source must be registered.

## Implementation batches

### Batch 1 — Confirmed worker lifetime and capability setup

Keep one hotkey worker for the plugin lifetime, independent of initial `Hotkeys.Enabled`. Use the checkbox solely as the action gate. Source inspection confirms the worker loop already has a per-iteration enabled gate, but initialization also uses the setting to request cinematic/dialogue hooks. Because rebinding can enable these actions after launch, initialize the existing runtime components independently of this policy and retain their existing runtime policy checks, including Native pass-through behavior. The worker registry has three slots while runtime may start one hotkey plus three diagnostic workers; expand the bounded capacity to four and extend its deterministic lifecycle harness. Keep binding snapshots and INI writes synchronized while runtime rebinding occurs.

### Batch 2 — Deterministic rebinding state and persistence

Implement supported-key validation, one active capture, cancellation, duplicate rejection, immediate runtime update, authoritative persistence, and safe live enable/disable behavior. Add deterministic harness coverage before UI integration.

### Batch 3 — Overlay controls and notification feedback

Add the Hotkeys section below Dialogue in the left column. Wire capture to the tested state path, consume only capture keys, use existing tooltip and toast systems, and leave the right runtime column unchanged.

### Batch 4 — Validation and review

Run `test.cmd`, production build, overlay build, and `git diff --check`; perform read-only Git review. Do not launch the game automatically.

## Deterministic coverage

- F9 -> F6 success; old F9 no longer triggers; next F6 triggers the existing action.
- The F6 press used during capture is consumed and produces no action toast.
- Esc cancels; Insert closes and cancels; focus loss and teardown cancel.
- Starting capture on another action cancels the previous capture.
- Unsupported key is ignored and capture remains active.
- Duplicate key is rejected; old binding remains; conflict toast is queued; capture stays active.
- Hotkeys disabled blocks actions while rebinding remains functional.
- Successful change persists and reloads.
- Successful rebind toast appears; no action toast appears during capture.
- Keyboard passthrough outside capture remains intact.
- Live enable/disable does not create duplicate workers or leave a stuck key-down state.

## Runtime review after deterministic PASS

One combined runtime review should rebind all four actions, verify success/conflict/cancel feedback, close the overlay and verify new bindings, verify old bindings no longer act, toggle Hotkeys.Enabled while keeping rebinding available, then restart and verify persisted bindings.

## Risks and safe-failure behavior

- Capability initialization is the main behavior risk: hotkeys must remain operational when enabled after startup, while Native settings retain pass-through behavior. Gate active transformations on their existing runtime policies and exercise both states deterministically where possible.
- The hotkey worker reads configuration while the overlay can rebind it. Use a coherent thread-safe hotkey configuration snapshot or equivalent atomic fields; do not concurrently read and write the `g_config` key fields without synchronization.
- The fixed worker registry must have enough slots when optional diagnostic workers are compiled alongside the always-present hotkey worker.
- Input routing risk is limited by consuming only reserved/captured key messages and preserving existing passthrough elsewhere.
- Persistence failure must leave the visible/runtime binding consistent with the authoritative accepted state and report an Error rather than claiming Applied.
- On unsupported or conflicting input, retain the previous binding and capture state as specified.

## Stop conditions and phase gates

- Stop if runtime hotkey worker ownership or thread shutdown cannot be established from source.
- Stop if enabling the policy-gated hooks changes camera output under Native settings; present the concrete evidence before expanding scope.
- Stop if supporting live enable/disable requires changing existing action semantics.
- Stop if the settings configuration path cannot atomically apply and persist a binding without an overlay-only store.
- Keep in-game validation separate until deterministic and build validation pass.

## Expected final Git review

Compare all changed paths with this plan, preserve pre-existing dirty work, identify completed/remaining/deferred/blocked/not-runtime-validated items, and add a factual TASKLOG entry only after implementation and relevant static validation pass.
