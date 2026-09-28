# Research archive

This archive preserves the reverse-engineering path behind the unified v0.4.0
release. Plans are organized by the result of the bounded investigation, not
only by whether implementation work was performed.

## Completed

Established findings, validated implementation phases and completed supporting
work are in [`completed/`](completed/).

## Rejected

These branches produced negative or unsafe evidence and should not be repeated
without new contradictory evidence: [`rejected/`](rejected/).

## Deferred

Open questions or deliberately paused investigations are in
[`deferred/`](deferred/).

## Reports and evidence

Durable technical reports belong in [`reports/`](reports/), and reusable
evidence that should stand independently of a task plan belongs in
[`evidence/`](evidence/). The reports include the [2026-09-27 Overlay/DXGI
startup crash investigation and repair history](reports/OVERLAY_DXGI_STARTUP_CRASH_INVESTIGATION_2026-09-27.md)
and the [adversarial Overlay crash-safety audit](reports/OVERLAY_ADVERSARIAL_CRASH_SAFETY_AUDIT.md).
The [final production safety/performance audit](reports/OVERLAY_FINAL_SAFETY_PERFORMANCE_AUDIT_2026-09-27.md)
records the remaining replacement, pending-resize, factory-coverage and exception-boundary findings with reproducible offline probes.

For the concise current state, see
[`TESTING_AND_RESEARCH.md`](../TESTING_AND_RESEARCH.md).
