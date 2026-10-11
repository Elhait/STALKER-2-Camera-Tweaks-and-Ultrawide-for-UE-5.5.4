# B6 Gameplay Writer Boundary Review

Date: 2026-10-10  
Scope: read-only B6 boundary review, following approved B5 closure  
Production/vendor changes, tests, builds, game launch, and ASI installation: **not performed**  
Decision status: **Approved closure — Complete with documented Remain decisions (2026-10-10); no B6 implementation**

## Executive finding

The Gameplay writer remains a substantial and central native callback, but its
decision-making core is already split into production domain/transaction
helpers. The remaining adapter in `runtime.cpp` is where native register and
memory observations are associated with the sole `RuntimeState`, serialized
by the existing `CameraCorrectionGate`, coordinated with Dialogue and
Cinematic owners, and committed to publication/diagnostics in the existing
order.

This review found no narrow B6 extraction that would materially reduce
coupling while preserving the one-owner boundary. Moving the adapter would
require exposing a broad mutable state view or adding another owner-operation
interface across B2 settings, B3 publication, B4 Dialogue, and B5 Cinematic
recovery. The user approved classifying the native writer adapter and its
RuntimeState-bound factories as `Remain`; the already extracted transaction,
mode, and recovery policies remain `Already extracted`. B6 is closed with
these dispositions; no production code was changed.

## Scope and source paths examined

- `src/plugin/runtime.cpp`: `ReplayManualTransition`,
  `GetCameraWriterEngineOperations`, Gameplay/Cinematic/recovery state-view
  factories, observation/publication/diagnostic tail, and hook binding.
- `src/plugin/camera_writer_transaction.hpp`: `ProcessCameraWriter`,
  `ProcessGameplayHorPlusWriter`,
  `ProcessGameplayAspectRecalculationWriter`,
  `SynchronizeRuntimeCombineAspect`, recovery and writer-policy helpers.
- Existing Gameplay domain helpers in `src/plugin/gameplay_camera.*`,
  `src/plugin/horplus_gameplay.*`, and `src/plugin/gameplay_state.*`.
- B1–B4 dispositions in the implementation plan and move ledger.
- `NATIVE_RUNTIME_EVIDENCE_AUDIT_2026-10-09.md` and
  `N01_N03_NATIVE_RUNTIME_SAFETY_REMEDIATION_IMPLEMENTATION_2026-10-09.md`.

## Dependency and classification map

| B6 responsibility / production boundary | Dependencies and contract | Classification | Reason / evidence boundary |
|---|---|---|---|
| `ReplayManualTransition` native writer callback | Reads `RSI` source, `RBX` output, low `XMM0` incoming FOV; observes source aspect/flags; obtains viewport when applicable; acquires the non-recursive correction gate; invokes the transaction; writes the selected low `XMM0` result and returns through the existing trampoline path | **Remain** | It is the adapter boundary between the engine's live context, one runtime owner, and the existing gate. A move alone would relocate rather than reduce this coupling. |
| Native writer ABI and displaced store | The original writer stores `XMM0` to `[RBX+0x30]`; callback output and trampoline/continuation must preserve current register and instruction behavior | **Remain** | Actual-adapter fixtures assert callback context/output but do not execute the original game store or prove live engine consumption. Keep this inseparable from the hook site. |
| `GetCameraWriterEngineOperations` | Binds `SafeRead`, checked aspect/flags reads, writable-memory validation, aspect-only writes, and aspect-plus-flags writes to production functions | **Remain** | This is a small owner-local native I/O adapter, not a domain policy. Moving it requires exporting runtime memory operations and their current safety assumptions. |
| Gameplay/Cinematic/recovery state-view factories | Construct transaction views from the one `RuntimeState`, including baseline/restoration state, source association, continuation, recovery ownership, and coordinator fields | **Remain** | The views are references into shared owner state. Exporting factories or their mutable views would enlarge the cross-TU owner API and entangle B5 recovery with B6. |
| `ProcessCameraWriter` routing and lifecycle transaction | Routes coordinator/recovery ownership, foreign-source handling, Cinematic continuation, RuntimeCombine recovery, Dialogue exclusions, and Gameplay mode processing through supplied views/operations | **Already extracted** | The production transaction is in `camera_writer_transaction.hpp`. The remaining caller supplies actual owner-bound views and callback operations; moving that glue would not extract the decision engine. |
| HorPlus calculation and writer policy | `ResolveGameplayWriterPolicy`, HorPlus transform, eligibility and result classification | **Already extracted** | Transaction policy and `gameplay::EvaluateHorPlus` are already production helpers. |
| AspectRecalculation behavior | Validates writer source and applies checked aspect/flag operation under the existing transaction | **Already extracted** | `ProcessGameplayAspectRecalculationWriter` and `gameplay::ResolveCameraWriter` / write helpers contain the reusable policy; production memory adapters remain owner-bound. |
| RuntimeCombine synchronization and HorPlus gating | Current viewport target, source aspect read, conditional aspect write/readback confirmation, telemetry facts, and only then FOV transformation | **Already extracted** (decision/transaction); **Remain** (runtime input and I/O adapter) | `SynchronizeRuntimeCombineAspect` and writer transaction encode the contract. Runtime viewport acquisition and native read/write callbacks are supplied by `runtime.cpp`. |
| Cinematic EXIT recovery and continuation during Gameplay writes | B5 recovery owner, generation/source association, endpoint/interpolation state, Gameplay mode, source identity and coordinator handoff | **Already extracted** (domain rules); **Remain** (cross-owner wiring) | Recovery decisions/helpers exist, but the live writer consumes state prepared by B5 ENTER/EXIT. Moving one side would split a single cross-callback lifecycle and risk source/generation ownership. |
| Source identity, RSI/RBX association, ADS/source churn | Writer observations are qualified by live source/output identity and interact with Cinematic associations, recovery owner and baseline observations | **Already extracted** (domain identity/policy); **Remain** (runtime association and observation) | Existing tests cover selected RSI/RBX/source cases and ADS-like sequences. They do not establish arbitrary live source churn or all engine camera handoffs. |
| Dialogue context invalidation/reset callbacks | Transaction invokes owner-provided reset/is-active operations; production lambdas acquire `dialogueMutex` while the correction gate is held | **Remain** (adapter); **Already extracted** (Dialogue domain logic) | Preserve the established `CameraCorrectionGate → dialogueMutex` ordering and B4 boundary. Relocating the lambdas would require broad owner access or alter synchronization ownership. |
| Camera publication, freshness, Overlay wake, shared semantic snapshot | Publishes writer observations/telemetry and updates freshness/wakes presentation after decisions, under existing gate/mutex ordering | **Remain** (runtime adapters); **Already extracted** (pure snapshot/publication/freshness helpers) | This is the approved B3 disposition. A B6 move must not reopen B3 or create a second publication path. |
| Runtime diagnostics and deferred logging | Emits events and snapshots for source/mode transitions, writer outcome, recovery and continuation; some log work is deferred until after gate release | **Remain** (call sites/order); **Already extracted** (event/domain records where applicable) | Call sites span B3–B6 and depend on current gate-release timing. Moving a callback subset risks changing event order or lock-held logging. |
| Hook resolver and installation | `gameplay::ResolveCameraWriter` is used by owner-bound executable/site validation; production hook construction/enable remains in initialization | **Already extracted** (resolver policy); **Remain / outside B6** (installer orchestration) | Installation and startup are B7 territory and are not authorized for review here. No B7 classification or implementation is proposed. |
| `ReplayManualTransitionOriginal` and deferred replay experiment | Compile-profile-gated legacy/test path, not the production callback used by the normal hook | **Remain** | Preserve the existing profile boundary; this review does not authorize cleanup or movement of experimental/test-only code. |

## Cross-boundary assessment

| Neighboring batch | Concrete B6 dependency | Effect of extracting B6 runtime adapter |
|---|---|---|
| B2 settings | Runtime mode/enabled snapshots and settings transitions affect pending Gameplay application and replay state | Requires either sharing settings/runtime state across TUs or an owner API; no independent settings-only boundary is exposed by B6. |
| B3 publication/freshness | Writer result contributes camera facts, readback/freshness, and Overlay wake | Would either call back into B3-owned runtime glue or duplicate/reorder publication. B3 is already closed with adapters `Remain`. |
| B4 Dialogue | Writer detects Dialogue context/source changes and may reset boundary state under `dialogueMutex` | Cross-TU extraction would enlarge access to Dialogue state or relocate lock ownership. Preserve current lock order. |
| B5 Cinematics | Writer continues/retire recovery and validates source/generation ownership produced by ENTER/EXIT | Moving writer glue independently splits the cross-callback recovery lifecycle and risks divergent owner views. B5 is approved closed with adapters `Remain`. |

No new mutable-state API, mutex, owner, or publication path is justified by this
review. No native ABI, lock-order, callback lifetime, or semantic change is
proposed.

## Existing evidence and remaining limits

The native runtime evidence audit records actual-adapter cases for Gameplay
ON/OFF and mode behavior, RuntimeCombine synchronization/readback, HorPlus,
AspectRecalculation, source identity (including RSI/RBX distinctions), and
selected recovery/ADS-like sequences. The remediation report subsequently
marks **N01 Fixed** and **N02 Fixed** within its tested offline scope; live
smoke evidence is ordinary-path evidence only. **N03 remains Partially
mitigated.**

Limits that remain relevant to B6 and are not discharged by this review:

- The harness does not execute the original displaced game store at
  `[RBX+0x30]` or prove final camera consumption in the live engine.
- Synthetic RuntimeCombine resize cases do not prove live viewport timing or
  all display-mode changes.
- Selected RSI/RBX and ADS-like cases do not prove all runtime source churn,
  camera handoffs, or source-lifetime behavior.
- Recovery helpers/adapters do not substitute for the final exact-artifact
  live regression through Cinematic EXIT and Gameplay recovery.
- The supplied live-smoke logs loaded ASI SHA-256
  `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771`, while
  the final N01–N03 remediation artifact is
  `E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81`.
  This exact-artifact mismatch remains an open live-validation item.
- N04–N06 remain separate evidence gates; this boundary review does not close
  them: startup/install/partial rollback orchestration, callback quiescence /
  patch-removal races, and replay provenance respectively.

These are evidence limitations, not findings that B6 extraction would fix.
No additional tests were run because this was a documentation-only review.

## Recommendation and stopping point

Recommend B6 `Remain` for the complete native writer adapter, its owner-bound
state/engine-operation factories, publication and diagnostic call sites;
classify its already-separated domain/transaction logic as `Already
extracted`. The remaining `runtime.cpp` Gameplay writer is orchestration over
the sole live owner, rather than an unextracted standalone Gameplay algorithm.

The user approved this disposition: B6 is **Complete — with documented
Remain decisions**, and the B1–B6 decomposition campaign is complete.
Do not classify B7 as `Remain` or `Already extracted` from this review: its
startup/installer/worker responsibilities were not reviewed. Recommendation
is to defer B7 and revisit only when a concrete, independently valuable
boundary or maintenance need appears. B7 remains unreviewed and unapproved;
this review does not authorize later work.

No source files, tests, builds, release artifacts, or runtime state were
changed or executed for this review.
