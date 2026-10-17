# DComp target-release experiment

Current disposition: **Preserved experimental baseline / promotion held**, not
REJECTED. Latest user decision supersedes rollback proposals: retain production
root detach and both target-release/timing artifacts, without deleting or reverting
the compatibility work. See the report's artifact-history reconciliation.

Date: 2026-10-12. Approved scope: temporary experiment only.

Hypothesis: a retained empty-root target on the game's HWND may affect another
overlay. NVIDIA using the same target layer is unverified. Compare current
root-detach with target release, using the same INI and HUD activation sequence.
No new A/B bundles or configuration keys are required.

Implementation: copy current tracked build inputs, including working-tree edits,
into ignored build-artifacts/tr/source. Patch only that copy. After accepted
SetRoot(nullptr) + Commit, release only IDCompositionTarget. Retain private D3D11,
DComp device, visual, surface, ImGui and Delete. Before the next visible menu/toast,
reacquire target (existing TRUE/FALSE fallback), then attach/commit. Failed
reacquisition disables only the optional Renderer; no retries or uncertain reuse.

Expected research files: preparation script, temporary reconciliation source,
existing HWND fixture/runner extensions, experiment report. No canonical source,
vendor, build registry, installed ASI, release archive or Git index changes.

Validation: checked patch anchors; baseline hashes unchanged; existing root
transaction regression; isolated candidate build; real Renderer hidden-HWND
fixture for startup, three toggles, idle, toast expiry, resize, minimize/restore,
rebind and shutdown. Prove actual layer availability after release, occupancy
while open, and fail-closed reopen with both layers held by controlled foreign
targets. Log target create/release transitions and HRESULT/layer decisions only.
These tests prove API/lifecycle behavior, not NVIDIA compatibility or FG/VRR.

Handoff: one correctly named STALKER2CameraTweaks.asi. User installs manually and
repeats HUD-before-game -> startup hint expiry -> menu open/close -> HUD shortcut
restart -> menu open/close. Keep existing INI/settings. If symptoms persist, move
to presentation-path capture rather than redesigning Renderer. No permanent
production adoption before the game result is reviewed.

Preparation and offline gates passed. Isolated build and actual Renderer HWND
fixture exited 0, including real layer availability, FALSE fallback and both-layer
contention failure containment. Canonical input hashes are unchanged; only the
snapshot renderer_runtime.cpp differs. See the [experiment report](../reports/DCOMP_TARGET_RELEASE_EXPERIMENT_2026-10-12.md).
Game validation remains pending; production adoption is not approved.

Subsequent live result: HUD visibility improved according to the user, but a
second launch crashed and the following launch's mod UI was invisible. Dump/game
log establish a Streamline/DLSSG Present fatal error (DXGI_ERROR_INVALID_CALL);
target-release causality is not established. Newer mod logs show successful
initialization and two accepted open/close transactions, not terminal target failure.
Production adoption is held. Preserve offline results as bounded evidence, not
an overall PASS. See the report's live/crash section; no further implementation
or automatic installation is authorized by this diagnosis.

Current phase: existing-evidence lifecycle analysis, not further game experiments.
User reports a visible-toast-expiry stall with temporary loss of both NVIDIA
elements, absent when the toast is not seen while the window is inactive. The
review confirms shared owner/window-procedure mutex coupling and asynchronous
submission/completion limits; it finds no local concurrent target mutation.
No slow API is measured by current logs. Production retains root detach; target
release adoption remains held. No new game tests, waits or code fixes are requested.

Artifact-history reconciliation verified original C=78357...624E is root detach;
E-TR=5D2A...B55B adds target lifetime changes; E-Timed=011E...D8FB adds only timing.
The original E-TR feedback and 21:21:36 archived run already report problems before
timing instrumentation. No extra lifecycle change between those two target-release
artifacts was identified. Preserve each result; do not claim E-TR is a proven
stable rollback or reject the entire research direction. No additional code/test
work or installation was performed for reconciliation.
