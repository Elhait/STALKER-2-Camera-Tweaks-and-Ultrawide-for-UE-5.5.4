# v1.0.0 Batch 4 — Production Regression Validation Task Plan

## Objective

Validate that the v1.0.0 production refactor and Batch 3 safety changes
preserve the frozen v0.6.0 observable behavior on Steam 2.0.5, while validating
only the intentional new safety semantics. This phase does not search for new
features, mechanisms or architecture.

## Established evidence and current state

- Batch 2 moved production responsibilities into explicit plugin, config,
  hooks, gameplay, cinematics, dialogue and Win32 ownership boundaries.
- Batch 3.1–3.5 completed lifecycle, initialization, failed-write,
  persistence and hook-rollback classification/implementation.
- Production build and focused harnesses currently pass.
- Existing v0.6.0 runtime evidence covers gameplay, cinematics, dialogue,
  Auto aspect switching and post-cinematic handoff on Steam 2.0.5.
- The current production output is `STALKER2CameraTweaks.asi`.

## Approved scope

- Read-only preflight of the built artifact and startup log identity.
- In-game regression validation on Steam 2.0.5 only.
- Frozen Gameplay, Cinematics, Dialogue, configuration and resolver contracts.
- Intentional Batch 3 lifecycle, graceful-degradation and persistence-failure
  semantics.
- Evidence capture sufficient to classify each matrix row as PASS, REGRESSION,
  BLOCKED or NOT APPLICABLE.

## Explicit non-goals

- No new features or reverse-engineering probes.
- No Weapon Viewmodel FOV, subtitle-centering or deferred research work.
- No production code changes during a regression run.
- No synthetic failure injection inside game hooks without contradictory
  evidence.
- No compatibility claim for Steam builds other than 2.0.5.
- No README, version metadata or release packaging update before final PASS.

## Regression matrix

| ID | Area / scenario | Expected observable behavior | Previous contract protected | Evidence required | Method |
| --- | --- | --- | --- | --- | --- |
| G1 | Normal startup with Gameplay enabled | Gameplay resolver uniquely validates and hook installs; runtime reaches normal state | Guarded gameplay resolver and startup behavior | Startup log identity, resolver validation and status | Static/log + in-game |
| G2 | Normal ultrawide gameplay | Gameplay aspect correction applies atomically and selected player FOV is preserved | v0.6.0 gameplay correction | Before/after visual result and log samples; no staged oscillation | In-game |
| G3 | Camera rebuild / death / load | Gameplay correction re-arms after rebuild without stale or duplicate state | Existing re-arm lifecycle | Re-entry visual result and startup/runtime log | In-game |
| G4 | Post-cinematic recovery | First confirmed descending FOV sample performs one atomic gameplay handoff; native recovery continues | `RecoveryStart` handoff and no old `0x5` replay | ENTER/EXIT visual sequence, one handoff marker, no extra flick | In-game |
| C1 | Cinematic ENTER with `Auto` | Current runtime aspect is applied and authored FOV receives Hor+ transform | Auto cinematic framing/FOV | Aspect/FOV log and visual framing | In-game |
| C2 | Cinematic EXIT | Native EXIT transition remains game-owned after mod boundary; no extra projection/aspect flick | Native recovery preservation | EXIT log/visual sequence | In-game |
| C3 | Forced `Native` | Cinematic hooks are bypassed and native cinematic behavior remains | Native policy | Initialization status and visual result | In-game |
| C4 | Forced `16:9`, `21:9`, `32:9` | Requested framing policy is applied independently of display aspect; forced 32:9 on 16:9 produces bars | Forced framing policies | Policy log and screenshots/video | In-game |
| C5 | Auto runtime aspect changes | Resolution/aspect changes during one session are reflected by subsequent cinematics without restart | Dynamic Auto contract | Ordered aspect log and matching cinematic results | In-game |
| D1 | Dialogue `Native` | Native dialogue stream passes through | Dialogue Native policy | Policy/status log and visual result | In-game |
| D2 | Dialogue `Adaptive` | Optical zoom strength is preserved relative to captured gameplay baseline | Adaptive projection contract | Endpoint and recovery observations | In-game |
| D3 | Dialogue `Reduced` | Half-strength Adaptive behavior is applied | Reduced policy | Endpoint and recovery observations | In-game |
| D4 | Dialogue `Disabled` | Captured gameplay baseline is held through dialogue | Disabled policy | Baseline and dialogue observations | In-game |
| D5 | Dialogue transitions/cycles | ENTER/EXIT and sequential dialogues recover without stale state; cinematic isolation remains intact | Dialogue state lifecycle | Two sequential cycles and cinematic-to-dialogue sequence | In-game |
| X1 | Configuration load/defaults | Missing/default configuration produces established defaults; explicit values load correctly | Config model and defaults | INI plus startup status log | Static/log + in-game |
| X2 | Successful hotkey persistence | New policy becomes active for the session and complete new value is persisted | Existing F9/F10 behavior and Batch 3.4 success path | File contents plus runtime result | Harness + in-game if enabled |
| X3 | Optional hotkeys | Hotkeys remain disabled by default; when enabled they affect the next corresponding lifecycle only | Hotkey contract | Status log and next-lifecycle result | In-game, only if needed |
| S1 | Normal production resolver set | Requested production hooks resolve/install on Steam 2.0.5 with no unexpected FAILED status | Resolver safety and startup contract | ASI/game identity, status summary, resolver logs | Static/log + in-game |
| S2 | Normal running state | No new fatal initialization or unexpected feature degradation appears | Batch 3 graceful degradation contract | Startup status and gameplay session | In-game |
| S3 | Existing focused harnesses | Lifecycle, feature-status and config persistence contracts remain green | Batch 3 safety contracts | Harness output | Harness |

## Execution order

1. Build/preflight: verify `build.cmd`, output identity and clean relevant
   runtime configuration. Do not change source or release metadata.
2. Run focused harnesses and capture their output.
3. Start Steam 2.0.5 with only the production ASI and intended INI.
4. Use the following four-to-six integrated scenarios rather than one game
   session per matrix row:

   - **R1 — Normal startup and gameplay:** one normal launch covers G1, S1,
     S2 and X1; continue with ordinary gameplay, then one death or save reload
     to cover G2 and G3.
   - **R2 — Auto cinematic and recovery:** one cinematic sequence covers C1,
     C2 and G4, including the first descending-FOV handoff and no extra flick.
   - **R3 — Forced framing series:** on one available cinematic, cycle through
     `Native`, forced `16:9`, `21:9` and `32:9`; cover C3 and C4 without
     repeating startup or gameplay scenarios.
   - **R4 — Dialogue policy series:** use one repeatable dialogue and validate
     `Native`, `Adaptive`, `Reduced` and `Disabled`, including sequential
     transitions and cinematic-to-dialogue isolation; cover D1–D5.
   - **R5 — Auto hot-switch/config scenario, only if needed:** change runtime
     resolution/aspect without restart and validate subsequent `Auto`
     cinematics; cover C5 and X3 when the existing evidence is insufficient.
   - **R6 — Targeted persistence/hotkey scenario, only if needed:** enable
     hotkeys and verify one F9/F10 selection plus successful persistence; cover
     X2/X3 when harness and existing evidence do not already suffice.

5. Validate configuration/hotkey rows only where required by the frozen
   contract; do not repeat already sufficient research experiments.
6. Record each row as PASS, REGRESSION, BLOCKED or NOT APPLICABLE with the
   exact save, aspect, policy and log evidence.

The target is four core scenarios (`R1`–`R4`), with `R5`/`R6` added only when
their specific matrix rows cannot be closed from the core runs, harnesses and
existing validated evidence. This is an acceptance suite, not an exploratory
research campaign.

## Validation requirements

- A build proves compilation/linking only; it is not an in-game PASS.
- Every in-game PASS must identify Steam 2.0.5 and the loaded ASI identity.
- Visual claims require a reproducible scenario and screenshot/video or
  corroborating runtime log evidence.
- Resolver claims require startup identity plus successful structural/runtime
  installation evidence.
- A failure is recorded as expected-vs-actual evidence before any fix is
  considered. Stop the affected branch; do not modify production during Batch
  4 validation.

## Risks and safe-failure behavior

- If the loaded executable or ASI identity is not the intended Steam 2.0.5
  target, stop without interpreting results.
- If a row is blocked by unavailable save state or an external game failure,
  record it as BLOCKED rather than inferring PASS.
- If a regression appears, preserve logs/screenshots and stop the relevant
  validation branch before changing code.
- Do not treat known vanilla weapon/viewmodel FOV or subtitle issues as mod
  regressions without contradictory evidence against the baseline.

## Stop conditions and phase gates

- Stop before in-game execution if preflight identity/build checks fail.
- Stop the affected matrix branch on any regression or contradictory evidence.
- Do not enter release/documentation finalization until all required rows pass
  or have an explicitly documented, non-mod blocker.
- After the matrix is complete, perform a read-only Git review against this
  plan before declaring Batch 4 complete.

## Expected final review

- Compare actual changed paths with this plan; Batch 4 validation should not
  modify production source.
- Summarize PASS, REGRESSION, BLOCKED and NOT APPLICABLE rows.
- Record the exact runtime identity, ASI identity, configuration and evidence
  locations.
- Only after a final PASS may the v1.0.0 documentation and release metadata
  batch begin.
