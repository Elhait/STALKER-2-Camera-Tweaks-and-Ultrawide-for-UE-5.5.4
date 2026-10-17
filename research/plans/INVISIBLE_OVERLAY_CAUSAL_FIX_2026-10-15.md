# Invisible Overlay — bounded causal investigation

Authorized: restore autonomous desktop validation and implement a causal fix,
without launching the game or touching installed ASI/INI. Desktop/focus tests
explicitly authorized by the user on 2026-10-15.

1. Restore actual production presentation in a surrogate host. Prefer linking
   the complete preserved G5 and F4 object sets over loading ASI: DllMain starts
   game-image initialization, not a standalone renderer API. Do not mix Renderer
   layouts from different revisions. Identify any stable host API substitution.
2. Capture actual desktop pixels for toast/menu, verify background transparency,
   and require a deliberately hidden display to fail the same visibility oracle.
   Check normal z-order, focus, input forwarding and minimize/restore.
3. Find the earliest divergent production boundary. Change production only
   after a concrete mechanism is supported; prove focused RED -> GREEN.
4. Review relevant diff and report actual results and limits. Do not claim
   surrogate graphics proves game/Streamline/fullscreen compatibility.

Expected files: isolated research fixture/runner, this plan, findings report;
only causally implicated Overlay/config production files and focused tests if
the defect is established. No camera changes, broad redesign or tag rollback
based only on timing. Preserve F2/F4 changes, Git index and reference ASIs.

Safety: one compilation/graphics process tree at a time; external Job Object
timeout, capped output, owner-thread cleanup, verified process exit; restore
foreground/cursor. Generated files only under ignored build-artifacts. No ETW,
PresentMon, game or full suite during focused investigation.

Acceptance: meaningful negative control, actual desktop toast/menu evidence,
causal fix and focused regression, or explicit technical evidence of the exact
remaining reproduction limitation (never an invented fix).

Status: **bounded desktop investigation and correction complete; live qualification pending.**

## Established mechanism and bounded repair (2026-10-15)

The real desktop fixture reproduced an invisible but input-capturing menu by
raising the foreground surrogate host above the display HWND, without changing
size, DPI, focus, alpha or DComp content. Both preserved G5 and F4 fail this
scenario. Therefore this is a confirmed latent presentation defect, **not proof
of a new F3 tag regression or of the exact unrecorded live z-order**.

Approved task's causal-fix slice: invalidate input and asynchronously reconcile
host WM_WINDOWPOSCHANGED; include actual relative host/display z-order in owner
placement reuse and input readiness. Keep game callbacks nonblocking, avoid
retaining WINDOWPOS pointers and preserve idle scheduling.
No new mutable state, thread, polling loop, DComp target lifecycle or camera API.

Expected production files narrowed to composition_runtime.cpp and
renderer_runtime.cpp/.hpp. RED evidence: G5 20261015-142901-843-off and F4
20261015-142731-671-on under build-artifacts/invisible-overlay-desktop.
GREEN must show independent desktop pixels after host raise, including startup
toast, menu reopen, input return and cleanup, Diagnostics OFF/ON. A live
confirmation remains necessary; do not silently convert this into proof of the
specific user's game-side trigger.

## Completion record — 2026-10-15

The preserved G5 and F4 object sets both reproduced the invisible-but-input-
capturing display after the foreground host was raised. Relative `HWND_TOP`
did not restore visible pixels in the tested restore condition; topmost-band
placement did. This establishes a desktop presentation mechanism shared by
both references, not the exact game-side event and not a causal connection to
the INI documentation-tag edit.

The bounded production correction is implemented in the current presenter and
composition runtime: active display z-order is maintained and verified above
the host, input readiness depends on actual order, and host `WM_WINDOWPOSCHANGED`
queues an asynchronous owner refresh. No game callback waits or retains the
borrowed `WINDOWPOS` pointer. The attempted foreground WinEvent observer was
removed because it did not resolve the restore case.

Focused existing presenter/input state harnesses passed. The canonical
production candidate built successfully with SHA-256
`9ECB5E637BBDAA6D5658AFB76A764592CAB167CD9E0B8264386AD8E59EA84643`.
The final standalone Current run used the improved toast/menu-region oracle,
Diagnostics OFF and the installed INI copied into the isolated host directory;
toast, full menu, z-order, input, negative control, focus, minimize/restore,
and owner cleanup all passed. The desktop host ended with exit 0, no timeout,
verified cleanup and finalizer COMPLETE.

One earlier Fixed OFF run ended unexpectedly during restore without a recorded
terminal reason. Four subsequent OFF runs with the Fixed component set passed;
the final Current run also passed with `reason=display_close`. The event was not
reproduced or explained and remains a documented limitation.

No game was launched by the assistant and no full `test.cmd` was run. The user
subsequently installed and tried the candidate: they report that the menu is
visible, while the welcome toast is only noticed when opening the menu manually.
The live overlay log shows toast creation and Draw/Commit before the first
manual `OVERLAY_VISIBILITY=VISIBLE` transition, but Diagnostics was OFF, so
actual z-order during startup was not captured. This is evidence that toast
generation is not waiting for the manual toggle; it does not prove the toast
was visible on screen. Startup-toast presentation remains an open live issue.

The installed INI and historical G5/F4 references were not modified by this
investigation; the candidate ASI was later installed by the user. The Git index
was preserved. N03–N06 statuses are unchanged.

Controlled restore refinement: even a direct HWND_TOP setter accepted the call
without making the unowned non-activating display appear above the foreground
host; HWND_TOPMOST immediately restored actual pixels. Therefore the bounded
repair changes visible-display policy: use topmost for the active client-sized
Overlay HWND, irrespective of host topmost style. Existing inactive/minimized
hiding remains mandatory. No game HWND style/z-order mutation. An experimental
foreground WinEvent observer did not repair this case and is removed, including
its cleanup handle. Validate focus loss/restore and minimize/restore rather than
assuming accepted SetWindowPos establishes visibility.
