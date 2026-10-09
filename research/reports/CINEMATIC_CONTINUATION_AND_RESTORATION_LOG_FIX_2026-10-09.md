# Restoration log deduplication and Cinematic Gameplay-baseline continuation

Date: 2026-10-09. Two independently scoped release-blocker fixes.
Implementation is not a claim that the reported 0 FPS stall is resolved.

## Evidence and scope

The preceding read-only audit is
[CINEMATIC_SKIP_PERFORMANCE_AND_FOV_AUDIT_2026-10-09.md](CINEMATIC_SKIP_PERFORMANCE_AND_FOV_AUDIT_2026-10-09.md).
It identified 2,583 repeated restoration records among 3,024 log records and
the generation-2 output departure from 123.105 to 95.2186 while the coordinator
remained CinematicActive. Its runtime log SHA-256 was
`4DDC969AD30910D9698FC46BA947A17821EB41F6A028407877F0D356B8F360B4`;
the tested installed ASI identity was
`34816FF486AF670791CB9A4255958821610142FC527868791CF8606136F95A3E`.

Only three production files and the actual-adapter harness were edited in
this batch. Existing staged/unstaged work was preserved. No Git commits were
created: the pre-existing index contains the broader unreleased implementation,
including files added before this task. The two changes remain separately
described below rather than committing that unrelated work.

## L1 — restoration logging adapter

File: `src/plugin/runtime.cpp`, `ReplayManualTransition`.

```diff
-            if (applyResult.aspectRestorationObserved)
+            if (applyResult.aspectRestorationObserved && applyResult.aspectRestoration.changed)
```

The producer already computes `changed` without treating an observation-sequence
increment as a semantic change. The adapter now respects that result. This does
not skip the callback, restoration Update/Retain operations, sequences, aspect
writes, publication or wake. `Log`, logger sinks, flushes, deferred-log execution,
restoration producers, lifecycle logs and errors are unchanged.

The new `restoration-logging` fixture counts actual `RESTORATION_STATE` payloads
from the production logger sink. With Diagnostic Mode ON, the first change emits
one record; 64 unchanged writer samples emit no additional restoration records,
while the observation sequence advances by 64. A real RuntimeCombine viewport
change emits the second record and updates the source aspect; another unchanged
sample does not emit. Diagnostic Mode OFF emits zero restoration records.
The pre-fix adapter failed this test on duplicate logs (`restoration-red.log`).

## C1 — validated rapid reentry from Gameplay native baseline

Files:

- `src/plugin/cinematic_lifecycle_transaction.hpp`, `ProcessAcceptedCinematicEnter`.
- `src/plugin/camera_writer_transaction.hpp`, `ProcessCameraWriter`.

The existing authored-input continuation predicate is evaluated unchanged.
An additional, strictly bounded Gameplay-baseline route is allowed only when:

1. The qualified baseline was actually selected for GameplayHorPlus and carries
   a nonzero confirmed source.
2. The prior EXIT native operand matches that baseline within the existing
   strict 0.05-degree baseline tolerance.
3. The captured aspect policy and FOV mode are unchanged across generations.
4. The existing continuation predicate confirms prior CinematicExiting ownership,
   pending validated recovery/empty-generation carry, equal source and a matching
   transformed target.

The comparison uses a temporary reentry sample; authored ENTER state and
published evidence are not replaced with the Gameplay baseline.

For this new route, every writer retention additionally requires a valid active
generation, matching selection/association generation, enabled captured settings,
non-conflicting aspect-store association, matching RSI and RBX, and the same
confirmed Gameplay-baseline source/start FOV. A failed check passes a zero
continuation source to the existing resolver, preventing continuation retention.
The initial-establishment authorization and existing interpolation interval are
not widened. In particular, 95.2186 is not reclassified as the 95.1311 baseline;
it is retained as interpolation only after validated reentry.

No new owner, gate, TLS, shared state field, enum or lifetime was introduced.
Authored-input behavior, camera math, EXIT/recovery implementation, Gameplay,
Dialogue, hook installation, Overlay, logger architecture and decomposition
batches are unchanged.

## Actual-adapter matrix

The harness includes real `runtime.cpp`, `SafetyHookContext`, resident state,
correction gate, callbacks and publication. Its existing external memory/viewport/
wake/sink fixtures remain controlled; it does not test installed hooks or rendering
inside the game process.

New cases cover:

| Case | Required outcome |
| --- | --- |
| restoration-logging | Changed-only restoration logs; all observations/effects preserved |
| continuation-baseline / combine | Authored 90, qualified native 95.1311, target about 123.10542; generation-2 95.2186 writer outputs the target |
| continuation-chain | Gen1 EXIT → gen2 → gen3 without a writer → gen4 retains validated interpolation |
| foreign-source / foreign-writer | No target write to unassociated RSI/RBX |
| stale-generation / no-association | No continuation retention |
| invalid-baseline / native-mode | No alternative Gameplay-baseline continuation |
| target-change | Different Auto viewport/target rejects continuity |
| policy-change | Actual settings API change to Forced16x9 rejects the new continuation route |
| outside-low / outside-high | Incoming FOV outside the existing continuation interval passes through |
| correction-off | No correction |
| exit-departure | Prior native operand beyond 0.05-degree baseline tolerance rejects the alternative route |
| no-validated-exit | Missing EXIT association does not authorize continuation |

Positive cases also exercise Disabled Dialogue during CinematicActive, unchanged
ownership, register/publication consistency and final EXIT recovery. Native
Gameplay recovery is checked separately from resumed Gameplay HorPlus output;
the latter can numerically equal the old Cinematic target without retaining
Cinematic ownership.

## Validation

| Check | Result / exit code |
| --- | --- |
| Final production-profile actual adapter | PASS, 64 cases × Diagnostic Mode OFF/ON = 128 fresh processes; exit 0 |
| Final diagnostic-profile actual adapter | PASS, 64 cases × Diagnostic Mode OFF/ON = 128 fresh processes; exit 0 |
| Camera writer / Cinematic lifecycle / Cinematic FOV focused harnesses | PASS on final source, each exit 0 |
| Other registered harnesses independently executed | 57 PASS on final compiled source, each exit 0; not a replacement for full test.cmd |
| Final full test.cmd | FAIL, exit 1 at localization_harness; 31 missing text glyphs |
| 18-locale/resource audit | PASS in test.cmd |
| Production build.cmd | PASS, exit 0 |
| Diagnostic build.cmd | PASS, exit 0 |
| Game launch / installation / new runtime FPS trace | Not performed |

Raw logs are under `build-artifacts/tests/`:

- `release-fixes-adapter-production-final64.log` — 128 production-profile fixture processes.
- `release-fixes-adapter-diagnostic-final64.log` — 128 diagnostic-profile fixture processes.
- `release-fixes-supplemental-harnesses-final.log` — all other 57 registered harnesses.
- `release-fixes-test-final.log` — full runner, stopped at glyph coverage (exit 1).
- `release-fixes-build-production.log` / `release-fixes-build-diagnostic.log` — builds.

These 256 actual-adapter processes include the 17 new cases in this batch; they
are not runtime/visual validation. Production SHA-256 is
`B89780E9EDF33E4D74DD9A9FA7E148593D2B1F5BA08941B06B68721B2AA252AF`.
Diagnostic SHA-256 is
`4665CEC1FABD4B5E3BADD198DD24D224816BA7769C6F4301E74203F86F9FEFA1`.
Both artifacts were built in the repository root, not installed in the game.

Final source identities (SHA-256):

| File | Hash |
| --- | --- |
| src/plugin/runtime.cpp | B8DD557330AD807D42D9ACE2556A8C6D6315CC5336AC40E45B2AD86BF52F4ACB |
| src/plugin/camera_writer_transaction.hpp | 0AA9B4925005E962CD178A133CD82B0BE4BF3377DAFCCF2E2766751448D5C6E2 |
| src/plugin/cinematic_lifecycle_transaction.hpp | 9FE3E399C6C1A90A6AD53093E81AA27E6C6BCDEB44E640AAA188AECA4082E9EB |
| tests/plugin/runtime_native_adapter_harness.cpp | 005726565E1C83BC18C664F2A4197C5F91A47CFFA70DA6883D071C825AFD66DE |

Earlier focused attempts included an incomplete recovery-observation capability
fixture, a fixture without the runner's TEMP/TMP setup and an incorrect expectation that
recovery should not resume Gameplay HorPlus in the completing callback; these
fixture expectations were corrected without changing recovery production code.

The initial full suite exited 1 in `localization_harness`, reporting 31 unique
missing text glyphs (including U+0141, U+064E and CJK characters). Font assets and
locale catalogs were not changed in this task. No full-suite PASS is claimed.

## Separate observation and remaining gates

The policy-change negative case exposed a separate aspect-selection concern:
`TraceCinematicEnter` resolves the effective aspect through the previously active
policy before `ProcessAcceptedCinematicEnter` publishes the new selection.
This batch does not repair that path. The new route now explicitly refuses to
inherit continuation across changed Cinematic selections. The correctness of the
first target after a policy change needs its own bounded validation/fix; the
target-change test here uses Auto viewport change and does not prove that case.

Required runtime replay: the same first cutscene and internal Dialogue/rapid
EXIT/ENTER sequence, 27:9, native Gameplay FOV around 95.1311, GameplayHorPlus,
including Disabled Dialogue and skip. Confirm the target does not drop to 95
during the validated continuation and restoration duplicates no longer flood
the log. Confirm final EXIT/recovery separately.

The cause of the multi-second 0 FPS stall remains unmeasured. If it persists,
collect an external thread/File I/O trace; no logger or lifecycle rewrite was
performed or justified by these fixes. No release-readiness or in-game PASS is
claimed. B2–B7 remain frozen.
