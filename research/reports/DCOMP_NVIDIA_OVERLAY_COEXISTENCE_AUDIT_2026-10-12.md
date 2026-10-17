# DirectComposition / NVIDIA Overlay coexistence audit

Date: 2026-10-12. Mode: read-only source/platform-contract audit; documentation only.
Scope: current root-detach implementation, NVIDIA Performance HUD / G-SYNC
Indicator activation ordering, and possible presentation/composition interactions.
No source/vendor changes, builds, tests, game launch, installation or capture.

## Result

Root detach is retained, but NVIDIA coexistence is **unresolved**. Emptying the
root does not release the composition target's HWND-layer binding. This is a
concrete remaining integration dependency, not proof of NVIDIA's internal path.
No current evidence establishes an actual G-SYNC disable, an FG improvement,
an MPO transition or a change of the game's PresentMode.

## New user observations

These are user-reported visual observations in this conversation, not a new
instrumented capture. The precise ASI identity, GPU/driver, NVIDIA App version,
display mode and FG mechanism were not supplied with this follow-up. Earlier
A/B/C logs and fixture results retain their original scope.

| Sequence/state | Performance HUD | G-SYNC Indicator |
| --- | --- | --- |
| HUD enabled before game launch; mod visual detached | Disappears | Visible |
| Mod settings menu opened / visual attached | Appears | Disappears |
| HUD restarted by its shortcut, then mod menu opened | User describes changed behavior; complete HUD state after every subsequent close was not specified | Disappears when mod menu opens |

Do not expand the third observation into an unreported full state matrix. The
sequence makes HUD activation ordering relevant, but does not identify a
particular NVIDIA layer, target, hook or rendering API. Indicator visibility is
not a measurement of actual VRR operation.

## Current integration and dependencies

| Boundary | Source evidence | Consequence |
| --- | --- | --- |
| Target/layer acquisition | `src/overlay/presenter/renderer_runtime.cpp:848`, `CreateCompositionTarget`: CreateTargetForHwnd(game HWND, TRUE), falling back to FALSE only on failure | Uses the game's HWND and one of its two DComp target layers. The fallback log can show that TRUE was unavailable, not who occupied it or why it failed. |
| Settled close | `renderer_runtime.cpp:1400`, `ReconcileCompositionAttachment`: SetRoot(nullptr) + Commit; target/device/visual/surface remain resident | Removes this visual tree's content, not the target binding. It is not equivalent to Overlay.Enabled=false. |
| Open/toast | Same reconciliation method requests the existing visual; successful surface preparation precedes attachment and notification lifetime publication | Adds composition content again without acquiring a new target on each toggle. |
| Target release | `renderer_runtime.cpp:1274`, `ResetComposition` empties root then releases target, visual, surface and devices | Existing release occurs for reset/rebind/recovery/shutdown, not ordinary close. |
| Input | `src/overlay/composition/composition_runtime.cpp`, `ProcessOverlayWindowMessage` and `InstallInputHook` | Delete uses the game window subclass, independent of root attachment. Visible UI captures relevant input; hidden input passes through. No NVIDIA-control API is used. |
| Presentation | Private D3D11 device and IDCompositionSurface; no Overlay game swapchain / Present interception found | No direct mutation of game Present flags is established. This does not imply neutrality of the shared OS composition path. |

### Platform contracts

Microsoft specifies two target layers per HWND, at most one target per layer.
A target already bound to a layer causes another target creation there to fail;
destroying the target makes the layer available again. `topmost=TRUE` concerns
ordering relative to that HWND's child windows; it is not a universal ordering
control over every third-party overlay.
[CreateTargetForHwnd](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-createtargetforhwnd).

SetRoot(nullptr) makes the tree empty; it does not destroy its target.
[SetRoot](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiontarget-setroot).
Commit submits a device's pending command batch; different devices do not have
a shared atomic transaction. Successful application-side publication is not
proof of final display visibility.
[Commit](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-commit),
[asynchronous architecture](https://learn.microsoft.com/en-us/windows/win32/directcomp/architecture-and-components).

Microsoft also documents that content above an independently flipped application
may cause composed mode, reverse composition, or MPO-assisted independent flip.
Which occurs here is unknown; visible content does not necessarily disable
Independent Flip.
[DXGI flip model guidance](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model).

## Answers to the three audit questions

1. **Does attach/detach change the game's actual presentation path?** It changes
   this HWND's composition content by construction. A game PresentMode/plane/VRR
   change is not established without a graphics trace.
2. **Can HUD visibility change without the HUD being disabled?** Yes as a
   possible mechanism: compositing order/path and cached overlay activation can
   change visible output. No source call disables NVIDIA HUD. However NVIDIA's
   internal enabled/rendering state is not observed, so occlusion versus stopped
   rendering versus path selection remains unknown.
3. **Can we display our Overlay without affecting already active NVIDIA UI?**
   No guarantee follows from current evidence or these APIs. There are narrower
   candidate experiments than replacing Renderer, but none is yet a proven fix.

## Findings and minimal confirmation

| ID / status | Finding | Minimal next evidence |
| --- | --- | --- |
| COEX-01 / confirmed structure; causal hypothesis | Empty-root target still occupies an HWND DComp layer. If NVIDIA uses the same HWND/layer, activation order could matter. NVIDIA using it is unverified. | If separately approved, one temporary target-release-on-idle comparison retaining private device/surface/input; reacquire target before UI/toast. Record layer choice/HRESULT and the same HUD-first sequence. A changed symptom would implicate target lifetime, not prove NVIDIA internals. |
| COEX-02 / hypothesis | Active composition changes presentation/plane eligibility and the two NVIDIA components react differently. | One matched non-OSD PresentMon/graphics-ETW capture spanning idle -> mod open -> mod close -> HUD restart -> mod open/close; mark observed HUD/indicator visibility and inspect the game stream's PresentMode. |
| COEX-03 / evidence gap | NVIDIA activation state/path is not captured, so ordering cannot be reduced to a global z-order explanation. | Preserve a short visual record and activation timestamps with NVIDIA App/driver versions; do not call invisible HUD disabled. |

COEX-01 is not a confirmed bug in the accepted resident-resource contract. The
contract guarantees an empty root when settled, not release of every composition
binding. New observations reopen the compatibility question, not camera ownership.

## Alternatives, risks and recommendation

- **Keep current detach:** removes unnecessary hidden visual content; does not
  guarantee HUD coexistence or free its target layer. Retain it now.
- **Release only the target while idle:** smallest candidate to isolate retained
  layer occupancy. Requires separate temporary implementation approval. Risks:
  another component claims the layer before reopen; FALSE fallback, failed
  reacquisition, notifications, resize/rebind and cleanup need explicit handling.
  Do not add automatic retries, full Renderer destruction or change Delete.
- **Prefer FALSE permanently:** not recommended from this audit. Both targets
  overlay directly presented HWND content; FALSE only changes child-window
  ordering and could conflict with another occupant.
- **Separate overlay HWND or game-swapchain rendering:** larger integration
  changes with focus/input, presentation and lifetime risks; no evidence justifies
  either migration now, and neither guarantees NVIDIA coexistence.

Recommended next step is one sequence-focused external capture, not another broad
A/B/C matrix or a permanent backend change. If it cannot identify a presentation
transition, the separately approved target-lifetime experiment is the bounded
discriminator. Actual FG/VRR measurements remain distinct from UI visibility.

The prior full-suite/build/hidden-HWND PASS results remain unchanged; they never
tested NVIDIA coexistence. No new tests were needed for this documentation-only
audit. N03-N06 and established runtime/decomposition decisions are unchanged.

## Subsequent user decision

The user prioritized a single temporary target-release experiment before external
tracing. It is now prepared and offline validated in an isolated source copy;
see the [experiment report](DCOMP_TARGET_RELEASE_EXPERIMENT_2026-10-12.md).
This supersedes the next-step ordering above, not the audit's evidence limits.
Canonical production retains root detach; no target-release adoption is approved.

Later approved research evaluated alternatives without recreating a game-HWND
target. See [persistent-target coexistence research](DCOMP_PERSISTENT_TARGET_COEXISTENCE_RESEARCH_2026-10-12.md).
Original C-root ASI with indicator-return live evidence was pinned separately;
no implementation or new runtime testing was performed in that follow-up.
