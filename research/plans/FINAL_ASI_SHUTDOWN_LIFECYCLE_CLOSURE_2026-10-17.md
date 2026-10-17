# Native ASI shutdown audit and closure

The initial audit below is superseded by the user-authorized
`FINAL_ASI_SHUTDOWN_SAFETY_HARDENING_2026-10-17.md` plan and section 10 of the
shutdown report. SL01/SL02 now have scoped production fixes and offline PASS;
the safe canonical portion was run with WARP explicitly omitted. There is no
pending WARP permission request for the current hardening scope. Manual exit
qualification of candidate `97696DBA...2016F5D4` is the remaining user step.

User-authorized scope: the FINAL shutdown task supplied on 2026-10-17.

## Bounded execution plan

1. Trace DLL/CRT detach, resident Runtime ownership, worker/callback reset,
   Overlay owner finalization, graphics release, subclass continuation and logs.
2. Extend existing deterministic harnesses only where the actual detach and
   empty-renderer shutdown contracts lack assertions. Change production behavior
   only for a demonstrated defect. Existing owner-finalizer markers are reused.
3. Run focused checks and the canonical suite with finite external process-tree
   supervision and verified cleanup. The canonical registry contains a software
   WARP graphics case; explicit clarification is pending because the task also
   forbids graphics fixtures. No game/desktop graphics experiment is authorized.
4. Review the task diff and write the requested report, including resource graph,
   scenario guarantees, evidence limits and any unresolved risks. Build Release
   only if production code changes.

Expected files: existing native-adapter lifecycle evidence, architecture lifetime
documentation, this plan and the requested report. Keep unrelated staged and
unstaged changes intact. No ASI installation or user-configuration edits.

Acceptance: truthful distinction between controlled cleanup and process exit;
actual detach notification cannot join/reset/restore or emit file logs; tests
exercise repeated/partial cleanup and admitted callbacks; report does not turn
generic/mock tests or successful process exit into graphics/driver cleanup proof.

## Initial evidence

- The resident Runtime is heap allocated without a static destructor.
- Production `plugin::Shutdown()` has no caller; initialization failure calls
  Runtime reset, while Overlay terminal requests have their own owner finalizer.
- Process-termination detach returns immediately. Explicit unload is unsupported.
- The existing Overlay log is a global `std::ofstream`; its MSVC filebuf
  destructor calls fclose, which takes a CRT stream lock. Static /MT stdio
  termination also flushes/closes FILE streams. Assess this conditional exit
  risk separately from a claimed or observed shutdown failure.

No production change is justified merely by absent final log markers.

## Handoff state

- Lifecycle/source audit and requested report: complete.
- Focused deterministic coverage: 11/11 PASS; current registry audit PASS;
  process-tree cleanup verified; normal git diff --check PASS.
- Production C++/ASI/configuration/rollback: untouched by this task.
- Canonical suite: pending the explicit WARP clarification already presented to
  the user. Do not run the aggregate while that graphics-scope question is open.
- Overall safety closure: UNRESOLVED for SL01/SL02; missing process-exit markers
  are DOCUMENTED LIMITATION. No live failure or comprehensive safety PASS is
  inferred. See the report for source paths, causal conditions and bounded next
  proof. No new broad audit is necessary to resume these items.
