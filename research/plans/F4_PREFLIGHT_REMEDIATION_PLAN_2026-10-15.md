# F4 Preflight Remediation — R1–R7

Status: Complete — implementation and offline validation; live qualification pending.
Baseline: `F4_PREFLIGHT_CUMULATIVE_CODE_AUDIT_2026-10-15.md`.

## Scope and execution

Apply seven bounded batches in order, with focused checks and preservation of
earlier invariants. No broad camera changes, source splits, architecture campaign,
game launch, ASI installation, release packaging or index mutation. Keep the
ordinary ASI and its INI Diagnostics switch. N03 remains Partially mitigated;
N04–N06 remain Open. Preserve all pre-existing staged work.

| Batch | Findings | Intended change/files | Focused acceptance |
| --- | --- | --- | --- |
| R1 | PF01 | SHA scoped backing storage and ownership fixture | Destruction order safe on formatting/allocation failure; OFF/ON ownership checks |
| R2 | PF05/PF06 | Config template and persistence harness | Logical duplicate sections preserve effective settings; legacy Gameplay true/false precedence; no-op identity |
| R3 | PF03/PF08 | Composition input binding, startup language handshake, focused Win32 fixture | First prove predecessor publication race; then bind on a safe callback boundary and issue language request only after binding commit; no game-thread waits |
| R4 | PF02 | Presenter activity reconciliation and focused host lifecycle cases | Missed startup activation recovers; actual deactivation/minimize remains fail-open |
| R5 | PF04 | Menu display commit/readiness and focused cases | Toast-only display cannot admit interactive input; current menu frame must commit |
| R6 | PF07 | Backend ownership cleanup and focused failure cases | Partial backend resources released on owner; uncertain DComp retention preserved |
| R7 | PF09 | CTest commands and contained runner | Direct CTest scenarios use the same Job Object supervisor; failure/timeout/descendant cleanup |

PF02 and PF04 remain separate patches. PF08 requires a deterministic interleaving
before changing subclass installation; do not add speculative mutexes or waits.
Use existing test targets where appropriate and keep canonical inventory explicit.

All launched checks have finite external process-tree supervision and bounded
output; compilation is serial. Stop dependent execution if cleanup is unresolved.
Run the full canonical suite once after R1–R7. Then review the aggregate diff,
build one isolated production ASI and record SHA-256. Live qualification is a
subsequent user-run step; historical installed/rollback files remain untouched.

## Risks and gates

Preserve native continuation, GameHwnd input/language affinity, one Overlay
graphics owner, event-aware finalization, settings migration precedence, atomic
INI writes and executable retention. Any materially different ownership contract
needs a documented bounded decision before implementation. Record actual results
in the implementation report; compilation alone cannot establish runtime PASS.

## Progress

R1–R7: implemented; focused checks passed. One canonical regression completed:
59 targets / 62 invocations / 210 adapter processes / 4 Zoom scenarios, exit 0,
cleanup verified. Isolated ordinary ASI build passed, SHA-256
`24A50EAF166FD52E68AD90A5712C1415E5DC6CC4BA6D4157CC4BD53605A43E86`.
Live qualification remains pending; installed/root ASI and rollback unchanged.
See `research/reports/F4_PREFLIGHT_REMEDIATION_IMPLEMENTATION_REPORT_2026-10-15.md`
for actual results, failed attempts, cumulative review and evidence limits.

## Bounded implementation decisions

- R3: a real Win32 interleaving reproduced dispatch through a stale predecessor
  while the actual predecessor was a foreign subclass. Installation now runs on
  the GameHwnd thread through a temporary, thread-specific WH_GETMESSAGE hook.
  The shared installer refuses off-thread calls, publishes the actual returned
  predecessor before the next native dispatch, and keeps stable binding records.
  The existing Overlay owner receives completion asynchronously and only then
  enables startup language preparation. No game-thread wait, Renderer lock,
  settings persistence or graphics cleanup is added. Foreign-process/zero-thread
  targets are refused, not converted to global hooks. The owner unhooks the
  bootstrap observer on completion/finalization; failure is terminal/fail-open.
  Host replacement during an unfinished binding terminates the optional Overlay
  rather than reusing an ambiguous binding. This is not a claim of live proof.
- R4 and R5 are separate changes: current foreground reconciliation repairs
  activation; owner-local menu draw/commit acceptance governs input admission.
  A toast-only surface cannot grant interactive capture.
- R6: the production owner cleanup always shuts down the backend, irrespective
  of its success flag. A software-only WARP 9_1 test deliberately fails the
  production shader pipeline after acquiring real D3D references and verifies
  that DetachImGui clears them and destroys its context. No game HWND, DComp
  target, hardware renderer or capture session is created by this new case.
- Registry baseline is the actual current 59 targets / 62 runner entries /
  104 adapter cases (208 OFF/ON processes), not older 204-process reports.
  The new R6 case makes 105 cases / 210 processes. The source registry and
  strict inventory audit are updated together; no existing case is removed.
- R7 uses the existing Job Object supervisor for direct CTest, finite inner
  deadlines and a longer CTest outer timeout. CTest cases are serial. Its
  contract regression tests signed nonzero propagation and descendant timeout
  cleanup; it does not substitute an exit code for cleanup verification.
