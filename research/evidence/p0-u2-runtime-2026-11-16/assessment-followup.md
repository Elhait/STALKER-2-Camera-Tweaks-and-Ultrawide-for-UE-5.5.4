# P0/U2 Runtime Follow-up Assessment — 2026-11-16

## Capture identity

- Source: `STALKER2CameraTweaks-followup.log`.
- SHA-256: `3563037052B269989718D2F6BAFC1954929402198A1FE8FA9B1400654445459C`.
- Runtime settings: 32:9; Gameplay `RuntimeCombine` at configured FOV 130; Cinematic `Auto` / `GameplayHorPlus`; Dialogue `Reduced`; diagnostics enabled.
- This log is user-provided runtime evidence. It is not a visual recording and does not prove rendered output.

## Observed outcomes

- **Cinematic 1 → Dialogue provider handoff still failed in this capture.** At `01:43:34.099`, the validated provider armer captured generation 1, same receiver, `start=130.311°`, `target=70°`, `continuity_start=130.311°`. The first writer at `01:43:34.105` correctly had no matched setter yet. At `01:43:34.109`, an older Cinematic EXIT setter arrived for that receiver but from a different component, with old start/target/argument `130.311°`. It was rejected as `component_mismatch`. No provider setter match or `ProviderTransitionObserved` writer context followed. The capture therefore does not establish that the intended interstitial Dialogue obtained its Reduced mapping.
- **Cinematic 2 baseline selection improved.** On generation 2 the overlay/writer diagnostics report the retained same-receiver Gameplay baseline `130.311° / 153.931°`, a valid retention witness, and target `153.931°`; it no longer falls back to authored 90° / 126.87°. The initial native Cinematic 2 writer input is `70°`, and the current correction mapping starts at `108.941°` before progressing to `153.931°`. The user reports the Cinematic transitions looked better.
- **A distinct Dialogue was started after Cinematic 2.** Its ordinary Dialogue lifecycle was accepted at `01:44:06.260` with selected policy `Reduced`, native target 70°, and a mutated callback result around `129.996°`. Recovery started at `01:44:08.041` and completed at `01:44:08.290`. This short event is not evidence that the earlier between-Cinematics provider handoff worked, nor that a stable native 70° Dialogue output was reached.
- The final diagnostics summary reports 3,050 writer transactions and 12,200 accepted events, with 3,050 correlated post-store and post-process records, no identity mismatches, unavailable readbacks, readback mismatches, queue drops, contention drops, or pending records. Post-process branch remained inactive in the observed samples.
- `camera+0x230` reached the configured 130.311° Gameplay value while `camera+0x234` remained 90°; this again does not establish `+0x234` as an authoritative reference for the tested setting.

## Root cause and offline repair

The log supplies a concrete ordering failure: a valid provider armer was followed by an unrelated stale Cinematic EXIT setter for the same receiver. Previously any setter mismatch reset the candidate, so a later exact provider setter could no longer match. `PostExitProviderHandoff::ObserveSetterSample` now preserves the candidate across mismatched setter samples while authorization, receiver, generation and settings epoch remain current. Exact activation component/thread/endpoints and callback-to-setter value checks remain mandatory. Stale authorization or lifecycle identity still retires the candidate. Diagnostic rejection reporting is capped to one report per activation.

A regression in `dialogue_boundary_transaction_harness` replays the observed interleaving: stale component/old endpoint setter is rejected without retiring the candidate, then the exact provider setter and writer sample match.

## Validation and artifact

- Full `test.cmd -NonGraphics`: 62/62 runner invocations PASS, 0 FAIL. One hardlink subcase is an explicit SKIP (`error=5`); WARP graphics coverage is omitted by this profile.
- The wrapper reached its configured final keypress wait. A keypress was sent to finish the already completed run; the reported test exit code was 0 and owned process-tree cleanup was verified. No project compiler/test process remained afterward.
- Ordinary production Release ASI built and published to `build-artifacts/release-candidate/STALKER2CameraTweaks.asi`; SHA-256 `6AAB96F0BF8263AEF2BA9C819A00DA98C7E463652859DECFC0FFB2089BB4A659`.
- This is the ordinary single ASI; diagnostics are still controlled by the existing INI setting. It was not installed or exercised in game by the agent.

## Remaining validation

Runtime confirmation is still needed for `Cinematic EXIT → provider Dialogue Reduced → Cinematic ENTER`, specifically that the matching provider setter arrives after the stale EXIT setter, the writer admits the provider trajectory, and the visible Dialogue behavior matches expectation. The later standalone Dialogue in this capture does not substitute for that check. No new in-game commands are required; use the project's regular install flow and the existing INI diagnostics switch.
