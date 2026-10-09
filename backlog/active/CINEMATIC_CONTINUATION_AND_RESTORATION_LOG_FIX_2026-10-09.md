# Authorized release-blocker fixes — 2026-10-09

Scope approved by the user: two independent, bounded changes. Existing staged
and unstaged work must be preserved; no unrelated changes or commits.

1. L1: respect restoration `changed` in the production logging adapter. Keep
   observations, sequences, restoration effects and lifecycle/error logs intact.
   Exercise repeated samples and genuine changes through the actual adapter.
2. C1: accept the confirmed GameplayHorPlus baseline at rapid Cinematic reentry,
   retaining the existing interpolation interval only for validated generation,
   source and writer association. Preserve authored-input behavior, global FOV
   tolerances, ownership, EXIT/recovery, Dialogue and Gameplay algorithms.
   Exercise the reported 90 / 95.1311 / 123.105 trajectory and negative cases.

Validation: actual production-adapter harness, focused transaction regressions,
full test.cmd, production and diagnostic builds, with raw output and exit codes.
No game launch, ASI installation, logger rewrite or runtime decomposition.
Runtime replay is a separate acceptance gate. The cause of 0 FPS remains open;
removing duplicate logs is not proof of resolving the stall.

Baseline evidence: research/reports/CINEMATIC_SKIP_PERFORMANCE_AND_FOV_AUDIT_2026-10-09.md.
Implementation and offline validation are recorded in
research/reports/CINEMATIC_CONTINUATION_AND_RESTORATION_LOG_FIX_2026-10-09.md.

Status:

- L1 implemented: unchanged samples do not emit restoration logs; observations,
  sequences and RuntimeCombine synchronization still run.
- C1 implemented: confirmed Gameplay-baseline reentry with unchanged Cinematic
  selection; every retained writer sample requires validated generation/source/RBX
  association. Authored-input continuation remains unchanged.
- Final actual adapters: 64 cases × Diagnostic Mode OFF/ON × two build profiles
  = 256 fixture-process PASS. Other 57 registered harnesses PASS independently.
- Full test.cmd exits 1 at glyph coverage (31 missing text glyphs). No full-suite
  PASS is claimed and fonts/catalogs were not edited in this task.
- Production and diagnostic build.cmd both exit 0. Neither ASI was installed.
- Runtime acceptance and the measured cause of 0 FPS remain open. A separate
  aspect-policy selection concern is documented in the report, not fixed here.
- B2–B7 and logger/lifecycle architectural changes remain out of scope.
