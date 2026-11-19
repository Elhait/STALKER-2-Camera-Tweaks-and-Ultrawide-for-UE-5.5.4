# U2 post-copy observation implementation — 2026-11-15

## Changes

Added a diagnostic-only observer after the validated call to native helper `0x9CE0EE`, at its return continuation `0x9CD3F6` in the pinned game image. The resolver uses a unique byte signature around the call, validates the `CALL rel32`, requires the target to remain inside executable `.text`, and decodes the continuation as `CMP byte ptr [RSI+0x240], 0`. It does not hook or replace the helper, and it does not change FOV or recovery policy.

The observer correlates with the most recent primary writer transaction on the same thread and source/output addresses. It queues one `POST_PROCESS` event with:

- native source field `[camera+0x230]`;
- original XMM0 value arriving at the primary writer;
- writer source branch flags `[camera+0x262]` and, when bit `0x10` is set, the native addition `[camera+0x2D4]`;
- policy planned output and primary post-store readback from the existing transaction;
- helper modifier `[camera+0x25C]`;
- smoothing `[output+0x9D8]` before and after;
- output `[output+0x30]` before and after the helper;
- coordinator, writer context, policy and transaction identity.

Unreadable fields and uncorrelated observations are explicitly marked unavailable with a reason (`NO_PRIOR_PRIMARY_STORE`, thread/source/output mismatch, `NOT_LATEST_TRANSACTION`, unavailable primary readback, or duplicate callback). Summary counters separate correlated callbacks, identity mismatches, and incomplete post-helper readbacks. Queue drops remain tracked by the existing bounded queue counters. Logging stays on the existing background worker.

## Writer branch correction

The pinned writer sequence has two source branches. With bit `0x10` of `[RSI+0x262]` clear, XMM0 receives `[RSI+0x230]` directly. With the bit set, the game adds `[RSI+0x2D4]` before the same output store. The observer now records both raw native FOV and actual XMM0 writer input so the runtime data distinguishes these cases.

## Validation performed

- Targeted static scan against game SHA-256 `2bd3f115b959a8eee7e5c4b42b0142dd0025bd437e1014f92dfc60b9afee5479`: one post-helper signature match at call RVA `0x9CD3F1`, target RVA `0x9CE0EE`, return continuation RVA `0x9CD3F6`.
- Diagnostic Release ASI compiled in isolated directory `build-artifacts/cm/u2-postprocess-diagnostic-20261115`.
- No test harnesses or game runtime were run in this step.

## First runtime capture and log-volume repair

The user-provided log identifies the previously built observer artifact SHA-256
`CE99EAE8CB7F553ED207DD832C21B9CEC5C175CF13A38613C0B638EB66586A47` and game
image SHA-256 `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
The post-helper capability resolved and installed. Before the 16 MiB main-log
limit was reached, it recorded 4,702 correlated `POST_PROCESS` events with no
post-process identity mismatches or unavailable post-helper readbacks. All
recorded events were Gameplay / GameplayEligible, helper branch `Inactive`,
modifier 0, smoothing 0 before and after, and identical post-helper input and
output. The observed native/writer inputs were 90.3115 or 120 degrees; the
post-policy output was 127.119 or 147.796 degrees respectively. The writer's
`0x10` offset branch was not active in these samples. This confirms the
observer's path for Gameplay only; Dialogue/Cinematic and helper-active runtime
behavior remain unobserved in this capture.

The capture log SHA-256 is
`1C2D464DC288C3C9B35C38BC22C898A0C81547D5F8D8092773565DC94156CC63`.
It ended with `LOG_LIMIT_REACHED` at 16 MiB. Event counts at the cutoff were
4,703 BEFORE, 4,703 DECISION, 4,702 POST_STORE and 4,702 POST_PROCESS. There
was one queue contention drop by the last periodic summary; no queue-full or
worker-unavailable drops were reported. No shutdown summary was captured.

To keep a multi-mode capture inside the existing bounded main log, the worker
now emits one composite transaction summary at `POST_PROCESS` (plus explicit
unavailable records), instead of separately serializing BEFORE, DECISION and
POST_STORE lines that are already represented in the composite record. The
composite keeps the original policy decision reason, correlation status,
planned output, primary readback/status, and pre/post-helper observations.
This changes diagnostics serialization only; it does not change camera policy
or hook behavior. The ordinary Release ASI was rebuilt with diagnostics still
controlled by `[Diagnostics] Enabled` in the INI.

- Ordinary Release ASI:
  `build-artifacts/cm/u2-postprocess-production-20261115/bin/STALKER2CameraTweaks.asi`
- SHA-256: `D101064AE314C47A09C6927AEA0D1E6771571C9FA5233FA43413CED0F750635B`
- Size: 3,939,328 bytes.

## Follow-up runtime capture

The subsequent user-provided log loaded the ordinary ASI SHA-256
`D101064AE314C47A09C6927AEA0D1E6771571C9FA5233FA43413CED0F750635B`, with
`Diagnostics.Enabled=true`, and the user reported no crash. It recorded 2,600
transactions and 10,400 accepted queue events, with zero queue drops, zero
post-store mismatches, zero post-helper identity mismatches, and zero
unavailable post-helper readbacks. The log was 3,955,827 bytes and did not hit
the 16 MiB cap.

The ordered context runs establish this exercised sequence in the captured
writer path: initial Gameplay (524 samples), first Cinematic (734), then
Cinematic exit with 7 RecoveryOwned and 40 ProviderTransitionObserved samples;
one more provider sample followed in Gameplay. DialogueLifecycleObserved
accounted for 141 samples, followed by the second Cinematic (778), its exit
(44 RecoveryOwned samples), and Gameplay recovery. Further
TransitionAmbiguous samples followed. All 2,600 post-helper records were
correlated. In every sample the helper branch was inactive, modifier and
smoothing were zero, and pre/post-helper output matched. The writer's `0x10`
offset branch was clear, so native FOV equaled writer input in these samples.
This capture establishes helper inactivity for this specific path and run; it
does not establish helper-active behavior or rendered-view FOV.

The composite records showed `actualAfterValid=1` and primary readback equal to
planned output in all 2,600 transactions, but the serialized
`postStoreStatus=Unavailable` was inconsistent. The status computation was
reading the event's validity flag before copying that flag from the source
frame. It now computes status directly from the already validated source-frame
readback. No camera behavior changed.

- Refreshed ordinary Release ASI:
  `build-artifacts/cm/u2-postprocess-production-20261115/bin/STALKER2CameraTweaks.asi`
- SHA-256: `FA236C5F2C5C3D9982B9FAC4D70B3D0B3A7037C3FF54577A67159F7F4D5F096A`
- Size: 3,939,328 bytes.

## Corrected-status runtime capture

The latest supplied log identifies that refreshed ordinary ASI hash and the
pinned game image, with diagnostics enabled. It contains 5,217
`POST_PROCESS` summaries / transactions and 20,868 accepted queue events. The
last periodic summary reports zero queue-full/contention drops, zero worker
unavailability, zero primary readback mismatches, zero post-helper identity
mismatches, zero unavailable post-helper readbacks and no pending events. There
is no `LOG_LIMIT_REACHED` marker. The log ends at a periodic summary, not a
worker-stop summary.

The corrected `postStoreStatus=MatchedAtCorrelatedSite` is present for all
5,217 records, and actual primary readback equals planned output in each. All
records are `POST_COPY_CORRELATED`; the helper branch is `Inactive` throughout,
modifier is zero, smoothing remains unchanged, and helper input equals output.
The writer flag is `0x1` and native FOV equals writer input throughout. Thus
the helper is not the source of a FOV change in this run; its active branch
remains untested.

### Native `120°` interpretation check

A focused read-only scan of those 5,217 `POST_PROCESS` records was made to
check whether the repeated Gameplay `nativeCameraFov=120` could be explained
as a direct echo of this run's corrected output. The first captured transaction
for receiver `0x28d8b8b1a80` is transaction 1 at `01:59:37.581`: native field
and writer input are both `120°`, while the planned/actual output is
`147.796°`. The same timestamp's earlier `Camera mode` line reports
`cameraWorldFov(+0x230)=120`, `cameraFirstPersonFov(+0x234)=90`, and output FOV
before the correction as `90°`; the composite `POST_PROCESS` record's
`previousField`, however, is `147.796°`. Since these two output-before values
disagree, the legacy `Camera mode` value is not used here as authoritative
readback of the pre-store buffer. At that point the diagnostic summary had
reported zero writer transactions. The native field and writer input are
nevertheless observed as `120°` before the first policy correction recorded
by this capture. This does not establish how the game or any earlier
unobserved path produced it.

Across the 2,251 `GameplayEligible` records, native/writer input ranges from
`90.3115°` to `120°`, while the planned/actual output is above native input in
all 2,251 records. The full capture contains native values above `90°` in
4,965/5,217 records (maximum `126.87°`). Therefore `nativeFov > 90°` is not a
valid feedback detector for this dataset: `+0x230` is not equivalent to the
separate `+0x234` field logged at `90°`, and the log does not identify the
origin or game semantics of the difference.

This is useful evidence against a simple same-transaction or immediate
Gameplay echo of corrected output back into native `+0x230`: the first
observed native input is already `120°`, and subsequent Gameplay samples keep
native input at or below `120°` while corrected output is higher. It is not a
no-mod control and does not exclude delayed/indirect feedback, earlier
unobserved writes, other camera paths, or a native game transition. No
correction-disabled runtime comparison is present in this capture.

### Dialogue native FOV from the same capture

The 5,217-record capture has 205 records labeled
`writerContext=DialogueLifecycleObserved`, from `02:00:48.466` through
`02:00:51.330`. In all 205, `nativeCameraFov` / `inputFov` (the source
`camera+0x230`; writer offset branch clear) is between `80.1509°` and
`80.1891°`. The policy label is `Reduced` for every record, and all 205
decisions mutate the writer output. Planned and primary readback output range
from `118.589°` to `122.812°`; the post-helper observer reports no additional
change. The coordinator label remains `Gameplay` in these records, so this is
Dialogue-lifecycle writer-context evidence, not proof that every sample was
taken during a semantically active Dialogue scene.

Thus for the user's question, in this observed Dialogue-lifecycle interval
native `camera+0x230` was about `80.15°`, while the mod's output at the
instrumented writer was about `118.59°–122.81°`. These values do not establish
the final rendered-view FOV. The older `80.1444°–90.2849°` range cited in
other provenance notes belongs to a different capture and should not be
substituted for this 5,217-transaction run.

### FOV-correction-off control capture

The next supplied log is tied to the same ordinary ASI
`FA236C5F2C5C3D9982B9FAC4D70B3D0B3A7037C3FF54577A67159F7F4D5F096A` and game
image `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
Log SHA-256: `D4A20E41061223E5B5A298F7264173AA561C0B1F7FDAEF00364075505D0D9EBD`;
size 11,844,369 bytes. Diagnostics and both writer/post-helper observation
capabilities were available.

The startup configuration line still shows the saved settings
`RuntimeCombine` / Cinematics enabled / Dialogue Reduced. The effective
settings were then changed live before the first writer transaction:

- `02:51:37.634`: Cinematics changed `true -> false`; lifecycle observation
  was retained and camera corrections disabled.
- `02:51:50.751`: Gameplay changed `RuntimeCombine -> AspectRecalculation`.
- Before each of the two accepted Dialogue lifecycles, the callback reported
  `selectedPolicy=OFF`; the second start at `02:53:31.165` explicitly reports
  `activePolicy=UNSELECTED_OFF` and `correctionEnabledAtStart=false`.
- The first writer transaction did not occur until `02:52:07.256`, after
  Gameplay and Cinematic FOV correction had been disabled. All 8,923 captured
  transactions report `gameplayMode=AspectRecalculation` and
  `mutateFov=0`.

For all 8,923 transactions, `inputFov == plannedAfter == actualAfter` and
post-helper input equals post-helper output. All primary readbacks have
`MatchedAtCorrelatedSite`; there are no identity mismatches or unavailable
readbacks. The native writer offset branch stayed clear (`writerBranchFlags=0x1`),
so native `+0x230` equals the input used by the writer. One diagnostic queue
contention drop was recorded; queue-full and worker-unavailable drops are
zero. The final periodic summary reports `pending=0`, but there is no worker
shutdown summary.

Observed FOV values with FOV correction off:

- `GameplayEligible` writer context: 5,217 transactions across the log have
  input/output values `90.0073°–120°`. The user clarified that the first
  `120°` sample at `02:52:07.256` was recorded in the main menu, before loading
  the save, after changing the FOV value there. It is **not** an in-save
  Gameplay sample and is excluded from conclusions about Gameplay FOV in the
  save. A later sample reports `90.3115°` after writer aspect changed to
  `2.38889`, unchanged by the mod; the log has no explicit save-load marker to
  classify that sample independently. `GameplayEligible` is a writer
  coordinator/context label, not evidence that a save is loaded.
- DialogueLifecycleObserved: 583 transactions, native/writer input and output
  range `70°–90.2454°`, including lifecycle transitions. On the second
  confirmed Dialogue start, 211 subsequent writer samples descend from
  `90.1506°` to `70°`; the final samples remain at the native target `70°`.
  The callback explicitly logged Dialogue correction OFF. This provides a
  direct control observation of the uncorrected Dialogue endpoint in this
  pass.
- CinematicActive samples occurred while Cinematics was disabled; they are
  pass-through / `TransitionAmbiguous` observations, not evidence of active
  Cinematic correction behavior.

### Gameplay comparison boundary

For future Gameplay FOV comparisons, use the logged moment when the Gameplay
FOV correction actually becomes active as the beginning of the analyzed
Gameplay interval. Do not use earlier writer samples, including main-menu
samples, and do not substitute the broad `GameplayEligible` context label for
that activation event. Compare `camera+0x230` / `inputFov`, `plannedAfter`,
and `actualAfter` from that boundary onward. The current correction-off
capture has no Gameplay-correction activation event because Gameplay FOV
correction remained disabled; it therefore provides no corresponding
correction-off Gameplay interval. No new save-load detector or runtime hook is
required for this comparison.

### Two-pass Gameplay-mode comparison — 2026-11-15

The user reports that this session was deliberately split into two passes:
first Gameplay with only `AspectRecalculation`, then a return to the main menu,
activation of the current mod parameters, and a repeat run. The log starts
with `Gameplay.Mode=AspectRecalculation`, Cinematics and Dialogue disabled,
and diagnostics enabled. The transition to the second pass is independently
visible when the source trace first reports `RuntimeCombine` at `03:15:24.687`;
the log itself does not record the menu action, so that part of the chronology
is attributed to the user's report.

| Pass | Writer interval | Mode | Transactions | FOV mutations | Input = planned = actual |
|---|---|---|---:|---:|---|
| 1 | `03:13:50.624–03:14:53.403` | `AspectRecalculation` | 6,286 | 0 | all 6,286 |
| 2 | `03:15:24.736–03:16:31.722` (captured portion) | `RuntimeCombine` | 5,924 | 3,069 | all 5,924 readbacks matched |

The mode-specific Gameplay traces provide a useful paired sample. In pass 1,
at `03:13:59.006`, native `camera+0x230` / writer input is `90.3115°` and the
AspectRecalculation mode leaves the FOV unchanged (`eligible=false`,
`applied=false`). In pass 2, at `03:15:30.891`, the same native value
`90.3115°` is supplied to `RuntimeCombine`, which produces `127.119°`
(`eligible=true`, `applied=true`). This is direct evidence that the two
observed passes received the same native FOV at these samples, while only the
second applied the Gameplay FOV transform. This completes the requested
Gameplay A/B check for whether `RuntimeCombine` changed the paired native
input: it did not in these observed passes. Treat this practical runtime
question as closed; the same A/B pass does not claim to cover every frame or
every camera lifecycle.

The `120°` sample at `03:15:24.687` coincides with the first trace reporting
the switch to `RuntimeCombine` and precedes the paired `90.3115°` sample. Do
not use it as the paired in-save sample or infer save state from
`GameplayEligible`; the comparison above uses the matching `90.3115°`
samples. The first pass is the no-FOV-transform control; it is not a separate
ASI or a diagnostic-only build.

Across all 12,210 retained `POST_PROCESS` records in this log, all had
`MatchedAtCorrelatedSite`, all post-helper observations correlated, the helper
branch was `Inactive`, and there were zero readback mismatches or unavailable
readbacks. Three contention drops were reported; queue-full and
worker-unavailable drops were zero. The log ends at `LOG_LIMIT_REACHED`, so
these counts describe retained records, not an asserted complete session or
worker-shutdown flush.

Mode-specific values from the same two-pass capture:

| Mode/context | Pass A: no FOV mutation | Pass B: current correction | Coverage note |
|---|---|---|---|
| Gameplay | Paired sample `90.3115° -> 90.3115°` | Same native `90.3115° -> 127.119°` | Direct A/B pair. |
| Dialogue lifecycle | 266 pass-through samples: native/output `70°–90.2285°`; trajectory reaches `70°` | 83 mutated samples: native `80.1013°–90.2405°`, actual output `118.511°–127.062°` | Ranges include lifecycle/transition samples, not only steady Dialogue. |
| Cinematic-owned | No CinematicOwned samples; Cinematics were disabled in pass A | 135 mutated samples: native `70.0328°–126.834°`, actual output `108.951°–127.119°`; 2,456 additional pass-through samples at `126.860°–126.870°` | Pass B evidence only; no A/B pair for Cinematic. |

For every range above, the logged planned result matched the primary-store
readback. The earlier correction-enabled Dialogue capture separately recorded
stable native samples `80.1509°–80.1891°` with output `118.589°–122.812°`;
that is corroborating B-only evidence from another pass, not the A/B pair in
this table.

### Cross-capture Dialogue setter implication

Compared with the preceding correction-enabled capture, Dialogue writer input
was about `80.15°` while policy `Reduced` was active and writer output was
`118.59°–122.81°`; in this control capture, the active Dialogue reaches the
native target `70°` with no FOV mutation. The source implementation's
`ReducedTarget` maps a Gameplay baseline around `90.3°` to approximately
`80.15°`, and the Dialogue blend callback can replace the tick's XMM1 before
the camera's virtual setter. The derived setter stores that sample at
`camera+0x230` and updates `+0xC7C`, which the evaluator consumes.

Together, these two captures strongly support that the earlier `~80.15°`
native-camera value was influenced by the mod's Dialogue setter correction,
not merely a raw game target. This is distinct from `output+0x30` feedback.
The control log contains no setter-argument/receiver-pair event, and the prior
capture does not record receiver vtable identity, so exact runtime dispatch
through derived setter `0x5353CC8` remains unconfirmed. No FOV/recovery code
was changed for this control.

The ordered context sequence is: GameplayEligible (2,251), first
CinematicActive/CinematicOwned (1,246), cinematic exit (7 RecoveryOwned plus
59 ProviderTransitionObserved), Gameplay provider handoff (1),
DialogueLifecycleObserved (205), second CinematicActive/CinematicOwned (1,324),
cinematic exit/recovery (73), then Gameplay recovery and 50
TransitionAmbiguous samples. At the Dialogue-to-second-Cinematic writer
boundary, the final Dialogue output and first Cinematic output are both
`122.808`; provider/Dialogue boundary samples move `118.586 → 118.574 →
118.589` across adjacent transactions. This is evidence of writer-output
continuity for the instrumented path, not proof of rendered-view continuity.

The log shows the 16 MiB overflow repair and corrected readback-status field
working on this artifact. Frame-time impact is still not established; the
capture's `PERF_RUNTIME_SUMMARY` is not a diagnostic-worker/frame-time A/B.

## Limits

The log will establish whether this helper callsite was reached and whether its condition was expected active from the captured modifier. It still observes only this validated derived GetCameraView path. It does not establish final rendered FOV, other camera overrides, or global output-writer coverage. Runtime behavior and instrumentation cost remain to be measured in a controlled capture.
- Earlier observer ASI SHA-256: `CE99EAE8CB7F553ED207DD832C21B9CEC5C175CF13A38613C0B638EB66586A47` (3,938,816 bytes).

