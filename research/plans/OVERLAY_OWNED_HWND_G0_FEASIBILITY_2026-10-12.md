# Overlay-owned HWND G0 feasibility

Status: approved isolated experiment; G1–G5 and Renderer integration unapproved.

Implement one standalone native EXE with a surrogate game HWND on its main thread
and an owned layered Overlay HWND/DComp target on a worker thread. No mod sources,
game HWND, hooks, ASI or Renderer integration. Keep one target for the entire test.

Measure screen pixels for transparent, half-alpha and opaque regions; use real
SendInput delivery to distinguish passive click-through from the first interactive
click without activation. Check foreground/focus before and after. Synthetic
input is bounded to fixture windows; abort if another foreground window takes over.
Save screenshots/logs under ignored build-artifacts, plus a durable result report.
Compile only this fixture, preserving production and historic ASIs. Do not run
canonical tests/builds or game. Stop after G0; a failure revises the design rather
than silently substituting styles or a different rendering backend.

Validation limits: local Windows desktop/driver only, not NVIDIA/FG compatibility,
not production cleanup handoff. DwmFlush may be used solely to sample settled
fixture pixels; no production wait is proposed.

## Execution disposition

G0 local feasibility PASS, fixture exit 0, after sandbox input delivery was
identified as unavailable and the same final EXE was run with desktop permission.
Pixels, passive click-through, first interactive click/focus, one persistent target
and absence of a host-target call verified. See
[result report](../reports/OVERLAY_OWNED_HWND_G0_FEASIBILITY_RESULT_2026-10-12.md).
No deviation to production scope. G1–G5/Renderer integration remain unapproved.
Await user review; no task-log closure is asserted here.
