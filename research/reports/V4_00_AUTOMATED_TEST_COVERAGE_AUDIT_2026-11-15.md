# v4.0.0 Automated Test Coverage Audit — 2026-11-15

## Scope and evidence boundary

This audit examines the current production entry points, the registered 59
harness targets / 62 runner invocations, the current runtime evidence, and the
test sources. It adds regression coverage and a narrow baseline identity fix.
A harness PASS proves only its deterministic inputs and exercised production
functions. It does not prove hook installation, engine scheduling, rendered
output, or visual continuity.

The latest 90° → receiver replacement/reload → 130° runtime pass is the
motivating regression. The log showed the old receiver `0x22707d56700` with
baseline `90.3115° / 127.119°`; after receiver replacement to
`0x2279302d9c0`, native `+0x230` was `130.3115°` and Gameplay HorPlus output
was `153.931°`, while the baseline projection remained `RETAIN`,
`eligible=false`, `writerContext=TransitionAmbiguous`, and still referenced the
old receiver. Cinematic ENTER consequently had stale baseline provenance.
This is a defect in baseline identity/invalidation; the exact causal share in
the observed rendered Dialogue transition remains a separate runtime claim.

## Coverage matrix

| Area / production entry point | Existing automated coverage | Material gap / risk | Automatable |
|---|---|---|---|
| Gameplay HorPlus: `EvaluateHorPlus`, `ProcessCameraWriter` | `runtime_evidence_replay_harness`, `camera_writer_transaction_harness`; 16:9/21:9/32:9 and flags are exercised | Numeric sweep mostly checks finiteness; limited independent expected-value oracle and sparse boundary FOVs | Yes. Added independent double-precision oracle sweep for 70/90/100/120/130, 130.3115 and valid-edge inputs at 16:9/21:9/32:9 |
| Cinematic FOV / `GameplayHorPlus` / fallback: `SelectCinematicBaseline`, ENTER transaction | `cinematic_fov_harness`, `cinematic_lifecycle_transaction_harness`, runtime adapter cases | Receiver replacement has direct and integrated coverage; actual engine reload ordering remains runtime-specific | Deterministic receiver replacement, ENTER fallback, and cross-mode writer/provider route are covered |
| Dialogue policies / boundary lifecycle | `dialogue_fov_harness`, `dialogue_boundary_transaction_harness`, writer and lifecycle harnesses | Provider lifecycle with receiver replacement and Cinematic ownership now has an integrated deterministic sequence; engine callback schedule remains runtime-specific | Yes, through deterministic production transactions; visual-only perception remains manual |
| Receiver identity / lifecycle | `camera_writer_transaction_harness`, `cinematic_fov_harness`, `cinematic_lifecycle_transaction_harness` | Receiver replacement is covered as a stale Gameplay-baseline invalidation event in `TransitionAmbiguous`; actual save reload event ordering remains runtime-specific | Direct and integrated receiver regressions pass |
| Gameplay baseline admission / invalidation / recovery: `GameplayBaselineStore::Project`, writer projection | `gameplay_baseline_harness`, `camera_writer_transaction_harness` | Receiver identity is covered for new/reverse/same-FOV/repeated replacement; engine reload ordering remains runtime-specific | Direct store and integrated writer tests verify stale baseline is unusable while Gameplay HorPlus still computes |
| Cinematic ENTER / EXIT and repeated/missing edges | `cinematic_lifecycle_transaction_harness`, `camera_writer_transaction_harness` | Many duplicate/foreign/stale cases covered. Cross-mode receiver replacement and FOV changes at every boundary are not exhaustive | Yes, with bounded deterministic event sequences |
| Post-EXIT provider: armer / setter / writer order | `camera_writer_transaction_harness`, `dialogue_boundary_transaction_harness` | Transaction order and fresh/foreign baseline behavior are integrated; actual engine callback schedule still needs runtime evidence | Yes. The actual engine callback schedule still needs runtime evidence |
| FOV changes in game settings | `dialogue_fov_harness`, `gameplay_baseline_harness`, writer transaction tests | Several policy/config state changes tested, but setting change before/during/after cross-mode sequence is incomplete | Yes, by deterministic settings epochs and production transaction entry points |
| Mod configuration / hotkeys | `config_persistence_harness`, `hotkey_binding_harness`, `overlay_*_harness`, `camera_writer_transaction_harness` | Persistence and mutation surfaces are covered separately; exhaustive cross-mode in-flight combinations are not | Yes for effective settings and pending transitions; actual key delivery/focus behavior needs native/visual environment |
| Aspect ratio transitions | `runtime_evidence_replay_harness`, camera writer and aspect-policy harnesses | Dynamic aspect sweep exists; not every aspect change is combined with receiver replacement and active recovery | Yes, deterministic ratios/events |
| Startup / loading / save reload / shutdown | `worker_lifecycle_harness`, `runtime_native_adapter_harness`, runtime evidence fixtures | Startup/shutdown ownership and adapter entry are covered. Save reload is represented only as modeled receiver replacement, not actual engine load semantics | Partly. State/lifecycle contract is automatable; engine's actual reload callback ordering needs runtime evidence unless a native fixture can faithfully reproduce it |
| Nested / missing / unexpected transitions | lifecycle and transaction harnesses | Individual edge cases exist, but no bounded matrix guarantees all orderings while receiver and FOV also change | Yes, deterministic finite sequence matrix |
| Diagnostics ON/OFF | writer/lifecycle and runtime adapter tests | Several production transaction cases run both modes; not every cross-mode sequence is dual-run | Yes. Diagnostics must remain observational |
| Integrated interaction of all listed mechanisms | `runtime_evidence_replay_harness` and production transaction harnesses are the nearest equivalents | No generated/bounded state-machine coverage across receiver, FOV, aspect, settings, lifecycle and provider samples; isolated PASSes missed the observed composition | Yes for deterministic model/production transaction APIs. Keep seeds and minimize failing sequences if generation is introduced |

## Added tests in this audit

1. `gameplay_baseline_harness`: seed a valid old-receiver 90.3115° Gameplay
   baseline, submit a 130.3115° new-receiver sample under
   `TransitionAmbiguous`, and assert the old baseline is no longer current or
   usable. Production now invalidates freshness when an ineligible primary
   writer sample comes from a different receiver.
2. `camera_writer_transaction_harness`: run the 90.3115° → receiver A to B →
   130.3115° sequence through `ProcessCameraWriter`. It verifies that Gameplay
   HorPlus still outputs 153.931° while the old baseline is retired. The
   integrated path passes.
3. The unit regression validates the expected 32:9 HorPlus result with an
   independent double-precision tangent-space oracle, then verifies that a
   trusted Gameplay sample from the new receiver reestablishes the baseline.
4. `camera_writer_transaction_harness`: establish Cinematic ownership for
   receiver B, then run the real provider tracker through armer → matched setter
   → writer evidence. Mapping rejects receiver A's stale Gameplay baseline,
   then accepts the same generation-bound provider after a trusted receiver B
   baseline is admitted and computes Reduced from B's native value. This
   verifies source ownership, ordering and that old state does not cross
   receiver identity.
5. `runtime_evidence_replay_harness`: independent numeric oracle checks for
   1.01°, 70°, 90°, 100°, 120°, 130°, 130.3115° and 178.99° at 16:9, 21:9 and
   32:9, including the documented fail-closed output when a transformed value
   falls outside the accepted output interval.
6. `dialogue_fov_harness`: independent double-precision Adaptive and Reduced
   target oracles over representative and valid-edge baseline FOVs.
7. `cinematic_fov_harness`: independent double-precision mapping oracle across
   the 70°–130° authored/target values and 16:9, 21:9 and 32:9 aspects.
8. `gameplay_baseline_harness`: adjacent receiver-replacement cases cover the
   reverse 130°→90° direction, replacement with no FOV change, an ambiguous
   FOV change on the same receiver, and repeated receiver replacements. The
   replacement variants must retire the old source; the same-receiver
   ambiguous sample remains excluded under the existing baseline-protection
   contract. All receiver-replacement assertions pass.

The integrated writer test composes `Gameplay receiver A → receiver B ambiguous
sample → Cinematic → provider armer/setter/writer → Dialogue Reduced → recovery
→ Cinematic` through production transactions. It verifies stale/foreign
baseline rejection, fail-closed Dialogue mapping before fresh receiver B
admission, and successful mapping after trusted receiver B admission. This is
deterministic contract coverage, not proof of the game's callback schedule.

## Findings

- **F1 — Confirmed production defect, fixed and regression-covered:** an ineligible
  `TransitionAmbiguous` observation from a different receiver does not
  invalidate the old baseline. `IsUsableGameplayBaseline` checks validity and
  freshness but has no receiver argument, so downstream code can treat that
  old baseline as current. The store now retires freshness on a primary-writer
  receiver identity change; Gameplay HorPlus remains independent of admission.
- **F2 — Remaining provider contract question:** the full armer → setter →
  writer tracker sequence runs after receiver replacement and verifies
  rejection of A's baseline plus successful B mapping after B admission. The
  explicit contract tested here is fail-closed mapping until an admitted
  same-receiver baseline exists. The game's provider callback schedule remains
  a runtime evidence boundary.
- **F3 — Numeric oracle coverage improved:** independent expected values now
  cover representative Gameplay, Dialogue and Cinematic calculations at the
  named FOV/aspect points. Additional clamp boundaries and recovery interpolation
  combinations remain candidates for the integrated sequence batch.
- **F4 — Stateful composition gap:** production transaction harnesses exercise
  many hand-authored event sequences, but there is no bounded seeded sequence
  runner varying receiver, FOV, aspect, lifecycle and config together. A
  deterministic finite sequence matrix is sufficient; a new property-testing
  dependency is not justified.
- **F5 — Neighbor-class regression batch added:** the captured receiver/FOV
  failure is now checked against reverse direction, no-FOV-change replacement,
  same-receiver ambiguous transition, and repeated replacement. This separates
  source identity from numeric coincidence while preserving the existing rule
  that ambiguous same-receiver transition samples invalidate Gameplay baseline
  freshness.

## Cross-subsystem coverage inventory

The existing registry already exercises more than camera transforms. The
following inventory bounds the remaining audit surface without treating
presence of a harness as proof of complete coverage:

| Production area | Existing automated coverage | Boundary / remaining evidence gap |
|---|---|---|
| Native hook resolution, memory and instruction checks | `signature_scanner_harness`, `instruction_validator_harness`, `memory_harness`, gameplay camera resolver/safety harnesses, native adapter cases | Synthetic images and adapters verify our guards; they do not establish hook behavior in the current game image. Failure/partial-install order should be checked at the owning integration boundary where a deterministic seam exists. |
| Runtime startup, worker lifetime, feature availability | `worker_lifecycle_harness`, `feature_status_harness`, `optional_overlay_startup_harness`, `startup_journal_harness`, startup timeline | Lifecycle state and owned-worker cleanup are deterministic; actual injected-DLL loader timing and game callback schedule remain native runtime evidence. |
| Settings, INI migration and hotkeys | `config_persistence_harness`, `runtime_settings_api_harness`, `camera_setting_transaction_harness`, `gameplay_mode_harness`, `hotkey_binding_harness` | Persistence failure paths and effective-setting ownership have automated coverage; hardlink fixture is host-sensitive. Actual keyboard focus/input delivery remains OS/game integration. |
| Diagnostics and runtime evidence | `diagnostics_gate_harness`, camera/dialogue provenance, writer identity, lifecycle context, performance telemetry, recorded evidence replay | Deterministic emission gates and selected evidence replay are covered; diagnostics-on/off equivalence still needs to be retained for each new production transaction path, and logs cannot prove uninstrumented writers. |
| Overlay, presentation and localization | overlay integration/input/layout/presenter/semantic snapshot/localization harnesses plus embedded-resource audit | State, geometry and catalog contracts are automated; D3D/DComp lifetime, in-game focus and visual readability remain graphics/runtime evidence. |
| Zoom and camera compatibility observers | `n06_m01_zoom_game_derived_harness`, `zoom_transition_harness`, runtime native adapter cases | Derived sample logic and selected adapters are automated; actual game-thread timing and observer availability require runtime confirmation for changed binaries. |
| Test infrastructure and release build surfaces | registry audit, process-tree runner exit contract, aggregate runner, `test.cmd` build/runner orchestration | Full canonical runner continues after independent failures, reports aggregate totals, and returns nonzero when any invocation fails. |

This inventory is an audit map, not a claim that every enumerated production
branch has been exhaustively tested. The next test batches should be selected
from concrete uncovered contracts and runtime-log evidence, not by duplicating
already exercised unit cases.

The production repair is narrow and does not change FOV calculation or
Gameplay HorPlus selection: it retires baseline freshness when a primary
writer observation identifies a different receiver. Tests enforce the safety
contract (old baseline unusable) without requiring stale historical
provenance fields to be overwritten on each later ambiguous receiver sample.

## Manual-only / engine-dependent evidence

- Whether a specific camera transition looks continuous to the player: it is
  perceptual/rendered behavior and requires a game capture or operator visual
  observation; numerical writer tests cannot establish it.
- Actual game hook installation, native callback ordering, engine reload
  semantics and driver/render pipeline behavior: mocks can verify our callback
  contracts but cannot establish that the supported game binary invokes those
  points as modeled. Keep one bounded runtime integration check for changed
  hooks or unresolved engine ordering.
- Overlay readability, focus/input interaction, and observed on-screen FOV:
  requires the real overlay/game window unless an equivalent native graphics
  fixture is available and validated.

Most FOV arithmetic, policy selection, receiver identity, admission, recovery,
provider ordering, config epochs, diagnostics observational equivalence and
bounded event-state behavior are suitable for automated tests and should not
be sent to the user as routine manual test steps.

## Validation status

- Registry audit: PASS (`59` targets, `62` invocations; no missing inputs).
- Build: PASS for all test targets, including the production baseline change.
- Independent numeric Gameplay/Cinematic/Dialogue oracle checks: PASS.
- `test.cmd`: PASS; canonical runner completed `62/62` invocations with
  `passed=62 failed=0`. The first full run exposed one over-constrained
  repeated-replacement assertion. It was corrected to check the intended safety
  contract (stale baseline remains unusable), and the full suite then passed.
- Receiver replacement matrix: PASS for 90°→130°, reverse 130°→90°,
  same-FOV replacement, repeated replacement, and same-receiver ambiguous
  sample exclusion.
- End-to-end production transaction sequence: PASS through receiver A→B,
  Gameplay HorPlus, Cinematic fallback/ownership, Dialogue provider armer →
  setter → writer mapping, recovery, and second Cinematic writer. Dialogue
  mapping fails closed without a usable receiver B baseline and succeeds after
  trusted receiver B Gameplay admission.
- Configuration persistence: all deterministic checks PASS; Windows denied
  hardlink fixture creation with `error=5`, so that one hardlink-specific
  subcase was explicitly SKIP and is not counted as exercised coverage.
- Process cleanup: every contained build/test step reported cleanup verified;
  post-run checks found no game, test harness, compiler, CMake, Ninja, vctip,
  cmd or PowerShell child process left by the run.
- Production behavior changed only at baseline freshness on receiver change;
  FOV formulas, Gameplay HorPlus output, and mode priority were not modified.
- Pre-existing staged changes and the prior `test.cmd` pause/result-summary
  change are preserved.

## Automated coverage batch follow-up — 2026-11-15

### Added coverage

- `camera_writer_transaction_harness` now runs a bounded 48-transaction sequence
  with fixed seed `0x4F9A31C7`, varying receiver identity, native FOV, aspect,
  Gameplay mode, writer context, and enabled/pass-through state through
  `ProcessCameraWriter`. An independent double-precision HorPlus oracle checks
  outputs; failures report seed and step.
- The sequence runs with Diagnostics OFF and ON and compares output, mutation
  decision, context, and baseline source/freshness. All 48 paired outcomes match.
- The receiver-replacement provider route covers 3 aspects × 2 Gameplay modes
  × 3 Dialogue policies. At native 16:9 the route correctly fails closed when
  no HorPlus transform is available. At 21:9 and 32:9 it checks transformed
  targets. The integrated route changes aspect before Dialogue recovery,
  completes recovery, then runs a second Cinematic writer for receiver B.
- The actual runner contract now tests a synthetic named skipped subcase,
  checking its visible `SKIP` line and aggregate `skipped_subcases` count.

### Validation result

- Targeted `camera_writer_transaction_harness`: PASS.
- Full `test.cmd`: PASS; `runner_invocations=62 passed=62 failed=0
  skipped_subcases=1`.
- The one skipped subcase is the hardlink-ownership fixture, explicitly shown
  as `SKIP ... unavailable_error=5`; the synthetic skip-reporting contract
  also passes. SKIP is not represented as tested coverage.
- Process-tree supervisor reported cleanup verified. Post-run checks found no
  running game or test/build process left by this run.
- This batch changed test and runner-contract sources only; no production FOV
  or lifecycle behavior was modified.

### Remaining boundary

The tests establish deterministic transaction and policy behavior. They do not
prove the game's callback schedule or rendered-view continuity. A short runtime
check remains appropriate after the next behavior-changing production patch;
this test-only batch does not require another game run.
