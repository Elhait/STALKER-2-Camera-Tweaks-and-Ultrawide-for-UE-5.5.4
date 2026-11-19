# Overlay / Shared Camera State Remediation — Audit Findings F6–F8

**Updated:** 2026-10-07  
**Scope:** Findings F6–F8 from `OVERLAY_SHARED_CAMERA_STATE_ARCHITECTURE_AUDIT_2026-10-07.md`. These IDs belong to the Overlay/shared-snapshot audit and are separate from the F6–F8 IDs in `CAMERA_MODULE_REMEDIATION_F1_F8_TASK_PLAN.md`.  
**Disposition:** F6–F8 implementation and offline characterization complete. No camera correction, coordinator transition, ownership, or transaction behavior was changed.

| Finding | Established contract / bounded result | Regression evidence | Status |
| --- | --- | --- | --- |
| F6 — owner/state metadata | `CameraFovTransactionPublication::transactionOwner` is the context of one FOV publication and is used only for transaction publication policy. `CameraStateSnapshot::presentation.state` remains the last full-publication presentation; it is not rewritten by transaction publication. The live coordinator is an independent semantic snapshot fact. Diagnostics now label all three distinctly; expanded Camera State shows last full publication state and current coordinator separately. | `camera_writer_transaction_harness` checks a Cinematic transaction retains the Gameplay baseline without relabeling the last full-publication state. Localization/source audit checks distinct UI labels. | **CLOSED — intentional contract documented + regression** |
| F7 — cinematic target re-resolution | Overlay value is a selection/viewport **preview**, not the production-resolved or confirmed-applied aspect. Auto preview uses current client viewport. Production may fall back to display/native aspect and may reject a store; those outcomes are not available in this preview projection. Field/API and UI label now say preview, and tooltip states the evidence boundary. | `camera_integration_harness` checks arbitrary Auto preview derives from viewport and does not claim a production resolution. | **CLOSED — presentation contract clarified + regression** |
| F8 — writer aspect availability | Summary is a live view and exposes writer/policy aspect only while writer observation freshness is `Current`. Expanded details may show the valid last-observed number while `Historical`, with writer freshness directly adjacent. These are intentionally different levels, not contradictory values. | `camera_integration_harness` checks historical facts are suppressed from live projection while remaining valid in the shared snapshot with Historical freshness; localization/source audit checks the detailed freshness row. | **CLOSED — contract clarified + regression** |

## Validation

- `camera_writer_transaction_harness`: PASS.
- `camera_integration_harness`: PASS.
- `localization_catalog_audit.ps1`: PASS; 18 catalogs, key parity and embedded resources validated.
- `test.cmd`: all 58 harnesses compiled and executed; camera, projection, persistence, and overlay harnesses reported PASS. The command still exits nonzero because `localization_harness` reports missing CJK text glyphs. This batch did not change font ranges or embedded-font policy; the full suite is therefore **not PASS** for this revision.
- `build.cmd`: PASS; production `STALKER2CameraTweaks.asi` emitted (3,386,368 bytes; SHA-256 `0AC6E0A9880E1DED6F5E9BAD1936D5C6F9C3FA4BC42F49F028D673E5299D4212`). No game launch or ASI installation.

## Deferred boundaries

- F9 state consolidation remains out of scope; retain the field/consumer mapping from the architecture audit until lifecycle reset semantics are established.
- F10 known camera-owner boundary freshness is remediated offline; Main Menu/save-load session reset remains open because current production source exposes no authoritative lifecycle event. See [F10 lifecycle plan](OVERLAY_SHARED_CAMERA_STATE_F10_LIFECYCLE_PLAN.md). F9 consolidation remains deferred.
- Runtime acceptance remains deferred until F6–F10 work and the final independent audit are integrated; this batch does not request a game launch.
- No claims are made about rendered Overlay appearance, real callback interleaving, or final camera behavior from offline tests/builds.
