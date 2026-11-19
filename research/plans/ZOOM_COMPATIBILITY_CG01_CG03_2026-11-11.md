# CG01–CG03: Zoom compatibility and observation validity

Approved by the project owner on 2026-11-11 following the compatibility audit. Status: implementation, offline validation and isolated Release build complete; manual acceptance pending. See [implementation evidence and checklist](../reports/ZOOM_COMPATIBILITY_CG01_CG03_IMPLEMENTATION_2026-11-11.md).

Contract: executable identity is provenance only. Unique bounded Zoom signatures and decoded native operands govern admission. Both hooks must commit before any callback publishes state. A failed pair stays unavailable; callbacks before commit and after rollback cannot publish. Unknown Zoom observation cannot confirm neutral Gameplay FOV. Diagnostics selection and observer capability have separate status records.

Scope: header-only Zoom resolver, runtime install/callback/reset/publication integration, Camera State publication, focused resolver/adapter/publication tests, current architecture/safety/manifest documentation. Preserve other camera algorithms and prior ASIs. No Git, game, debugger, graphics fixtures or automatic installation. Missing cinematic ENTER remains a separate open investigation.

Batches:
1. Implement bounded resolver and SHA-independent paired hook installation, with inactive admission before commit and during rollback.
2. Propagate observer availability and valid observation into neutral FOV confirmation; separate operational capability logs from Diagnostics selection.
3. Exercise resolver cardinality/operands/bounds and changed image metadata, pair failure/early callbacks, unavailable observation, valid Zoom hold/release and diagnostics parity. Run contained focused CPU checks and relevant non-graphics canonical suite. Build one isolated Release artifact after offline checks; retain existing ASIs.

Validation must cover real production resolver/adapter/publication behavior. Test rollback by invoking callbacks while installation is incomplete and after failure, and assert no state publication. Existing global native callback admission/drain and process-lifetime SafetyHook retention remain the resource lifetime owners. Do not hold camera serialization across patch installation or drain waits.

Risks: signatures establish only their decoded native boundary, not arbitrary engine ownership. Current image matching and in-game behavior require distinct evidence; offline PASS cannot close runtime acceptance. If actual game-image portability cannot be established, record that limit without inventing a supported version. Tests/builds run sequentially under the existing supervised runner and verify child cleanup. Do not run heavy compilation while the user may still be gaming without confirming host availability.

Completion: CG01–CG03 production changes plus focused regression evidence and canonical outcome; SHA-256 of isolated build if produced; remaining manual current-game Zoom and Camera State check explicitly pending. The first missing ENTER is not closed by this task.

## Verified progress

- CG01: header-only resolver validates unique IN/OUT matches, executable `.text` bounds and decoded MOVSS/MOVSS/UCOMISS register/offset contracts. SHA and disk hashing success no longer participate in Zoom admission.
- Paired installation: disabled creation, activation of both hooks, then publication admission. Callback checks availability/stopping before and after acquiring the existing correction gate. Reset closes Zoom admission before native callback drain. No camera lock is held across patch installation. Repeated already-committed initialization is idempotent.
- CG02: Diagnostics selection and ZoomObserver availability have separate startup status; capability failure reasons are logged even with Diagnostics OFF.
- CG03: neutral Camera State GameplayFov requires available and valid inactive Zoom observation. Unknown observation cannot confirm it. Existing reference becomes historical when the required evidence is absent. Before first valid OUT the neutral reference may remain unavailable; OUT without prior reference starts ordinary stable-sample qualification.
- Focused checks: 17/17 PASS, including actual production resolver/install failures at first/second enable, concurrent callbacks before commit, post-rollback callbacks, restored native bytes, changed image metadata, diskless image, invalid/missing/duplicate/truncated/wrong-register contracts and native Zoom continuations with Diagnostics OFF/ON.
- Full `test.cmd -NonGraphics`: 62/62 PASS, native matrix 222 fresh processes with graphics case omitted. Initial restricted runs failed filesystem fixtures; those same binaries passed outside the execution restrictions, followed by a successful full canonical run. No production filesystem changes were made to obtain this result.
- Current EXE `2BD3…5479`: one match per signature in `.text`, hook RVAs `0x531AF1` / `0x531C9E`. Static byte/cardinality evidence only, not in-game hook validation.
- Read-only/no-analysis Ghidra check: one IN/OUT match each on retained 2.0.5 and 2.0.6; zero on 2.0.2–2.0.4. Older variants are outside this correction. The launcher initially timed out and verified cleanup; an explicitly configured local JDK retry completed. An unrelated existing research script produced compiler warnings, while this task's script executed on all five images.
- Canonical runner and static-analysis supervisors verified owned process-tree cleanup. Prior root/installed ASI is preserved. No game/debugger/graphics launch or installation performed.
- Isolated Release: SHA-256 `55DB2E4C4DEB92F2BD2F7C1EE99D2E5336DB51C9F9C0331E3887DD2DB6C7D872`, version `3.2.2.0`. Prior root/installed SHA `A9A38A68E4F69FFB6C0127E4C8A430268C79C60AB7619BCF008E2805B91F9842` unchanged. Manual acceptance and the separate missing-ENTER investigation remain open; no task closure is claimed.
