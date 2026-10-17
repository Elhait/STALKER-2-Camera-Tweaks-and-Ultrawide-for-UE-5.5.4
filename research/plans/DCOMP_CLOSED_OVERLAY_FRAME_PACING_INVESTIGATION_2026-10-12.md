# DirectComposition closed Overlay / frame pacing investigation

Date: 2026-10-12. Authorized scope: architecture audit, temporary A/B/C experiment,
controlled runtime comparison, findings before any permanent production change.

Baseline: `e756442bcf9942923b4531fb513af893f6aecae7`; initially clean working tree.

## Contract and acceptance

The existing Renderer and composition owner thread retain ownership. A is the
configured-disabled path; B preserves the current attached-hidden behavior; C
detaches the target root only when settings are hidden, no notification remains,
startup notification creation has finished, and the final frame has succeeded.
Retain resources, window subclass, Delete, notification behavior, source identity,
camera policies and synchronization. Reattach only after a successful current
surface draw, before notification lifetimes start. Root/Commit failure disables
the optional experiment Overlay through the existing boundary.

Instrument B/C identically: bounded transition/snapshot logs and cumulative draw,
render and Commit counters. A/B use the same instrumented binary; A changes only
Overlay.Enabled. C differs only by compile-time root-detach policy. No new INI
keys. Commit acceptance is not compositor acknowledgement or frame-time proof.

## Work and files

1. Audit current `src/overlay/composition`, presenter, input and runtime startup.
2. Research tooling copies tracked build inputs into an ignored snapshot and
   applies checked mechanical insertions there. Production source and CMake are
   read-only. Build separate B/C trees; produce correctly named
   `STALKER2CameraTweaks.asi` with settings and hashes in A/B/C bundles.
3. Compile experiments and run focused presenter/input/optional-startup checks.
   Check baseline input hashes and camera source parity. Build success does not
   exercise graphics attachment in the game.
4. Runtime: same save, route, game/driver/settings, six A/B/C x FG OFF/ON pilot
   cells; repeat apparent differences at least three times with rotated order.
   Start capture after startup toast and UI close have reached idle. Record
   display pacing, application pacing, PresentMode, spikes and lifecycle events.
5. Record observations and unresolved measurements in the report. A production
   decision requires reproducible runtime evidence and a separate review.

Research files: this plan, the report in `research/reports`, temporary probe assets
in `research/prototypes/overlay_attachment`, and preparation/build tooling in
`research/tools`. Generated snapshots, manifests, ASIs, INIs and raw outputs stay
under ignored `build-artifacts/fp` (earlier long-path trial remains ignored).

## Risks and gates

Resize draws its replacement off-tree and publishes before reconciling attachment.
Device recreation can briefly create an attached initial surface; measure only
after the next accepted idle transition. Minimized or pending-toast states are
excluded from steady-state comparisons. The supported lifetime remains resident
until process termination; no dynamic unload or new shutdown API is introduced.

The RX 9070 XT report is user-supplied context. FG timing and driver/DWM activity
cannot be inferred from no application draw/Commit calls. If no suitable game
session/capture is available here, runtime cells remain Not exercised and the
prepared experiment is handed off for user-run capture.

## Approved production adoption — 2026-10-12

The user approved adopting the verified C lifecycle after live A/B/C showed the
NVIDIA indicator remaining in A, disappearing in attached-hidden B, and returning
after each of three closes in detached C. This addresses unnecessary closed-state
composition presence and the observed interaction with another overlay. It does
not assert a measured FG/VRR or presentation-mode fix.

Scope: existing Renderer owns an accepted root-attachment flag; reconcile root
after successful frame/resize publication and before notification lifetimes.
Commit failure closes only Overlay through its existing terminal boundary. Preserve
Delete and all resource/input/worker ownership. No prototype counters, switches or
clock-demand logging enter production. A small transaction helper beside existing
surface publication helpers permits failure/order checks without native seams.

Files: renderer_runtime.hpp/.cpp, composition_presenter_state.hpp, the existing
composition presenter harness, architecture/safety contracts and this report.
The localization source audit's startup-lifetime check follows the new propagated
attachment result; it still requires successful rendering and committed toast
publication, now additionally requiring attachment reconciliation.
Research tooling may reuse the real-Renderer fixture against actual production
objects, with output confined to ignored build-artifacts/detach.

Validation: focused attachment ordering/failure tests, full canonical test.cmd,
then production build, then real-Renderer HWND smoke for open/close, notifications,
resize, minimize/restore, rebind and explicit shutdown. Preserve camera source,
configuration, test inventory and release packages. Do not install or launch the
game. The new production artifact gets its own live-validation status; successful
experimental C observations remain evidence for C, not a test of that artifact.

Broader active-UI coexistence, actual VRR, FG pacing, other wrappers and DWM/MPO
mechanisms remain deferred investigations triggered by captures or a concrete
reproduction; this adoption does not expand into those changes.

Implementation status: production adoption and offline validation completed.
Canonical test.cmd and subsequent production build.cmd returned exit 0. The actual
production Renderer HWND fixture passed notification expiry, repeated open/close,
closed resize, visible/closed minimize/restore and HWND rebind, and explicit
Renderer shutdown. Artifact: build-artifacts/detach/STALKER2CameraTweaks.asi;
SHA-256 BF5B1E9460D5A75AF32D0CB09A86CCDE6FA86A0B1EE04BBF569CDA373391ABCD.
Live smoke of this new production artifact remains pending; no game installation
or release-package replacement was performed. Broader pacing research stays open.
