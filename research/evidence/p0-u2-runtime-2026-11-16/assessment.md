# P0/U2 Runtime Capture Assessment — 2026-11-16

## Capture identity and configuration

- Source log: `STALKER2CameraTweaks.log`, SHA-256 `15D9320252EE44B3E43E31A62EAB47B902F13C769C5C0BF259958EB804DE3EB2`.
- Mod image reported by the log: SHA-256 `922139116B5A40B86DA6BD34BCB46F5756D001D63B8E7F5521FD8E45C0BA77B4`.
- Game image reported by the log: SHA-256 `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Gameplay: enabled, `RuntimeCombine`; Cinematic: enabled, `Auto`, `GameplayHorPlus`; Dialogue: enabled, `Reduced`; diagnostics enabled.

## Confirmed observations

- During loaded-save Gameplay the native writer input was `130.311°`; `camera+0x234` remained `90°`. RuntimeCombine produced `153.931°` at 32:9.
- Cinematic 1 selected `GameplayNativeNumericBaseline` and targeted `153.931°`. Its writer samples retained that target.
- Cinematic 1 recovery completed at `00:46:29.596` with post-cinematic exclusion still active. At `00:46:29.717`, the provider producer callback with start `130.311°`, target `70°`, the same receiver and generation was rejected as unauthorized. The diagnostic `owner_valid=0` reports `HasRecoveryAuthority()==false`; source code confirms `MarkComplete` retains the owner record and marks `recoveryComplete=true`.
- At Cinematic 2 ENTER, the stored baseline values remained `130.311° / 153.931°` for the same receiver, but were marked unusable and had no retention witness. `GameplayHorPlus` therefore used the authored `90°` reference and targeted `126.87°`.
- The baseline became unusable because the same-receiver provider/recovery samples were classified as generic ambiguity. Baseline projection retains the numeric record but sets its `current` flag false. This was distinct from a receiver replacement.
- A later normal Dialogue lifecycle was accepted around `00:47:10`, but ran for only about three seconds before a runtime reset; its native FOV did not reach `70°`. It is not treated as a completed active-Dialogue sample.
- The post-helper branch remained inactive for all 3,695 transactions. All 3,695 store/readback and post-process correlations matched; identity mismatches, unavailable readbacks, readback mismatches and queue drops were zero. The log ends on periodic summaries, so no final worker shutdown summary is available.

## Bounded production correction

- Provider armer, setter and writer authorization now accept the completed recovery owner's receiver/generation metadata only while the post-cinematic exclusion remains active and the current receiver/generation match. This does not restore recovery authority. Other lifecycle states, stale identities and inactive exclusion fail closed.
- Added `PostCinematicRecoveryExcluded` writer context. Such samples cannot update Gameplay baseline, but may retain the last trusted baseline for the same receiver. A matched `ProviderTransitionObserved` sample receives the same retention treatment. Unclassified ambiguity and receiver replacement continue to invalidate baseline currency.
- This addresses the two independent causes evidenced by the capture: rejection of the true Dialogue provider handoff and loss of the prior Gameplay reference before Cinematic 2.
- No FOV formula or `+0x234` reference policy was changed. The capture reinforces that `+0x234` did not follow the tested in-game 130 setting and does not authorize its use as the reference.

## Offline validation

- Focused Gameplay baseline, camera writer transaction and Dialogue boundary harnesses: PASS.
- Full `test.cmd -NonGraphics`: 62/62 runner invocations PASS, 0 FAIL. One hardlink subcase remains SKIP (`error=5`); WARP graphics case omitted.
- Production Release build succeeded. Ordinary ASI candidate SHA-256: `1BB6934CB1EE40802113AF7EDEBFCD4EA0DDD9270FF6B6EDF78C909F4B9F42F5`. Candidate is in `build-artifacts/release-candidate/`; not installed in the game.
- Test/build process trees were supervised and exited; cleanup was verified. No game process was launched or terminated.

## Evidence limits and next check

This capture establishes logged writer outputs and the internal control-flow causes, not rendered-view continuity. The fixes are offline-validated only. A later runtime pass with the ordinary ASI and the existing INI diagnostics switch is still needed to confirm that Dialogue Reduced is admitted between the two cinematics, the same Gameplay baseline survives that handoff, and Cinematic 2 targets the expected value. The `+0x234` authority gate remains open.
