# F5 — Startup Overlay & Welcome Toast Final Hardening

Date: 2026-10-15  
Result: **No production change justified by available evidence; startup-scene readiness remains unobservable under the current no-hook constraint.**  
Scope: bounded source and existing-evidence investigation. No game, graphics fixture, tests, build, ETW or PresentMon was run in this task.

## Executive summary

The startup welcome toast is gated by Overlay presenter readiness, completed input-hook binding, and (when enabled) Auto-locale synchronization. It is **not** gated by the first stable game frame or by a confirmed visible gameplay scene.

The current log establishes that the Overlay's own DirectComposition surface was created, drawn and committed; it does not establish that the game had presented a stable scene or that the toast pixels reached the display. The available camera lifecycle snapshot is observational camera/runtime state, not a game-frame or compositor-presentation acknowledgement. Game HWND visibility, foreground state and non-minimized geometry likewise do not prove scene readiness.

The earlier standalone desktop fixture captured toast pixels in a surrogate host, including a hidden-window negative control. That proves the production Overlay component can render a toast in that desktop setup. It does not resolve the game-side startup timing question. A previous desktop investigation included a display blackout requiring a forced restart; under the project's safety rule, this task did not repeat graphics experiments while the user was away.

**Disposition:** do not introduce a guessed delay, extend the TTL, add a Present/swapchain hook, or make camera lifecycle a proxy for actual displayed frames. Those changes would not establish the missing fact. Preserve the working menu path and current startup behavior pending one safe live observation that can distinguish game-scene readiness from Overlay commit.

## Startup lifecycle reconstructed from source

| Stage | Current condition/action | Source | What it proves / does not prove |
|---|---|---|---|
| Camera runtime initialized | Runtime publishes camera-core readiness and notifies Overlay discovery. | `src/plugin/runtime.cpp:6827-6833`; `src/overlay/composition/composition_runtime.cpp:1699` | Runtime initialization completed; not that a game scene was drawn. |
| Game window selected | Discovery chooses a visible process top-level window and checks geometry/minimized state. | `src/overlay/composition/composition_runtime.cpp:1231-1278` | A candidate HWND exists and is usable for placement; not that its client pixels contain a stable scene. |
| Renderer ready | Private D3D/DComp resources and Overlay HWND target/surface are initialized; an initial full Overlay surface draw and DComp commit are accepted. | `src/overlay/presenter/renderer_runtime.cpp:815-860`; `composition_presenter_state.cpp:50-65` | Overlay presenter/surface readiness only. |
| Input binding ready | Game-thread WndProc binding is committed and acknowledged to owner. | `src/overlay/composition/composition_runtime.cpp:1068-1135`; `renderer_runtime.hpp:49-54` | Input callback binding is available; not a presentation signal. |
| Startup hint may wake presenter | Pending hint requires presenter `Ready` and `inputBindingReady`. | `src/overlay/presenter/renderer_state.hpp:17-31`; `renderer_runtime.hpp:100-110` | Readiness is Overlay-owned, not game-frame readiness. |
| Auto locale | If Auto locale is enabled, an accepted synchronization request is awaited before hint creation; failed request uses the existing English fallback path. | `src/overlay/presenter/renderer_runtime.cpp:1568-1605` | Locale selection is resolved; no information about game scene presentation. |
| Toast created | A single startup notification is added to the queue, then startup state is marked created. | `src/overlay/presenter/renderer_runtime.cpp:1548-1565` | Notification exists in presenter state; not yet visible pixels. |
| Draw submitted | ImGui notification draw-list evidence and submitted ID are recorded. | `src/overlay/presenter/renderer_runtime.cpp:1688-1778` | Toast geometry was submitted by ImGui; not proof of display output. |
| Composition commit / TTL | After the surface draw/commit path, the owner reconciles attachment/display and starts lifetime only for a submitted notification while the Overlay window is visible. | `src/overlay/presenter/renderer_runtime.cpp:1198-1228, 1657-1686` | TTL does not run during Auto-locale wait or while the Overlay HWND is not visible. DComp commit is not a monitor-pixel acknowledgement. |
| Expiry / detach | Expired notifications are removed; when no menu, toast or pending hint remains, the root is detached and the display window hidden. | `src/overlay/presenter/renderer_runtime.cpp:1522-1544, 1616-1650` | Normal expiry can explain the later detach; logs without per-event timestamps cannot prove elapsed wall time or user-visible duration. |

The startup journal calls its marker `OVERLAY_FIRST_FRAME`, but its detail is `first_directcomposition_surface_commit`. It means the first Overlay surface commit, **not** the game's first frame.

## Existing evidence reviewed

### Current installed-run logs

The available log set records:

- `CAMERA_CORE_READY` before game-window selection.
- `COMPOSITION_DEVICE_READY`, `COMPOSITION_SURFACE_READY`, `RENDERER_ACTIVATED`, and `OVERLAY_FIRST_FRAME` for the Overlay's own surface.
- `INPUT_ACTIVATED` after WndProc binding.
- Overlay log state `host_active=1`, `host_minimized=0`, `display_visible=1`, `z_order_above_game=1` before automatic toast creation.
- `OVERLAY_STARTUP_HINT_WAITING_FOR_AUTO_LOCALE`, followed by creation, lifetime start, and `DRAWN_AND_COMMITTED`.
- Later root detach/hide before the first manual `OVERLAY_VISIBILITY=VISIBLE` event.

The main runtime log records Auto-locale synchronization, but the Overlay log does not timestamp each toast transition. These records prove automatic toast processing and a favorable sampled host/display state. They do not prove the game scene was ready or that toast pixels were visible in the actual game at that time.

### User clarification after initial F5 conclusion

The user clarifies that the welcome toast **did not disappear before the Overlay was opened manually**. This direct live observation supersedes any interpretation that the logged detach proves the user-visible toast had naturally expired before manual open. It does not establish that the toast pixels were visible before opening; the earlier report was that the toast was only noticed when opening the menu.

There is an unresolved discrepancy: the log orders root detach before the manual visibility event, while the source's reconciliation predicate keeps the root attached whenever a notification remains queued. Since the log has no per-transition timestamps or notification-state snapshots, it cannot explain whether the toast was still pending, was already expired, or became visible only after a later redraw/reattach. Do not label the detach as confirmed TTL expiry from this log alone.

### Existing desktop fixture

`research/reports/INVISIBLE_OVERLAY_DESKTOP_INVESTIGATION_2026-10-15.md` records a bounded standalone host run with a pixel-capture oracle, toast/menu regions, a deliberately hidden negative control, z-order/input checks and verified process cleanup. The corrected current production component passed that surrogate-host test. This is useful component evidence, but not integrated game-startup evidence. The same report records a prior blackout/forced-restart incident; the fixture was not rerun for F5.

## Game-ready signal assessment

| Candidate signal | Available? | Why it is insufficient as a stable-scene guarantee |
|---|---|---|
| Game HWND exists / selected | Yes | Window discovery and geometry can precede loading completion. Existence is explicitly not treated as readiness. |
| Foreground, visible, non-minimized, valid geometry | Yes | Describes window state and placement, not whether the scene is stable or its pixels have been presented. |
| Camera-core initialization | Yes | Indicates runtime/hook initialization, not an observed game render. |
| Camera state `Gameplay` / `CinematicActive` | Available through the semantic snapshot after camera callbacks publish it | Classifies camera/runtime lifecycle. It is not an acknowledgement of a rendered or displayed game frame; state may be unavailable during menus/loading and is not atomic with compositor output. |
| Existing window move/activation/restore events | Yes | Can trigger Overlay reconciliation but do not signal completion of loading or a first stable scene. |
| First game Present / stable displayed frame | No approved signal found | Current Overlay explicitly has no DXGI factory/swapchain hooks; adding Present/Streamline interception is outside this task's constraints. DComp `Commit` is asynchronous and concerns the Overlay composition tree. |

Therefore, the current source has no reliable, already-available **Game Ready** signal matching the requested meaning. A first gameplay camera callback would be a new proxy contract, not proof that a stable scene reached the monitor. No such proxy was introduced.

## Alternatives considered

- **Defer until foreground/visible/non-minimized:** already effectively required for display/TTL; does not distinguish a loading screen from a stable scene.
- **Use camera `Gameplay` state:** not equivalent to rendered-frame readiness and can delay indefinitely in menus or non-gameplay startup. It would couple an optional UI hint to camera intervention/lifecycle without proving the desired output.
- **Bounded startup grace period:** an arbitrary delay can still expire during a long load and would merely move the failure boundary. No evidence supports a duration.
- **Pause TTL while HWND is hidden/minimized/inactive:** TTL already starts only after a committed notification while the display HWND is visible. This cannot detect a visible-but-not-yet-useful game scene.
- **Extend TTL or show repeatedly:** explicitly rejected; no evidence justifies it, and it changes notification behavior without solving the observability gap.
- **DXGI/Present hook or polling:** prohibited by this task and the production Overlay contract; would add compatibility/lifetime risk.

No production change was made because none of these alternatives is both causally supported and within the existing guarantees.

## Regression and safety status

- Production source/configuration, installed ASI/INI, release artifacts, tests and build configuration: unchanged by this F5 investigation.
- Focused deterministic tests and full canonical `test.cmd`: **not run**; no production behavior was changed, and the existing test suite cannot establish actual game-scene display timing.
- Production build / ASI candidate: **not produced**; no source fix was justified.
- Desktop graphics qualification: **not repeated** due the previously recorded blackout incident and the task's no-repeat safety condition.
- Existing candidate identity from the preceding investigation: `9ECB5E637BBDAA6D5658AFB76A764592CAB167CD9E0B8264386AD8E59EA84643`. This is historical provenance only, not an artifact built or validated by F5.
- N03 remains `Partially mitigated`; N04–N06 remain `Open`.

## Remaining evidence needed

One controlled live observation is still needed to close the startup-toast question: correlate a visual capture of the actual game scene and toast with the startup sequence, and capture whether the toast remains queued/visible through manual open. It should be performed only when the user is present and the game is already being tested; no separate high-risk graphics stress test is warranted.

If the toast is absent while the game scene is visibly stable, the next investigation should compare captured pixels and Overlay visibility at that same time. If it appears during loading but is not noticed, that is a product-timing decision rather than proof of a rendering defect. Until such evidence exists, retain the current menu/z-order implementation and do not guess at a new Game Ready contract.
