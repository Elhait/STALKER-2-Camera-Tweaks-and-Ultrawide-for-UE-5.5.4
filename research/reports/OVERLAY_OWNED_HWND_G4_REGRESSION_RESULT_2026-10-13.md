# Overlay-Owned HWND — G4 Regression Result

**Date:** 2026-10-13  
**Scope:** One canonical regression run for the integrated G1–G3 candidate.  
**Game / ETW / PresentMon:** Not run.

## Result

G4 offline regression criteria: **PASS**.

The single canonical `test.cmd` run completed with exit code `0`. Its registry
audit reported 59 registered harness targets matching 59 repository harnesses,
62 ordered runner invocations, 102 native-adapter cases / 204 fresh child
processes, three resource targets and zero missing inputs. Every registered
invocation passed, including all four game-derived Zoom scenarios.

The test build compiled the changed Overlay composition and Renderer translation
units as adapter dependencies. The exact isolated production candidate had
already been built from this source state; its SHA-256 was rechecked after the
suite and remained unchanged.

## G4 evidence

| Criterion | Evidence | Status |
| --- | --- | --- |
| Canonical test inventory | `canonical_test_registry=PASS`; 59/59 targets and repository harnesses; 62 runner invocations; 0 missing inputs | PASS |
| Canonical runner | `runner_invocations=62 exit=0` | PASS |
| Native camera/runtime adapter regressions | 102 cases with Diagnostics OFF/ON = 204 fresh processes; adapter harness PASS | PASS offline |
| Zoom continuation regressions | zoom-in OFF/ON and zoom-out OFF/ON | 4/4 PASS |
| Changed Overlay production sources | Both composition and Renderer TUs compiled in the test build and linked into the production candidate | PASS |
| Production candidate identity | `123D792A61A550892141DBEC12BB059A606BEA6A30B0C8B300491B6CAFC7DEF2`, 3,572,736 bytes; unchanged after `test.cmd` | PASS |
| Additional game hooks | G2/G3 changes add no camera hook or game-memory hook sites; the existing Overlay WinEvent observation and input-window subclass remain the G1 design | PASS by source review |
| Warnings | No warnings appeared in the canonical test build. The preceding production build reported only the existing vendor C4201 warnings in `external/safetyhook/Zydis.h`; no changed Overlay-source warnings. | PASS within build evidence |

## Process containment and record

- One `test.cmd` run; CMake build parallelism was capped at 2.
- External supervisor deadline: 9 minutes. Run completed before the deadline;
  no process-tree termination was needed.
- After completion, no registered harness, CMake, Ninja, compiler, linker or
  resource-compiler process remained.
- Full log: `build-artifacts/g4-regression/test-cmd.log`.
- The candidate remains isolated in `build-artifacts/g2-g3-candidate/`; it was
  not installed or copied into the game directory.

## Limits and next gate

This PASS is offline regression evidence for the tested source and candidate.
It does not establish live camera output or graphics-stack behavior for this
exact ASI hash. It does not inherit the earlier G1 live result. The next gate is
G5 compatibility for the current candidate (NVIDIA HUD/G-SYNC activation order
and the already-selected FG/VRR observations); no new ETW/PresentMon work or
G1–G3 implementation cycle is implied.
