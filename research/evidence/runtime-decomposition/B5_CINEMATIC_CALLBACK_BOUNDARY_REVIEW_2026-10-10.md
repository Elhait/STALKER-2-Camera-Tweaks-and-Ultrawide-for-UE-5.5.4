# B5 Cinematic Callback Boundary Review

Date: 2026-10-10  
Scope: B5 boundary review and B1–B5 architecture checkpoint only  
Production/vendor changes, tests, builds, game launch, and ASI installation: **not performed**  
Decision status: **Approved closure — Complete with documented Remain decisions (2026-10-10)**

## Executive finding

The cinematic domain decisions are already separated into selection, aspect,
lifecycle transaction, recovery-authority, and continuation-trace helpers. The
remaining B5 code in `runtime.cpp` is the production integration boundary:
it owns the one `RuntimeState`, reads native callback context, serializes with
`CameraCorrectionGate`, coordinates other owner-held domains, and orders native
stores/register changes, publication, wake, and deferred diagnostics.

No candidate was found whose physical movement would reduce coupling or risk
without either exposing broad mutable owner state, introducing another
owner-operation API, or splitting a cross-callback lifecycle. The user
approved classifying B5 runtime adapters as `Remain` and existing domain
helpers as `Already extracted`; B5 is closed as **Complete — with documented
Remain decisions**. No B5 implementation or translation-unit split is needed.
The separately authorized B6 boundary review is recorded in
`B6_GAMEPLAY_WRITER_BOUNDARY_REVIEW_2026-10-10.md`; B6 implementation and B7
review/implementation remain unapproved.

## Evidence reviewed and limits

- Current `runtime.cpp`, `cinematic_lifecycle_transaction.hpp`, cinematic and
  gameplay domain helpers, and the move ledger were inspected on 2026-10-10.
- B5 adapter coverage and its gaps are taken from
  `NATIVE_RUNTIME_EVIDENCE_AUDIT_2026-10-09.md`, especially its B5 coverage
  rows and native callback/trampoline matrix.
- N01–N03 status and validation limits are taken from
  `N01_N03_NATIVE_RUNTIME_SAFETY_REMEDIATION_IMPLEMENTATION_2026-10-09.md`:
  N02 and N01 are `Fixed`; N03 is `Partially mitigated`.
- The user-provided live smoke logs were reviewed in the preceding task. They
  show ordinary startup/readiness and normal-path camera/Overlay activity, but
  their loaded/game-directory ASI identity was `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771`,
  not the remediation report's final production artifact
  `E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81`.
  Therefore those logs are not live-runtime proof for the final N01–N03
  artifact. The user reported a normal game exit; the logs do not establish
  hook-level teardown or callback quiescence.
- Offline actual-adapter evidence is bounded to its tested contexts. It does
  not execute the original game EXIT loads/call relocation, prove the live
  engine's final camera consumer, or run full production Initialize/install/
  reset orchestration.

No tests or builds were run because the review changed no code.

## B5 dependency map and proposed classification

| Responsibility / current production boundary | Main dependencies and ordering contract | Decision | Evidence / rationale |
|---|---|---|---|
| `GetCinematicLifecycleStateView` | Returns references/atomics for coordinator, selection/generation, active settings epoch/policy, transformed/EXIT FOVs, pre-ENTER Gameplay observations, continuation, writer association/recovery owner, Dialogue exclusion, and atomic handoff—all from the one resident owner | **Remain** | The view shape is already defined in `cinematic_lifecycle_transaction.hpp`; this factory binds it to `RuntimeState`. Moving the factory would export the same broad mutable state surface, not create a component. |
| `TraceCinematicEnter` gate and native context entry | Checks cinematic hook gate/stopping; acquires the non-recursive `CameraCorrectionGate`; reads prior coordinator and qualified Gameplay observation/baseline; consumes native low `XMM0` after the original load/call-site sequence | **Remain** | Callback ABI, admission, owner state, and lifecycle are inseparable at this adapter. Existing adapter matrix is partial; the game-site call stack and engine consumption remain live-only. |
| ENTER selection and generation | Reads requested aspect/FOV mode and enabled state, then `PrepareCinematicEnter` validates selection and advances generation through owner-bound state | **Remain** (adapter); **Already extracted** (selection policy) | Selection snapshot and capture validation exist in `cinematic_selection.*`; generation commit is in the shared transaction helper but uses the owner-bound state view. Moving only the policy is already done; moving the commit adapter adds no boundary. |
| ENTER aspect/baseline resolution | `ResolveCinematicAspect(selection.aspectPolicy)` uses runtime viewport resolver and constants; reads Gameplay baseline, writer association/publication, diagnostic provenance, previous continuation targets | **Remain** (runtime resolution/provenance adapter); **Already extracted** (aspect/baseline policy) | `cinematics::ResolveAspect`, selection/baseline selection, and transform rules are domain helpers. The current policy is captured from the accepted selection; viewport and owner observations are runtime inputs. |
| Accepted ENTER state transition | `ProcessAcceptedCinematicEnter` validates and updates selection, active policy/epoch, association generation, transformed target, continuation, recovery/exclusion/handoff, and coordinator; caller then begins camera-transition boundary | **Remain** (callback orchestration); **Already extracted** (transaction) | Helper-level transaction is present and actual adapter scenarios exercise selected paths. Splitting the caller would divide one ordered transition across owner API and TU boundaries. |
| ENTER native register and publication ordering | Writes low `XMM0` only after accepted transaction; commits FOV observation and cinematic transition facts; emits diagnostics/logging and wakes semantic presentation via existing publication paths | **Remain** | The order ties native output, publication freshness, and observability. B3 already classified publication/wake adapters as owner-bound `Remain`. No evidence supports moving this callback tail independently. |
| `ApplyCinematicAspectStore` | Reads target from `RAX`; advances `RIP` by the replaced 10-byte store length before auxiliary work; observes lifecycle/association; resolves Auto viewport; uses checked memory write or fail-closed path | **Remain** (adapter); **Already extracted** (store policy/application) | `ObserveCinematicAspectStore`, `ResolveAspect`, and `ApplyAspectStore` are existing helpers. Adapter case covers controlled valid/read-only/OFF/null/no-access paths and stopped lifecycle, but not live target lifetime/instruction binding. `RIP` and actual memory semantics must stay with the hook boundary. |
| Writer/source association and generation identity | ENTER begins the association generation; aspect-store callback associates target object; EXIT reads association and recovery owner; Gameplay writer later validates source and retires/completes recovery | **Remain** (cross-owner wiring); **Already extracted** (association/recovery domain types) | `CinematicWriterAssociation` and `CinematicRecoveryOwner` are in cinematic domain code, but the callbacks coordinate them with shared writer state. The chain crosses B5 and B6 and cannot be moved as an isolated ENTER/EXIT helper. |
| `TraceCinematicExit` admission and native operands | Checks hook gate/stopping, acquires correction gate, captures generation/coordinator; native site has legacy and indexed source-load forms; callback must preserve the loaded native low `XMM0` and context | **Remain** | Adapter evidence covers accepted/rejected and indexed paths, but the actual original load variants and call/trampoline relocation at the game site are explicitly unverified. Moving this boundary does not improve that evidence and increases ABI risk. |
| EXIT dispatch / rejection | `DispatchCinematicExit` decides accepted vs rejected lifecycle; rejected logging is deferred until gate release and makes no state mutation | **Remain** (runtime adapter); **Already extracted** (dispatch policy) | Dispatch authority is in `cinematic_exit_authority.*`. Gate-aware deferred logging and native callback ordering belong to runtime. |
| EXIT recovery handoff | Reads previous continuation/source/target, Gameplay settings and recovery-observer capability; resolves next coordinator; `ProcessAcceptedCinematicExit` captures/clears owner, carries rapid-chain state, updates exclusion/handoff and selection lifecycle | **Remain** (orchestration); **Already extracted** (transaction/policy) | `ResolveCinematicExitTransition`, accepted-exit transaction, and recovery-owner helpers are already separate. Runtime still coordinates Dialogue reset, diagnostics, Gameplay writer continuation, and owner publication in order. |
| Rapid-chain continuation | ENTER→EXIT→ENTER generation replacement, empty/superseded generations, source identity, continuation trace, recovery completion/retirement, and writer-side interpolation | **Remain** (cross-callback orchestration); **Already extracted** (reentry/continuation rules and trace model) | Existing actual-adapter rapid-chain cases establish selected sequences, not the original native call/loads or all source churn. The same state is consumed by Gameplay writer and settings disable paths; moving just B5 callbacks would not be an independent lifecycle owner. |
| Gameplay recovery handoff and completion | EXIT prepares recovery; Gameplay writer validates samples/source/endpoints and completes or rejects recovery; helper wrappers clear auxiliary and Dialogue exclusion state | **Remain** (runtime glue); **Already extracted** (recovery decisions) | The runtime flow spans B5 EXIT and B6 writer callback. Existing local recovery tests do not establish live source churn or final camera output. No second coordinator/state owner is acceptable. |
| Publication, freshness, Overlay wake | Camera transition/FOV publication and wake operate on shared snapshot and runtime state under existing gate/mutex order | **Remain** | This is the accepted B3 disposition. Reopening it as a B5 side effect would violate the approved boundary and increase synchronization coupling. |
| Continuation diagnostics | `CinematicContinuationTrace` event model is a diagnostics-domain type; begin/record/finish/flush call sites bind generation, targets, Diagnostic Mode, and runtime logger | **Remain** (runtime adapters); **Already extracted** (trace model) | The trace model is extracted. Begin/flush call sites occur in ENTER, EXIT, Gameplay writer, and settings changes; logger deferral follows gate ownership. Moving one group would split trace lifetime. |
| B5 transactional installer functions (`ResolveCinematicFovCallsites`, `InstallCinematicAspectComponent`, `InstallCinematicFovComponent`, `CommitCinematicComponents`, `RollbackCinematicAspectComponent`, `RollbackCinematicFovComponent`) | Executable scanning and instruction validation, disabled hook creation, coordinated enable commit, reverse disable/reset, gate publication and component capability | **Remain** | These are production owner-bound installers: the commit groups aspect+ENTER+EXIT and publishes the hook gate only after enable success. They share RuntimeState HookSet and startup ordering; moving them would not create an independent owner. The existing `cinematic_initialization.hpp` helper tests cover transaction decisions/rollback ordering only, not these production functions or full Initialize/install/reset orchestration. N03 remains partial; N04–N06 remain separate verification gates. |
| Cinematic resolver/application primitives | Bounded executable-span/signature/store validation and aspect application | **Already extracted** | Existing `cinematics::ResolveAspectStore`, `ResolveAspect`, and `ApplyAspectStore` helpers; this classification does not certify live signature/site binding or target lifetime. |

### Native side-effect and lifecycle invariants to preserve

1. Aspect-store callback uses `RAX` target and advances `RIP` exactly by the
   replaced store length before logging/auxiliary work; successful writes remain
   validated and failed writes preserve the existing native fail-closed path.
2. ENTER accepts only the current validated generation, selects policy from
   the captured selection, and applies its low-`XMM0` output only after the
   transaction accepts. Publication and continuation evidence follow the
   existing order.
3. EXIT preserves the native FOV operand and distinguishes legacy/indexed
   instruction forms. Rejected EXIT has no lifecycle mutation; accepted EXIT
   captures recovery ownership only from the current validated association.
4. Correction-gate ownership, Dialogue lock scopes, deferred logging, freshness
   publication, Overlay wake, and the single coordinator remain unchanged.
5. Recovery interpolation/completion remains shared with the Gameplay writer;
   an extraction must not duplicate generation, source, or coordinator state.

## Evidence sufficiency by candidate

The current evidence is sufficient to support a **Remain** recommendation; it
is not sufficient to claim full live callback correctness or to justify moving
the native adapters:

- ENTER: actual-adapter cases include invalid/rejected and selected initial,
  continuation, generation, source-association and disable/reenable scenarios.
  The original game call-site/callee behavior and live engine camera consumer
  are not exercised.
- EXIT: accepted, rejected and indexed adapter paths are represented, but the
  actual legacy and indexed original memory loads/call relocation are not
  executed by the fixtures; native source-operand accessibility is not proven.
- Aspect store: controlled store/skip and context behavior are covered; live
  instruction binding and target-object lifetime remain unverified.
- Installers: helper transaction, resolver and capability tests do not run
  actual production Initialize/install/reset orchestration. No helper PASS is
  promoted to whole-startup proof.
- N01/N02: remediation report records both `Fixed` with offline fault/adapter
  coverage. The normal-path live logs reviewed for this smoke test identify a
  different ASI hash than the report's final artifact, so they do not close
  exact-artifact runtime validation. N01 concurrent game callback/teardown
  behavior and N02 actual game-site exception/continuation behavior remain
  bounded live gates.
- N03: remains **Partially mitigated**. Thread-entry fault containment and
  controlled cleanup are tested, but full game-specific initialization,
  partially installed live hooks, and teardown are not. The live smoke is
  ordinary success-path evidence only and is for the hash mismatch noted above.
- N04: full production startup/install/partial rollback remains unverified.
- N05: game callback quiescence and patch-removal races remain unproven; the
  supported process-resident lifetime must not be confused with dynamic unload.
- N06: current native replay provenance remains absent for the audit's four
  replay fixtures; only identity-tagged evidence may support current-runtime
  claims.

These are evidence limits, not newly asserted production defects. No new tests
are requested merely to record a B5 `Remain` recommendation.

## B1–B5 architecture checkpoint

| Batch | Verified structural result | What remains in `runtime.cpp` |
|---|---|---|
| B1 | Private cross-TU access boundary binds the same process-resident runtime owner; no second owner/gate was introduced | Owner definition, lifetime and runtime orchestration |
| B2 | Hotkey mapping and persistence moved to `runtime_settings.cpp`; four settings/readback functions were approved `Remain` | Mutations/readback coupled to coordinator, gate, owner state and lock order |
| B3 | Pure snapshot/publication/freshness helpers and performance event-counter implementation are separate | Cross-owner publication/readback, freshness, wake and gate-aware deferred logging |
| B4 | Dialogue lifecycle/policy/transaction helpers are separate | Native callback, state-view factory, reset wrappers, diagnostics and install orchestration |
| B5 | Cinematic selection/aspect/lifecycle/recovery/continuation domain rules are already separate | Native aspect/ENTER/EXIT adapters, owner-bound state view, cross-callback generation/association/recovery wiring, publication/wake and installer orchestration |

Within the reviewed B1–B5 scope, the remaining code is predominantly a
composition root and native integration owner, not an unextracted standalone
Dialogue/Cinematic policy engine. This is not a claim that every domain
responsibility elsewhere in `runtime.cpp` has been audited: B6 Gameplay writer
and B7 startup/worker/hotkey orchestration require separate boundary decisions.

Residual architectural risks are the already documented shared mutable owner
surface, high integration coupling across ENTER/EXIT/Gameplay recovery, native
site ABI/relocation uncertainty, unverified actual installer/reset failure
orchestration, callback-quiescence limits, and incomplete exact-artifact live
evidence. Splitting files without a narrower owner boundary could relocate
these risks rather than reduce them.

## Recommendation for B5 and future reviews

1. Approve B5 closure as **Complete — with documented `Remain` decisions**;
   keep the B5 runtime callback/state-view/installer adapters in `runtime.cpp`
   and mark the already-separated helpers `Already extracted`.
2. Do not start B5 implementation. No new API, mutex, state store, callback
   facade, or build/test work is recommended for this disposition.
3. Do not automatically run B6 or B7. A single B6 boundary review has a
   plausible information benefit because the shared Gameplay writer is a
   distinct large native adapter with an existing actual-adapter matrix; that
   review would need to prove meaningful coupling reduction and address the
   original displaced store and recovery coupling, not merely relocate lines.
4. B7 has no demonstrated extraction boundary yet. Its startup/install/reset/
   worker responsibilities are strongly composition-root and failure-order
   work, with N03 partial and N04–N06 gates open. Defer any B7 review until a
   concrete maintenance problem or a B6 result identifies a coherent target.
   This is a prioritization recommendation, not a blanket `Remain`
   classification of unreviewed B7 symbols.
5. The user approved B5 closure and separately authorized only the B6 boundary
   review on 2026-10-10. B6 implementation and B7 review/implementation remain
   unapproved; see the B6 findings for its recommendation and stop point.

