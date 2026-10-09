# Overlay and shared Camera State architecture audit

**Date:** 2026-10-07  
**Status:** initial read-only source audit, followed by the bounded F9 disposition in Finding 9. That remediation was built and tested offline; no new in-game validation was performed.  
**Scope:** production observation/publication, shared snapshot, semantic read, Camera Integration projection, expanded Camera State, settings/status presentation, and their lifecycle. This report describes the current dirty workspace source, not a particular installed ASI. Earlier screenshots and logs motivated the audit but do not independently validate the findings against a named binary.

## Evidence discipline

- **Code-proven defect:** current source establishes a contradiction with the stated presentation/data contract or an incorrect source-of-truth label. A possible visual manifestation is not claimed as observed unless separately stated.
- **Architectural risk:** a concrete mechanism could produce an incorrect or misleading display under specified conditions; the runtime occurrence is not established here.
- **Redundant state:** the same or closely related semantic quantity is stored more than once; this does not by itself justify deletion.
- **Ambiguous semantics:** the source supports more than one interpretation of a label or lifetime; clarify the contract before modifying production ownership.
- **Intentional behavior:** explicitly implemented or tested current behavior; it may still conflict with a newer requirement.
- **Insufficient evidence:** source inspection alone cannot establish engine-visible output, callback order in a particular scene, or an unobserved lifecycle boundary.

The shared snapshot reports a mod-observed/selected transaction result. It is **not** a readback of the engine's final projection. `Current` is relative to the individual fact or invalidation group, not a guarantee that every displayed value was sampled atomically. See [snapshot fact types](../../src/camera/camera_state_snapshot.hpp#L20) and [semantic snapshot contract](../../src/plugin/runtime_settings.hpp#L96).

## Authoritative data-flow map

```text
INI / effective Runtime settings ──→ RuntimeSettingsSnapshot ──→ controls, hotkeys,
                               │                              language, font size
                               └──→ OverlaySemanticSnapshot ──→ feature statuses,
                                                               active/next selection

Gameplay writer ──→ FOV observation store ──→ GameplayBaselineStore (production baseline)
                └──→ shared CameraStateSnapshot ──┬──→ ProjectCameraIntegration
                                                  │     └──→ summary renderer
                                                  └──→ expanded Camera State renderer

Cinematic ENTER ──→ separate Transition Boundary Observation ──→ transition assessment
Cinematic/Recovery writer ──→ shared Native / Target / Output FOV roles
Dialogue hook ──→ shared Native / Output FOV roles; retained cinematic target

client-window query ──→ runtime viewport fact ──→ summary
active/configured cinematic policy + viewport ──→ UI-resolved target preview
```

`RuntimeState` is the integration owner; the shared Camera State is stored there under its own mutex. The semantic read copies it, plus independently owned settings/capability/coordinator/dialogue/selection/baseline/boundary/viewport facts. The semantic object is a read-only projection, not another persistent camera store. Relevant entry points: [RuntimeState aliases](../../src/plugin/runtime.cpp#L220), [full Gameplay publication](../../src/plugin/runtime.cpp#L4138), [transaction publication](../../src/plugin/runtime.cpp#L4229), [semantic read](../../src/plugin/runtime.cpp#L5950), [integration projection](../../src/overlay/camera_integration.cpp#L90), [summary renderer](../../src/overlay/renderer_runtime.cpp#L1877), [expanded renderer](../../src/overlay/camera_state_view.cpp#L136).

### Visible-value inventory

| Visible group | Authoritative producer / cache | Validity, freshness, invalidation and final projection |
| --- | --- | --- |
| Enabled flags, Gameplay/Cinematic/Dialogue modes, locale, font size and hotkeys | Effective Runtime settings; persisted config is input, not the display authority. `ReadRuntimeSettings` copies effective settings and hotkeys. | Controls render `frameSettings`; mutations go through the settings API. Semantic settings facts are independently read for statuses and active/next policy decisions. No camera observation is implied. [ReadRuntimeSettings](../../src/plugin/runtime.cpp#L5921), [controls](../../src/overlay/renderer_runtime.cpp#L1651). |
| Gameplay/Cinematics/Dialogue status and explanatory text | `Project*Presentation` consumes configured/effective settings, hook availability, transition flags, coordinator, active selection, Dialogue phase, and for RuntimeCombine, confirmation telemetry from Camera State. | These are mod capability/readiness/lifecycle claims, not independent FOV measurements. `RuntimeCombine` requires `runtimeCombineValid && runtimeCombineConfirmed && coordinator==Gameplay` to say Active. [Status projections](../../src/overlay/feature_presentation.cpp#L22). |
| Viewport Aspect | Win32 process-window client rectangle sampled at semantic read. | `AtRead`; invalid if no usable client rectangle. Not the writer aspect or final engine projection. Displayed only after aspect formatting. [Viewport resolver](../../src/platform/win32/viewport.cpp#L19), [semantic fact](../../src/plugin/runtime.cpp#L6124), [renderer](../../src/overlay/renderer_runtime.cpp#L1893). |
| Gameplay FOV | Shared role derived from retained neutral/stable Gameplay writer samples; related but not equivalent to `GameplayBaselineStore`. | Per-role `valid/provenance/freshness/source/sequence`; neutral capture requires Gameplay, inactive Dialogue, no exclusion/pending/zoom, applied correction, ultrawide aspect and three matching samples. ENTER/EXIT and non-Gameplay transactions retain it rather than making it a live sample. Summary uses the role directly; details add provenance/freshness. [Capture](../../src/camera/camera_state_publication.hpp#L196), [transaction retention](../../src/camera/camera_state_publication.hpp#L448). |
| Native FOV | Incoming writer XMM0 or Dialogue XMM6 supplied to transaction publication. | Per-role Current on a valid transaction, Historical on writer-observation invalidation. It means native input to the observed hook, not final engine/native projection. [Gameplay publication](../../src/camera/camera_state_publication.hpp#L301), [transaction publication](../../src/camera/camera_state_publication.hpp#L455), [Dialogue hook](../../src/plugin/runtime.cpp#L3669). |
| HorPlus Target | Actual applied Gameplay policy output or production Cinematic/Recovery target supplied to transaction publication. | Per-role Current when an actual target is published, Retained when a transaction supplies no new target; full Gameplay pass-through has a separate edge case in Finding F2. It is not recomputed by the UI. [Gameplay publication](../../src/camera/camera_state_publication.hpp#L331), [transaction publication](../../src/camera/camera_state_publication.hpp#L459). |
| Current Output FOV | Selected return value of Gameplay/Cinematic/Recovery writer transaction or Dialogue boundary transaction. | Per-role Current on publication; Historical on writer-observation invalidation. It is a mod transaction result, not a readback of the final camera projection. [Gameplay publication](../../src/camera/camera_state_publication.hpp#L320), [transaction publication](../../src/camera/camera_state_publication.hpp#L457), [Dialogue publication](../../src/plugin/runtime.cpp#L3786). |
| Writer Aspect, Policy Aspect, writer flags | Full Gameplay writer observation stored in shared snapshot. | Numeric validity is separate from the group `writerObservation` freshness. Summary requires Current for aspects; details print valid historical numbers and the group freshness. Flags are hidden unless Current. Transaction-only FOV updates do not refresh these Gameplay-writer facts. [Publication](../../src/camera/camera_state_publication.hpp#L336), [summary gate](../../src/overlay/camera_integration.cpp#L111), [details](../../src/overlay/camera_state_view.cpp#L257). |
| RuntimeCombine detail | Gameplay writer result: aspect before/target/after, mismatch, write/readback/confirmation, flags, input/output FOV. | Copied only on full Gameplay publication; individual validity gates include `runtimeCombineValid` and readback validity. Details show it when the snapshot's published Gameplay mode is RuntimeCombine. [Publication](../../src/camera/camera_state_publication.hpp#L357), [details](../../src/overlay/camera_state_view.cpp#L286). |
| Presentation, Gameplay mode, zoom, Dialogue, source and generation details | Full Gameplay writer publication copies coordinator-derived presentation, effective mode, zoom/Dialogue metadata and writer source. Transaction-only FOV publication updates role facts and global event sequence but not all of these groups. | Expanded view displays each group's provenance/freshness, a source token and counters. `presentation.state` can therefore be the last full writer state, not the current transaction owner. Source tokens are observed pointers, not independently confirmed engine camera identity. [Publication](../../src/camera/camera_state_publication.hpp#L275), [details](../../src/overlay/camera_state_view.cpp#L149). |
| Aspect Match, FOV Match, Camera Transition | Separate Gameplay-before-ENTER and Cinematic-ENTER observations. | Projection checks accepted boundary, generation, boundary kinds, Gameplay provenance and sequence order; then compares observed policy aspect against ENTER aspect and FOV with canonical semantic tolerance. Config/mode are not evidence for Match/Mismatch. Currently the assessment is displayed only while `CinematicActive`. [Boundary capture](../../src/plugin/runtime.cpp#L3112), [assessment](../../src/overlay/camera_integration.cpp#L125). |
| Cinematic Aspect Target, Selection, FOV Mode | Active latched selection during a cinematic, otherwise next configured selection; UI resolves aspect again from selected policy and read-time viewport. | These are configured/derived predictions. Neither the actual aspect-store result nor the live cinematic FOV is published by these rows. [Projection](../../src/overlay/camera_integration.cpp#L183), [renderer](../../src/overlay/renderer_runtime.cpp#L1985). |

Summary FOV projection checks each fact's validity and the numeric range `1 < FOV < 179`, **without a coordinator gate**. The renderer appends `Current`, `Retained`, or `Last known`. The expanded view reads the same four facts directly and displays provenance/freshness, but its numeric formatter checks only `valid && finite`. [Projection](../../src/overlay/camera_integration.cpp#L16), [summary](../../src/overlay/renderer_runtime.cpp#L1910), [details](../../src/overlay/camera_state_view.cpp#L225). Formatting an aspect as `X:9 (decimal)` is presentation only; the semantic re-resolution of Cinematic Auto is not.

## Lifecycle audit

The entries below describe **what this code can establish**, not a guarantee that every hook fires during a particular game sequence. `C/R/H` mean Current/Retained/Historical; `U` means Unavailable until a qualifying observation exists.

| Phase | Shared FOV roles | Ownership and transition evidence |
| --- | --- | --- |
| Main Menu / before any camera publication | Normally all U, because no shared snapshot has yet been published. | Coordinator defaults to Gameplay; there is no MainMenu enum. Returning to menu after Gameplay has a separate stale-state risk (F10). |
| Gameplay writer | Native and Output C after valid full publication; Target C if applied; Gameplay FOV C only after qualifying stable neutral samples, otherwise U or retained/historical prior value. | Full publication refreshes presentation, mode, Dialogue/zoom, aspect, source and sequence. `GameplayBaselineStore` has different eligibility (F1). |
| Accepted Cinematic ENTER | Existing Gameplay FOV becomes R; existing target becomes R; presentation/Dialogue groups become H until refreshed. | A distinct ENTER boundary observation is captured. ENTER does not itself publish four live FOV roles into the shared snapshot; a later writer transaction may do so. No inference that a live sample is absent in every runtime frame. [ENTER](../../src/plugin/runtime.cpp#L3112). |
| CinematicActive writer | Native and Output C; Target C if production transaction supplies it, otherwise R; Gameplay FOV R. | Transaction `owner` is CinematicActive, while `presentation.state` may still describe the last full Gameplay publication (F6). [Transaction publication](../../src/camera/camera_state_publication.hpp#L432). |
| Dialogue inside Cinematic | Dialogue observation can replace Native and Output with C; target remains R unless another transaction supplies a target. Gameplay FOV remains R. | Dialogue phase is independently sampled. A source-specific final camera projection is not measured. [Dialogue hook](../../src/plugin/runtime.cpp#L3659). |
| Cinematic continuation | The next accepted writer transaction again publishes Native/Output C and a production target when available. | Detailed Gameplay-writer aspect/flags may remain historical because transaction-only publication does not replace them. |
| EXIT / Recovery | Native/Output C on accepted recovery transaction; Target C when supplied or R otherwise; Gameplay FOV R until Gameplay confirmation. | ENTER evidence remains stored, but the UI projection suppresses its operands and evaluation outside CinematicActive (F3). [EXIT freshness](../../src/plugin/runtime.cpp#L3372). |
| Gameplay after handoff | New full Gameplay publication updates Native/Output and other Gameplay groups; Gameplay FOV becomes C only when its stable neutral criterion confirms it. | Coordinator can return to Gameplay before every shared fact is re-confirmed. This is not permission to label older samples Current. |

Group invalidation is implemented in [camera_state_publication.hpp](../../src/camera/camera_state_publication.hpp#L100). Settings invalidation maps Gameplay enable/mode and Cinematic/Dialogue policy changes to freshness groups in [camera_writer_transaction.hpp](../../src/plugin/camera_writer_transaction.hpp#L33). A `valid` role can be Retained or Historical; neither is automatically Unavailable. Conversely, a container-level `cameraStateValid` means only that a snapshot exists.

## Findings and recommended bounded directions

### F1 — Dual Gameplay baseline authority diverges at 16:9

**Classification:** code-proven defect in the desired shared publication contract; related redundant state. **Cause:** production `GameplayBaselineStore` accepts a valid pass-through Gameplay writer, including 16:9, while shared `gameplayFov` needs ultrawide, applied HorPlus and three stable neutral samples. [Production eligibility](../../src/camera/gameplay_baseline.cpp#L29); [shared eligibility](../../src/camera/camera_state_publication.hpp#L204); [ultrawide predicate](../../src/gameplay/aspect_policy.cpp#L7). **Possible runtime manifestation:** `gameplayBaselineUsable=true` in the semantic snapshot while summary Gameplay FOV says Unavailable. **Direction:** define whether the two baselines represent the same semantic role, especially during zoom/source changes. Align publication only after proving equivalence; do not add a UI fallback straight from the production store.

### F2 — Full Gameplay pass-through may leave an old HorPlus Target Current

**Classification:** code-proven defect in per-role freshness under the stated current-transaction interpretation; runtime incidence not exercised here. **Cause:** full publication copies the previous snapshot; `horPlusTargetFov` changes only when `input.applied && horPlusValid`. With an unapplied same-source writer sample and no preceding invalidation, the previous valid target/freshness can survive unchanged beside a newly published pass-through Output. [Copy and target update](../../src/camera/camera_state_publication.hpp#L210). **Possible manifestation:** `HorPlus Target (Current)` is actually a prior policy result. **Direction:** characterize applied→pass-through within one source and define target retention for that path before changing code.

### F3 — Validated ENTER evidence is hidden after EXIT, although stored

**Classification:** intentional behavior conflicting with the requested evidence lifetime. **Cause:** RuntimeState resets boundary at the next accepted ENTER and does not clear it on EXIT; semantic read still exports accepted evidence. `ProjectCameraIntegration` forces Waiting in Gameplay and only copies/assesses operands during CinematicActive. The current harness explicitly expects that behavior. [Runtime capture](../../src/plugin/runtime.cpp#L3112), [semantic export](../../src/plugin/runtime.cpp#L6100), [projection](../../src/overlay/camera_integration.cpp#L125), [test](../../tests/overlay/camera_integration_harness.cpp#L294). **Manifestation:** after EXIT, no visible last validated boundary operands/result despite stored evidence. **Direction:** separate live-next-transition status from historical last validated ENTER, with generation and provenance; do not present historical evidence as current camera FOV.

### F4 — ENTER operand is labelled like live Cinematic FOV, and details omit operands

**Classification:** code-proven presentation defect. **Cause:** `integration.cinematicFov` is populated from `CameraTransitionBoundaryObservation.cinematic.resultFov`, then rendered as `Cinematic FOV` among four main FOV rows. Expanded Camera State has no Transition Evidence section or operand source/sequence/generation. [Boundary projection](../../src/overlay/camera_integration.cpp#L148), [summary placement](../../src/overlay/renderer_runtime.cpp#L2045), [details](../../src/overlay/camera_state_view.cpp#L225). **Manifestation:** a one-time ENTER output can be read as live cinematic output; an assessed Match cannot be audited in details. **Direction:** label it `Cinematic ENTER FOV` and keep both operands and assessment in a separately identified evidence area.

### F5 — Freshness changes can be stored without waking an open Overlay

**Classification:** code-proven notification gap; visible delay is conditional on other redraw events. **Cause:** `MarkCameraStateFreshness` and `MarkCameraStateForSetting` mutate the shared snapshot without notifying Overlay. Full publication always notifies; transaction publication does so only on `SnapshotSemanticsChanged`. That predicate omits per-role source/sequence, generation event sequence and several numeric diagnostics shown in details. [Mutators](../../src/plugin/runtime.cpp#L4119), [transaction notify](../../src/plugin/runtime.cpp#L4253), [predicate](../../src/camera/camera_state_snapshot.cpp#L188), [event-driven presenter](../../src/overlay/composition_runtime.cpp#L801). **Possible manifestation:** prior `Current`, source or sequence remains visibly displayed until another wake. **Direction:** establish which visible semantic changes require a redraw and test the wake contract; avoid indiscriminate per-callback rendering.

### F6 — Transaction owner and last full-publication presentation are conflated

**Classification:** ambiguous semantics, plus a code-proven misleading diagnostic label. **Cause:** transaction publication receives an owner derived from the current coordinator and uses it to retain Gameplay FOV, but does not rewrite `snapshot.presentation.state`. The `CAMERA_FOV_STATE owner=` log prints that older presentation field; expanded Camera State presents it as `State`, with separate Historical freshness but no adjacent current coordinator. [Owner assignment](../../src/plugin/runtime.cpp#L4244), [publication](../../src/camera/camera_state_publication.hpp#L448), [log](../../src/plugin/runtime.cpp#L4268), [details](../../src/overlay/camera_state_view.cpp#L149). **Manifestation:** `owner=Gameplay` while `coordinator=CinematicActive` can describe two different metadata lifetimes, not necessarily wrong camera ownership. **Direction:** name transaction owner, current coordinator and last full-publication presentation separately; do not mechanically change camera ownership.

### F7 — Cinematic target is re-resolved in presentation with a different fallback

**Classification:** architectural risk / ambiguous `Target` label. **Cause:** Overlay resolves Auto from current client viewport and returns unavailable if it is invalid. Production cinematic resolution can fall back from client to display and then native aspect; actual store may also be refused. [UI resolver](../../src/overlay/camera_integration.cpp#L22), [viewport fallback](../../src/platform/win32/viewport.cpp#L38), [production resolver](../../src/plugin/runtime.cpp#L2218), [store](../../src/plugin/runtime.cpp#L2327). **Possible manifestation:** summary target is unavailable or differs from the aspect production selected/applied. **Direction:** call this a configured/selected target preview, or publish the actual production resolution/store outcome if the UI promises an observed applied aspect.

### F8 — Summary and details apply different availability gates to writer aspects

**Classification:** ambiguous semantics; code-proven display divergence. **Cause:** summary requires `writerObservation==Current`; details print valid writer/policy aspect numerically even when the group is Historical, followed by a shared freshness row. The FOV summary additionally range-checks, while details only require valid+finite; production publishers normally enforce the range, so a normal-source FOV contradiction is **not proven**. [Summary](../../src/overlay/camera_integration.cpp#L107), [details](../../src/overlay/camera_state_view.cpp#L61), [aspect rows](../../src/overlay/camera_state_view.cpp#L257). **Manifestation:** `Writer Aspect: Unavailable` in summary versus a numeric last-known aspect in details, without a clear historical label on that value. **Direction:** explicitly distinguish live summary from historical details or share one value/freshness formatting contract.

### F9 — Legacy aggregate FOV fields duplicate the canonical roles

**Initial classification:** redundant state / architectural risk; deletion required pair-by-pair proof. **Disposition after bounded consolidation:** `retainedGameplayFov` scalar/validity/provenance/source/freshness was a proven second copy of `gameplayFov` and is removed. The canonical `CameraFovFact gameplayFov` now owns all role metadata; its stable-neutral qualification is unchanged. `GameplayBaselineStore` remains separate because production correction accepts a broader observation contract and has its own invalidation/consumers. The `nativeWriterFov` numeric copy is removed; `nativeFov` owns the number while `writerObservation` freshness independently records whether the full writer aspect/flags group is Current/Historical/Unavailable. The `transformedFov` numeric copy is removed; applied writer output/target are held by canonical facts while `transformedResult` freshness independently records whether the latest full writer sample transformed. Pass-through can therefore make that group Unavailable while the previous HorPlus Target remains Historical. See the [F9 consolidation map](../active/OVERLAY_SHARED_CAMERA_STATE_F9_CONSOLIDATION_PLAN.md). **Regression:** production-equivalent writer transaction harness covers stable baseline acquisition, source transition, settings boundary, Cinematic/Dialogue/Recovery lifecycle, and Gameplay handoff using canonical facts. **Residual:** Main Menu/save-load session invalidation remains unknown; full suite still stops on existing CJK glyph coverage.

### Independent post-F9 review

A separate read-only Sol review found **no correctness blocker** to the planned runtime lifecycle acceptance. It confirmed that the writer-observation invalidation preserves the previous `nativeWriterFovValid || aspectValid` behavior through `writerObservationHasEvidence`, and that lifecycle coverage spans Gameplay through Recovery and handoff. Two non-blocking, pre-existing presentation risks remain outside this remediation: (1) summary viewport aspect is read during semantic snapshot capture, while expanded Camera State uses the viewport aspect from the last full writer publication, so a resize can make those two displays differ; (2) RuntimeCombine detail telemetry has no explicit freshness marker after lifecycle changes. Neither was changed in F9. Main Menu/save-load invalidation remains open for lack of an authoritative event. The audit clears F9 for matching-artifact runtime acceptance, but does not itself provide runtime evidence.

### F10 — Main Menu has no explicit camera-state lifetime boundary

**Classification:** architectural risk / insufficient runtime evidence. **Cause:** coordinator defaults to Gameplay and its enum has no MainMenu; no explicit shared-snapshot reset was found in the inspected return-to-menu path. [Coordinator default](../../src/plugin/runtime.cpp#L220), [overlay enum](../../src/plugin/runtime_settings.hpp#L80). **Possible manifestation:** after leaving Gameplay, old facts may remain marked Current if no later callback invalidates them. This was not established by an in-game return-to-menu test. **Direction:** characterize that boundary with the same artifact and decide whether it invalidates or retains each role; do not infer menu ownership solely from coordinator.

### F11 — Semantic read and controls do not form one atomic UI frame

**Classification:** architectural risk; transient inconsistency not measured. **Cause:** renderer reads `RuntimeSettingsSnapshot` and later `OverlaySemanticSnapshot` separately, then can apply control mutations while continuing to render status/summary from the earlier semantic read. Summary Mode uses `frameSettings`, feature status uses semantic settings. [First read](../../src/overlay/renderer_runtime.cpp#L1491), [second read](../../src/overlay/renderer_runtime.cpp#L1587), [control mutation](../../src/overlay/renderer_runtime.cpp#L1654), [summary](../../src/overlay/renderer_runtime.cpp#L2024). **Possible manifestation:** one frame shows new selector value with previous feature status. **Direction:** verify whether next-frame readback/redraw reliably resolves it; only then decide whether an explicit frame-version or post-mutation refresh is needed.

### F12 — Per-role source/sequence exist but are not exposed beside each FOV

**Classification:** architectural risk / diagnostic coverage gap. **Cause:** each `CameraFovFact` carries source and publication sequence, but expanded view shows only value, provenance and freshness for each role, plus one general Gameplay writer source and global event sequence. [Role structure](../../src/camera/camera_state_snapshot.hpp#L131), [detail rows](../../src/overlay/camera_state_view.cpp#L225), [general source](../../src/overlay/camera_state_view.cpp#L317). **Manifestation:** mixed Gameplay/Dialogue/Cinematic samples can look like one simultaneous observation; exact per-role age cannot be audited from UI alone. **Direction:** expose per-role source/sequence where diagnostics needs them, or explicitly state that the displayed roles come from different transactions.

## Established non-defects and proof limits

- **Intentional/verified in source:** the four-role integration projection reads shared facts without gating them on Gameplay coordinator. A valid Retained Gameplay FOV remains available in CinematicActive, Dialogue and Recovery; the existing projection harness exercises those cases. [Projection](../../src/overlay/camera_integration.cpp#L96), [harness](../../tests/overlay/camera_integration_harness.cpp#L125).
- **Intentional/verified in source:** Aspect/FOV Match uses accepted boundary observations, with ordering/generation checks and the project's semantic FOV tolerance. Configured modes do not prove Match/Mismatch. [Assessment](../../src/overlay/camera_integration.cpp#L46).
- **Insufficient evidence:** static source does not prove the final visible FOV, actual engine projection aspect, precise callback interleaving during a specific Dialogue scene, or that a suspected redraw delay was visible in the user's runtime. No build or test was run for this report.
- **Existing test limit:** the integration harness tests the projection object; it does not render both summary and expanded Camera State together or establish a screenshot-level consistency contract. It presently asserts the post-EXIT suppression described in F3. [Harness](../../tests/overlay/camera_integration_harness.cpp#L294).

## Bounded follow-up order

Investigate F1/F2 as shared publication lifetime contracts; F3/F4 as separate transition-evidence presentation; F5 as delivery semantics. Resolve F6–F12 through narrow characterization or naming decisions before considering state removal or structural refactoring. None of these findings authorizes a change to camera correction, owner/coordinator transitions, or a release claim.

## F6–F8 bounded remediation update (2026-10-07)

Implementation and offline characterization are recorded in [the F6–F8 remediation plan](../active/OVERLAY_SHARED_CAMERA_STATE_F6_F8_REMEDIATION_PLAN.md).

- **F6 — CLOSED, contract documented + regression:** transaction owner means the owner context of one FOV publication transaction; the snapshot's `presentation.state` remains the last full-publication presentation; the semantic coordinator is current lifecycle metadata. The publication field was renamed `transactionOwner`, the diagnostic emits `transaction_owner`, `coordinator`, and `last_full_publication_state`, and expanded Camera State distinguishes the latter two. No ownership transition changed.
- **F7 — CLOSED, preview contract clarified + regression:** the Overlay's viewport/policy calculation is explicitly a `Cinematic aspect preview`, not production resolution or store confirmation. Production display/native fallbacks and store refusal remain outside this projection. No camera aspect behavior changed.
- **F8 — CLOSED, presentation contract clarified + regression:** summary is live-only and requires `writerObservation=Current`; details retain a valid last-observed writer/policy aspect and show the shared writer-observation freshness beside it. No fact disagrees: it is current in one view and explicitly historical in the other.
- **Validation:** all 58 registered harnesses compiled and executed; camera writer transaction, Camera Integration, snapshot, config persistence, feature presentation and semantic snapshot harnesses passed. `localization_catalog_audit.ps1` passed for 18 catalogs and embedded resources. `test.cmd` exits nonzero because `localization_harness` reports missing CJK text glyphs; the complete suite is not claimed PASS. `build.cmd` passed and emitted the production ASI (3,386,368 bytes; SHA-256 `0AC6E0A9880E1DED6F5E9BAD1936D5C6F9C3FA4BC42F49F028D673E5299D4212`). No game was launched.
- **Deferred:** F9 consolidation and F10 menu/unload/reset lifecycle remain untouched. Runtime acceptance remains deferred until the later findings are integrated; no offline result establishes rendered Overlay or game behavior.

## F10 lifecycle characterization update (2026-10-07)

The bounded lifecycle pass and freshness-only remediation are recorded in
[the F10 lifecycle plan](../active/OVERLAY_SHARED_CAMERA_STATE_F10_LIFECYCLE_PLAN.md).

- **Confirmed source contract:** accepted Cinematic ENTER/EXIT and observed
  coordinator transitions are explicit camera presentation boundaries. Before
  this update, these paths marked presentation/transformed/Gameplay baseline
  freshness but left the prior `WriterObservation`, Native FOV and Current
  Output FOV `Current` until another publication. They now mark that prior
  writer group Historical; transaction and full writer publication remain the
  only events that refresh their respective live roles. Retained Gameplay FOV
  and HorPlus Target are preserved with their own freshness semantics.
- **Regression:** `camera_writer_transaction_harness` exercises a presentation
  boundary with no new sample, a Cinematic FOV transaction, Dialogue state
  transition, EXIT/recovery boundary, and first Gameplay publication after
  handoff. It asserts live FOV roles are Historical across an owner boundary,
  retained Gameplay baseline survives, independent Dialogue transitions do
  not retire transaction FOV, and new publications re-establish Current.
- **Still open — architectural risk / insufficient evidence:** no production
  Main Menu, save-load, world teardown, or session-generation observation was
  found in `src/`. The coordinator is initialized to Gameplay and semantic read
  marks its stored value valid; it cannot distinguish cold Main Menu from
  Gameplay without another authoritative signal. After Gameplay → Main Menu,
  camera facts and accepted Transition Evidence can persist because no
  authoritative reset event is available. This behavior was not confirmed in
  runtime. No silence timer, blanket clear, or guessed menu classifier was
  added.
- **Independent lifetimes:** viewport aspect remains an AtRead client-geometry
  observation; ENTER evidence is set on accepted ENTER, persists through EXIT,
  and is replaced by a later accepted ENTER. Neither is coupled to live writer
  FOV invalidation.
- **Validation:** `camera_writer_transaction_harness` PASS; locale catalog and
  embedded-resource audit PASS for 18 catalogs; `build.cmd` PASS, emitting
  `STALKER2CameraTweaks.asi` (3,387,904 bytes; SHA-256
  `688384001FDE50737D087B44A9EF6874312129D8B95D37ADC1E7073F9529A382`). All
  58 registered harnesses compiled and executed, but `test.cmd` exits nonzero
  because the localization harness reports missing CJK text glyphs. Full suite
  is therefore **NOT PASS**. No game launch or runtime validation was performed.
- **Disposition:** known camera-boundary freshness defect is **CLOSED offline**.
  Main Menu/session reset semantics remain **OPEN** pending an authoritative
  lifecycle observation and matching-artifact runtime characterization. F9
  consolidation remains deferred.
