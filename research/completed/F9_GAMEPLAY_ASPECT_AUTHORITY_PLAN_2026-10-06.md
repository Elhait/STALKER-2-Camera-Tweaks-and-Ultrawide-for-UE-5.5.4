# F9 Gameplay Aspect Authority — Investigation and Remediation Plan

Date: 2026-10-06
Status: **RuntimeCombine is integrated in the ordinary production source/build and the user-provided exact-hash runtime log plus visual report confirm one complete scenario: `Load @16:9 → viewport 3:1 → Gameplay → Cinematic ENTER → Cinematic EXIT → Gameplay`. The log confirms aspect sync/read-back `1.77778 → 3.0`, Gameplay FOV `90.6557 → 119.272`, matching Cinematic target, RuntimeCombine EXIT recovery holding `119.272` through native interpolation toward `90.6557`, recovery completion, and resumed Gameplay output `119.274`. Loaded runtime-tested mod SHA-256 was `BD7D96A668044C763EFBA23F55AD7840FA535714E22910B32B2A2BCEBA7A73BC`; game executable SHA-256 was `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`. The ordinary production build is `STALKER2CameraTweaks.asi`, SHA-256 `66D301551BAC31F041A6DCD744E0AF190A743526EEE817CA2B812DD88841EC75`; byte comparison shows it differs from the runtime-tested artifact only at two timestamp fields. The user reports the full sequence looked correct. This is runtime confirmation for that artifact/scenario only; mirrored load direction and a broader arbitrary-aspect matrix remain untested. Native engine projection semantics remain unknown.**

## Objective

Determine why the Gameplay writer's aspect field at `RSI + 0x254` remains at
the prior client ratio after runtime resolution changes, whether that field is
the correct authority for Gameplay HorPlus, and whether the game performs an
aspect-dependent FOV/projection transform before or after the Camera Tweaks
writer boundary. Only implement a correction after the evidence establishes
the intended aspect authority and rules out applying HorPlus twice.

## Evidence currently in scope

### Runtime-observed

- Exact game image reported for the supplied WinDbg run:
  `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
- At the validated Gameplay writer boundary, RSI was
  `0x22274f8f340`, RBX was `0x21ea86969c0`, and `XMM0` was `90.6557`.
- `RSI + 0x254` read as `2.3888888`; byte `RSI + 0x259` read as `0x04`.
- After client transitions to 16:9 (`2560x1440`) and 32:9 (`5120x1440`),
  the same source field still read `2.3888888`. A 4-byte hardware write
  breakpoint installed after the initial 16:10/21:9 transitions did not hit
  during those later transitions. This rules out only writes observed by that
  breakpoint during that bounded interval; it does not explain earlier writes
  or establish that the field is the render/projection authority.
- With Gameplay correction disabled, the sampled writer input remained
  `90.6557` at 16:9 and 32:9. This is an observation at the hook boundary, not
  proof that the value is authored FOV or that no other aspect-dependent
  transform exists upstream or downstream.
- The supplied production log reports the same writer identity and continued
  callbacks/publications while client ratios changed. HorPlus continues to use
  the writer-observed `2.38889` aspect and emits the corresponding transformed
  FOV. Runtime output/projection correctness is not independently read back by
  that log.

### Source-proven

- Production Gameplay HorPlus reads the writer's live aspect/flags through
  `CameraWriterEngineOperations`; pending restoration/mode transitions can
  write those fields before the policy read. The steady-state HorPlus path
  itself reads the aspect; it does not refresh it from viewport geometry.
- `ReadClientViewportAspect()` measures process-window client geometry. It is
  already used for presentation and cinematic Auto behavior, but source does
  not establish that it equals the active camera projection aspect.
- Aspect writes exist in explicit Gameplay restoration/recalculation,
  recovery/handoff, and cinematic store paths. Their targets and semantics
  must be distinguished; a shared numeric offset does not prove a shared
  object or authority.
- The current deterministic characterization allows viewport ratio to change
  without a writer callback and expects the writer-aspect HorPlus result to
  remain unchanged. It characterizes existing behavior; it does not establish
  correctness.

## Constraints and non-goals

- Do not add `cameraPresent`, `inGame`, timeout/frame-age heuristics, or a new
  engine identity.
- Do not infer render/projection authority from client-window dimensions,
  `identityEpoch`, writer presence, pointer stability, or numerical FOV
  similarity alone.
- Do not change camera policy until the evidence gate below is met. Keep
  Gameplay, Cinematic, Dialogue, recovery, and established F1–F8 semantics
  unchanged unless direct evidence requires a separately reviewed contract
  change.
- Do not launch the game, modify game memory, add runtime hooks, or run broad
  executable analysis. Use the already analyzed exact-image 2.0.6 material if
  accessible; do not re-import or re-analyze the executable.

## Work sequence and gates

### Policy-aspect validation addendum (2026-10-06)

The user explicitly stopped further reconstruction of native `RSI + 0x254`
semantics for this policy question. That field remains an independently
observed native camera fact, not a proven authority for the mod's Gameplay
HorPlus calculation. The candidate path therefore captures a validated
current-process client viewport aspect for the Gameplay HorPlus policy only;
it does not write viewport data to native camera memory or alter native flags.

The production-equivalent characterization sends the same writer/source and
`90.6557` input through `ProcessCameraWriter` with policy aspects `16:9`,
`21:9`, and `32:9`, while native writer aspect remains `32:9`. Expected
outputs are approximately `90.66`, `107.3`, and `127.4` respectively. Invalid
viewport must pass through. Gameplay OFF and AspectRecalculation remain
unchanged. Recovery authorization continues to use the existing native writer
source/aspect/flags; only the authorized HorPlus recovery target calculation
uses the current validated policy aspect. Camera State exposes native writer
aspect separately from policy aspect/provenance.

Passing offline tests/build authorizes only an internal candidate artifact.
It does not establish that client viewport ratio is native projection
authority or prove the runtime correction is visually correct. Runtime
validation remains the controlled same-writer sequence `16:9 → 21:9 → 32:9
→ 16:9`, with native writer aspect recorded independently. Do not launch the
game as part of offline validation.

**Policy candidate gate:** full harness, localization/resource/glyph audits,
production build, diff/whitespace audit, and internal artifact hash pass;
no native camera writes or unrelated mode/ownership semantics change.

### Runtime A/B evidence — current-viewport-only candidate falsified (2026-10-06)

The supplied `STALKER2CameraTweaks.log` identifies the loaded mod artifact as
SHA-256
`8191486C23386FB6D54C8483B3AB42B8CEBC31A9CE0E7C22599A77C9BBC0AAE6`,
matching the internal policy-aspect candidate. The game image is
`61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.

The log and accompanying runtime observations capture two opposite
load-save/resize states:

| Setup | Native writer aspect | Client/policy aspect | Writer FOV | Candidate output | Runtime observation |
| --- | ---: | ---: | ---: | ---: | --- |
| Load save at 16:9, then change viewport to 32:9 | `1.77778` | `3.55556` | `90.6557` | `127.393`, applied | User reports visibly excessive widening. |
| Load save at 32:9, then change viewport to 16:9 | `3.55556` | `1.77778` | `90.6557` | `90.6557`, bypass | User reports framing remains wrong; Gameplay OFF→ON does not repair it. |

Relevant production traces occur around `20:33:37` and `20:46:51` in the
supplied runtime log. They establish that the candidate uses the changing
client aspect and that the native writer aspect can remain unchanged. The
visual correctness statements above are user-provided runtime observations;
the log itself records the production calculation, not a final projection
readback.

**Verdict:** the characterization correctly validated the candidate's
calculation, but runtime evidence falsifies `HorPlus(writerFov,
currentViewportAspect)` as a safe universal drop-in replacement for the
previous native-writer-aspect policy. The phrase “stale writer aspect” is not
established: current evidence supports only that `RSI+0x254` is a native
aspect-like value with a lifecycle different from client viewport geometry.
Its exact engine meaning remains unknown. Writer FOV `90.6557` is still not
proven to be authored, horizontal/vertical, reference-space, already adjusted,
or final projection FOV.

The two opposite outcomes suggest the policy may need the FOV's source/reference
space as well as the destination viewport, but this remains a hypothesis—not
authorization to implement `viewport/nativeAspect`, another formula, or a
strength multiplier. Preserve both load-save/resize sequences as paired
characterization cases for future evidence.

#### Cinematic-boundary constraint

User-provided historical runtime evidence states that actual aspect/FOV state
changes at Cinematic ENTER/EXIT produce visible FOV jumps. Avoiding those
jumps was a reason for adopting a shared mathematical HorPlus approach for
Gameplay and Cinematics. Consequently, reintroducing native aspect/FOV
recalculation at those boundaries is not an acceptable generic remedy. No
safe way to invoke that native recalculation across Cinematic boundaries has
been established.

#### Recovery trace limitation

The supplied log contains `coordinator=CinematicExiting` entries with
`gameplayPolicyAspect=nan` and reason
`POST_CINEMATIC_RECOVERY_INTERPOLATION`. This field is taken from the top-level
writer result; the recovery branch does not currently propagate its policy
aspect into that result. Therefore those trace lines do not prove that the
recovery calculation received an invalid viewport aspect. Treat recovery
aspect as unverified by this telemetry until a targeted observation is added
or existing output/state evidence resolves it. This is a diagnostics gap, not
by itself evidence of a production correction defect.

**Candidate disposition:** retain the built ASI only as a historical internal
candidate/evidence artifact; do not describe it as a runtime fix or promote it
as the accepted Gameplay policy. No production behavior or F1–F8 semantics are
changed by this finding.

### Historical production-path reconstruction — aspect was reread, not explicitly cached

Inspection of the committed pre-F9 production path (`ReplayManualTransition` /
`ReplayManualTransitionOriginal` in `src/plugin/runtime.cpp`, at repository
`HEAD`) corrects the shorthand “the old code cached Load Save aspect”:

- For each ordinary Gameplay writer callback, production safely read the
  current source object's `+0x254` aspect and `+0x259` flags, then passed that
  freshly read aspect to `gameplay::EvaluateHorPlus(inputFov, aspect, flags,
  kNativeAspect)`. There was no dedicated `Load Save`/initialization aspect
  cache in this HorPlus decision path.
- The old code therefore followed the *live native writer field*, not the
  client viewport. In the observed session that field persisted across resize,
  making the effective policy input behave like a load/save reference aspect;
  source alone does not establish that Load Save is the event that authored or
  initialized the field.
- Under the old policy, if the field remained 16:9 after resizing to 32:9,
  `EvaluateHorPlus` would treat 16:9 as non-ultrawide and pass the writer FOV
  through. If it remained 32:9 after resizing to 16:9, the old policy would
  continue producing the 32:9 HorPlus transform. Those are source-derived
  policy consequences. Whether either path yielded correct *visible* framing
  is user-reported runtime evidence, not established by this source trace or
  by the logged writer output.
- The F9 candidate changed the policy authority at the validated writer
  boundary: it supplies a separately sampled, validated client viewport aspect
  to the HorPlus policy while retaining native writer aspect as an independent
  observation. The paired runtime results reject that viewport-only candidate
  as a universal drop-in; they do not yet prove a replacement formula.

**What this establishes:** the useful historical invariant is “HorPlus consumed
the writer's native aspect field on every callback, and that field could remain
stable while the viewport changed,” not “the mod captured a Load Save aspect.”
This distinction preserves the demonstrated behavior without inventing a
cache lifecycle. The user's report that the prior behavior framed correctly
is strong historical runtime evidence, but the mechanism that made the final
projection correct remains unknown. In particular, it does not by itself
prove native upstream/downstream HorPlus, nor authorize `HorPlus(FOV,
referenceAspect, viewportAspect)`.

**Next bounded offline question:** compare the committed pre-F9 writer path,
the F9 candidate path and the current non-candidate production path to identify
exactly which policy inputs/state were replaced, including the existing
Gameplay AspectRecalculation and restore transitions. Preserve the two
opposite load/resize cases as paired runtime characterization for a later
controlled matrix; do not launch the game or infer projection semantics from
the old policy's apparent success.

#### Policy-authority contract candidate (not yet promoted to production)

The user-provided historical interpretation is consistent with the committed
source trace: the legacy Gameplay HorPlus policy used `nativeWriterAspect`
read from the current callback's writer object on every invocation, while
`currentViewportAspect` is an independently observed window fact. The native
field may remain stable across a resize and therefore behave reference-like in
that runtime session. Do not call it `startupAspect`, `loadAspect`, or
`projectionAspect`; exact engine semantics remain unknown.

This is a strong candidate for restoring the legacy policy authority while
preserving F9's separate viewport telemetry, but the stronger claim that the
legacy policy produced correct visible framing in *both* mirrored
load/resize cases is not source-proven. Promote the following as a production
invariant only after the historical runtime evidence is tied to the exact
pre-F9 artifact and both visual outcomes are confirmed:

```text
HorPlus policy input = nativeWriterAspect read for this writer callback
currentViewportAspect remains an independent observation
viewport resize alone does not invalidate/replace nativeWriterAspect
Gameplay HorPlus does not write nativeWriterAspect from viewport geometry
```

The desired eventual paired characterization is:

```text
nativeWriterAspect=16:9, viewport=32:9 → policy uses 16:9; viewport reports 32:9
nativeWriterAspect=32:9, viewport=16:9 → policy uses 32:9; viewport reports 16:9
```

These cases establish which aspect the mod supplies to its policy, not final
projection correctness. Keep the user's visual/runtime evidence separate from
the deterministic policy-output assertions. Do not reintroduce the
current-viewport-only candidate, synchronize the native field, or add a
two-aspect formula based on these observations alone.

#### Follow-up: user-facing aspect-space explanation (after policy validation)

Once the Gameplay HorPlus authority is accepted, revise Camera State,
Camera Integration terminology, tooltips and user documentation so they do not
present the native writer aspect and current viewport as contradictory values
or imply that the HorPlus result is necessarily the final visible horizontal
FOV. Prefer separate labels such as `Reference Aspect` and `Current Viewport`
for user-facing text; keep precise internal provenance names such as
`nativeWriterAspect` and `currentViewportAspect` in diagnostics.

Do not yet state that Reference Aspect is definitively “the aspect ratio in
which this save was loaded.” The currently supportable wording is that it is
the native aspect observed from the Gameplay camera state and that it may
remain different from the current viewport after a runtime resize. Explain
that a difference is not by itself an error only after the legacy policy's
runtime behavior is revalidated. Do not add a warning or “conversion active”
status unless the accepted policy provides evidence for that claim.

This is a deferred presentation/documentation task, not authorization to alter
camera semantics or Overlay behavior during the current authority investigation.

### Native-writer-authority candidate implementation (2026-10-06)

The current-viewport-only policy substitution has been reversed in an internal
candidate. No claim is made that this candidate is runtime-correct.

**Production path:** `ReplayManualTransition` still obtains the validated
writer fields through the existing engine-operation boundary. For Gameplay
HorPlus, `ProcessGameplayHorPlusWriter` uses the fresh `RSI+0x254` value
returned as `result.aspect` for both `ResolveGameplayWriterPolicy` and the
HorPlus result. It does not cache an aspect at Load Save and does not write
viewport geometry into the native object. The separately sampled field is now
named `currentViewportAspect` in the transaction input and is observation-only.
The existing AspectRecalculation path and native-field restoration/write
semantics were not changed by this rollback.

**Recovery:** post-cinematic Gameplay target calculation now uses the fresh
native writer aspect/flags read for the authorized recovery sample, as in the
legacy authority model. It no longer consumes the viewport observation as its
HorPlus aspect. Cinematic lifecycle/owner/continuation decisions were not
changed.

**Telemetry/presentation:** the writer observation carries three distinguishable
facts: native writer aspect, HorPlus policy aspect (with
`NativeWriterAspect` provenance), and current viewport aspect (with
`ClientViewportAspect` provenance). Camera State exposes the viewport using
the existing localized viewport label; Camera Integration keeps native writer,
policy, and viewport observations separate. Diagnostic writer trace records
native aspect, HorPlus policy aspect, viewport aspect, writer FOV input, output,
and applied/eligible decision. The UI has not received the deferred full
terminology redesign.

**Production-equivalent characterization:** the writer transaction scenario
now exercises divergent pairs, including native `16:9` with viewport `32:9`
(no ultrawide transform) and native `32:9` with viewport `16:9` (32:9 policy
transform). It asserts both aspects/provenance remain distinct, the native
fixture field remains unchanged, invalid viewport telemetry does not replace
or disable native policy authority, and Gameplay OFF remains pass-through.
The former cinematic harness helper that filled a missing policy aspect from
the fixture's native aspect was removed. Recovery assertions now verify that
the native writer aspect—not viewport telemetry—determines its Gameplay target
and that recovery still works with unavailable viewport telemetry. Existing
AspectRecalculation and Cinematic ownership tests remain part of the suite.

**Offline validation:** the registry found 58 harnesses and all 58 executables
were run. 55 passed. Three harnesses outside the F9-edited source paths remain
failing. Their relation to other pre-existing working-tree changes is not
established; they are reported rather than suppressed:

- `config_persistence_harness`: migration/default-template/documentation and
  hotkey persistence assertions fail; staging/replacement failure-preservation
  checks pass. This F9 change did not touch its source or configuration path.
- `placement_config_harness`: concurrent INI writers lose setting/X/Y in all
  24 iterations; save and migration checks also fail. This F9 change did not
  touch placement/config persistence code.
- `localization_harness`: assertion fails for missing glyph `U+0148`. The
  localization catalog/resource audit itself passes for all 18 locales and
  188 keys; this F9 change added no localization key or catalog edit.

Production build succeeded with a separate internal artifact:
`STALKER2CameraTweaks_F9NativeWriterAspectCandidate.asi`, SHA-256
`6443919D92F00845508D575106B0BB3F2CD4895E69653AA48F7A19F38FC65B13`.
The candidate was not launched in game. `git diff --check` could not run in the
current Git invocation (`fatal: this operation must be run in a work tree`)
despite `git status` and `git worktree list` succeeding. A targeted trailing-
whitespace scan over the 11 F9-scoped production, test, and plan files passed;
this does not replace the unavailable Git diff check.

**Runtime gate (updated by the user-supplied evidence below):** both mirrored
pure-Gameplay resize cases have now been reported as visually correct on this
artifact. This does not validate Cinematic transitions, establish native
projection semantics, or make the candidate a generally validated fix. The
remaining runtime acceptance case is the Gameplay-to-Cinematic transition
after a resize, described in the later evidence section.

### Runtime follow-up — Gameplay resize passes; Cinematic ENTER divergence (2026-10-06, user-supplied)

This section records the user's report and quoted runtime measurements. The
visual outcomes are user-observed; the available text does not independently
establish final projection readback or prove the native meaning of
`RSI+0x254`. The tested candidate is identified as
`STALKER2CameraTweaks_F9NativeWriterAspectCandidate.asi`, SHA-256
`6443919D92F00845508D575106B0BB3F2CD4895E69653AA48F7A19F38FC65B13`.

**Reported pure-Gameplay resize cases:**

| Session setup and resize | Current viewport aspect | Native writer aspect | Writer FOV input | HorPlus output | Reported visible result |
|---|---:|---:|---:|---:|---|
| Load at 32:9; resize 32:9 → 21:9 → 16:9 | `3.55556 → 2.38889 → 1.77778` | remains `3.55556` | about `90.6557` | about `127.393` throughout | Correct framing at each viewport |
| Load at 16:9; resize 16:9 → 21:9 → 32:9 | `1.77778 → 2.38889 → 3.55556` | remains `1.77778` | about `90.6557` | native/bypass, about `90.6557` | Correct framing at each viewport |

These are user-reported runtime/visual results for the named candidate, not
independent verification by this assistant. They support the candidate's
native-writer-aspect behavior for **pure Gameplay resize** and further reject
using current viewport alone as the Gameplay HorPlus authority. They do not
prove that the HorPlus output is the final visible FOV or explain why either
reported framing is correct.

**Separately reported Cinematic ENTER case:**

- Session/save loaded at 16:9, then viewport changed to 32:9; Gameplay framing
  was reported correct before Cinematic ENTER.
- At ENTER, the reported state was: Gameplay baseline aspect `1.77778`, latest
  pre-ENTER native writer aspect `3.55556`, current/effective Cinematic aspect
  `3.55556`, and baseline native FOV about `90.6557`.
- The reported Cinematic target/output was about `127.393` with
  `transform_applied=1`; the user reports the visible FOV defect returns at
  ENTER.

The reported values show a cross-domain disagreement at the transition:
Gameplay baseline provenance refers to `1.77778`, while the latest writer
and Cinematic context use `3.55556`. This is a **runtime observation and
causal lead**, not yet a code-proven defect mechanism. In particular, the
numbers alone do not prove that the baseline aspect causes the visible error
or that synchronizing it would be safe.

#### Causal trace against the available runtime log and current source

The current `STALKER2CameraTweaks.log` is for the candidate hash above. Its
entries distinguish the first ENTER in this chain from later repeated ENTERs;
the three-value divergence in the request corresponds to a later ENTER, not
the first one. The log gives the following sequence:

1. The live Gameplay writer was first observed at RSI `0x1b1a5155a40`, aspect
   `1.77778`, flags `0x4`, and input FOV `120` while the separately observed
   viewport was already `3.55556` (`22:11:44.593`).
2. At `22:11:55.382`, the same RSI was observed at aspect `3.55556`, flags
   `0x4`, input FOV `90.6557`; the Gameplay HorPlus output was `127.393`. The
   aspect had therefore changed before any logged Cinematic ENTER. The log
   does not identify the writer that changed it: this hook records the new
   value at its callback boundary, not the native store instruction/caller.
3. The writer identity changed at `22:14:20.715` to RSI
   `0x1b1ef679a00`, initially aspect `1.77778`, flags `0x4`. The later
   viewport observations changed to `2.38889` (`22:16:20.606`) and `3.55556`
   (`22:17:02.206`), while the sampled writer aspect remained `1.77778`.
4. The first ENTER (`22:19:09.150`) captured a Gameplay baseline and latest
   writer snapshot both at sequence `26672`, aspect `1.77778`, input FOV
   `90.6557`, flags `0x4`. It then selected that baseline and produced
   `127.393` for effective Cinematic aspect `3.55556`. Only after this
   selection/ENTER did the Cinematic aspect-store log write aspect `3.55556`
   to object `0x1b1ef679a00`; the first post-ENTER writer observation then
   reported aspect `3.55556`, flags `0x5`.
5. At the later ENTER (`22:19:28.364`), the pre-ENTER writer snapshot had
   advanced to sequence `27152` and reported the same RSI, aspect `3.55556`,
   flags `0x5`, and input FOV `127.393`. Yet the stored Gameplay baseline
   remained at sequence `26672`, aspect `1.77778`, native input FOV
   `90.6557`, with no valid HorPlus-result pair. The transaction again chose
   `GameplayBaseline` and emitted target `127.393`. Subsequent ENTER records
   repeat this retained baseline with later writer sequence numbers.

Source explains why these values have different lifetimes:

- `GameplayBaselineStore::Project` stores the writer observation's native
  register input FOV, result only when it is a mod-transformed result, aspect,
  source and publication sequence. It accepts a projection only when the
  caller marks the observation Gameplay-eligible and the result is a valid
  transformed result or pass-through. Its eligibility does not mean “latest
  writer sample regardless of coordinator/flags.”
- The Gameplay HorPlus writer path projects baseline observations with
  `coordinator == Gameplay`. Writer samples during `CinematicActive` or
  `CinematicExiting` do not refresh this Gameplay baseline. The Cinematic
  aspect-store directly mutates the associated object's aspect, but does not
  itself publish a Gameplay baseline observation.
- On accepted Cinematic ENTER, `TraceCinematicEnter` reads the retained
  `GameplayBaselineStore` snapshot and passes it to
  `ProcessAcceptedCinematicEnter`. That transaction selects the stored
  `nativeFov` when GameplayHorPlus is selected and the baseline is usable; it
  does not replace the snapshot's aspect/FOV with the separately captured
  latest writer observation. The latter is diagnostic context, not a baseline
  selection input.

The code-level selection fact is confirmed: ENTER uses the stored eligible
Gameplay baseline (`1.77778`) rather than substituting the later
`3.55556 / flags=0x5 / FOV=127.393` writer observation. This is not, by itself,
a defect or a proposed fix: vanilla Cinematic framing is 16:9-oriented, and
the established Cinematic correction path translates that camera intent into
the selected runtime aspect/FOV domain. The writer sample with flags `0x5`
and FOV `127.393` may already represent corrected Cinematic-related state;
it must not be treated as a neutral Gameplay baseline. The relevant question
is whether Gameplay has already been brought into a compatible target domain
before the boundary, not whether ENTER should blindly replace its baseline
with the newest writer observation.

The log also answers when the writer aspect changed, but not who caused every
change. There are two observed transitions: the first RSI was already
`3.55556` at a Gameplay callback before Cinematic ENTER, with no logged
producer; the later RSI remained `1.77778` across viewport resize and changed
to `3.55556` at the first Cinematic aspect-store after ENTER. On repeated
ENTERs that object already reports `3.55556`. The remaining candidate is not
“why didn't baseline become the latest writer?” but whether Gameplay and
Cinematics reached that shared numeric aspect through compatible camera/FOV
semantics. A matching aspect number alone does not prove matching camera
domains.

**Updated four-way verdict:** **(4) another causal model remains under test:
Gameplay/Cinematic aspect-domain transition mismatch.** The retained-baseline
selection is explained by source and is not the target defect model. A
Gameplay synchronization defect is plausible for the pre-ENTER state where
Gameplay remains at `1.77778` after the viewport becomes `3.55556`, but is not
yet proven to cause the observed jump. The exact latest-writer values at
`3.55556 / flags=0x5 / FOV=127.393` are compatible with already-corrected
Cinematic-related state and do not establish current neutral Gameplay
authority. Thus neither (1) baseline-selection defect nor (3) both is
established; (2) remains a candidate causal defect, and (4) is the current
evidence classification until a production-equivalent boundary reproduction
links the domain transition to the visible result.

**RuntimeCombine stays only an architectural hypothesis.** It is the Gameplay
analogue of the existing Cinematic correction concept, not a literal reuse of
the legacy Gameplay `AspectRecalculation` policy. Its proposed target model is:

```text
detect actual validated runtime target aspect
→ if Gameplay camera aspect already equals target, do not correct aspect
→ otherwise synchronize aspect directly to target
→ confirm synchronization
→ only then run HorPlus
```

The intended aspect is an arbitrary validated runtime float, not a preset
ratio. Aspect mutation is conditional on an actual mismatch; the legacy
forced `16:9 → target` round-trip is excluded. The ordering invariant is
**aspect correction first, then HorPlus FOV correction, and only after the
aspect has been confirmed**. The existing Cinematic aspect-store and Gameplay
`AspectRecalculation` paths are evidence for identifying the minimum
production-equivalent transaction, not proof that a direct float store alone
is sufficient. No implementation is authorized. First establish which
Gameplay and Cinematic domains are active before and after each boundary
operation and reproduce the claimed visible jump through existing production
transactions.

If/when implementation is separately authorized, `RuntimeCombine` must be a
first-class third Gameplay mode in the same change—not an INI-only or hidden
experimental switch. Its vertical slice includes config parsing/name/cycle and
persistence, runtime dispatch and mode-transition handling, Overlay selector
and user-facing tooltip, localization in every supported locale, and
production-equivalent tests. The UI wording may describe automatic aspect
synchronization before Hor+ correction, but must not promise final visible FOV
semantics that remain unproven.

**Current evidence classification:**

- **Reported runtime observation:** native writer aspect is stable through
  resize in both mirrored cases; the candidate's pure-Gameplay visible result
  is reported correct; the Cinematic ENTER defect is reported for the
  16:9-load/32:9-current sequence.
- **Supported bounded conclusion:** the candidate's native-writer authority
  behaves acceptably in the reported pure-Gameplay resize matrix. The earlier
  viewport-only policy candidate remains rejected.
- **Hypothesis only:** Gameplay may remain in a native `1.77778 / ~90.66`
  domain after Load @16:9 and viewport resize to 32:9, even while native
  projection makes pure-Gameplay framing appear correct. At ENTER the mod's
  Cinematic path may translate vanilla 16:9 intent into the runtime 32:9
  aspect/HorPlus domain, producing a visible discontinuity at the boundary.
  This is not yet causally established.
- **Architectural hypothesis only:** a `RuntimeCombine` Gameplay path could
  detect the actual validated runtime aspect, synchronize only on mismatch,
  confirm the aspect transition, then apply HorPlus—without a 16:9 round-trip
  or fixed-ratio presets. The existing Cinematic and Gameplay
  AspectRecalculation paths must be analyzed to define the minimum
  production-equivalent transaction; direct `ApplyAspectStore` is not assumed
  sufficient. No production implementation is authorized.
- **Still unknown:** exact semantics/authority of `RSI+0x254`, whether the
  quoted Gameplay baseline is the causal input to the Cinematic target, the
  final visible projection/FOV contract, and whether any candidate transition
  would duplicate native or mod HorPlus work.

**Next bounded investigation:** model the actual cross-domain transition, not
a “latest writer replaces baseline” policy. Trace the existing Gameplay
AspectRecalculation native writes/restoration and the Cinematic aspect-store
transaction, including callback/event ordering and how HorPlus FOV is
selected/applied on each path. Establish the Gameplay camera's pre-ENTER
aspect/FOV domain after each resize and the Cinematic path's post-correction
domain; distinguish a matching aspect scalar from matching semantics. Build
the production-equivalent acceptance sequence:

```text
Load @16:9 → viewport 32:9
→ Gameplay aspect synchronizes to 32:9
→ Gameplay HorPlus runs in the 32:9 domain
→ Cinematic ENTER is corrected into the same 32:9/HorPlus domain
→ no visible FOV jump
```

Add the mirror `Load @32:9 → viewport 16:9` as a negative/control scenario.
The transaction must assert event order and state after each step, especially
that aspect correction precedes HorPlus and is skipped when already at target.
Use arbitrary validated aspect values, not only 16:9/21:9/32:9. Do not
implement the candidate until existing production paths demonstrate the
minimum safe aspect synchronization transaction and the scenario reproduces
the boundary behavior. Keep the baseline store's established lifecycle
semantics unchanged unless transaction evidence independently proves them
wrong.

### Existing-path comparison (source audit, 2026-10-07)

The two current aspect mechanisms have materially different contracts:

| Path | Trigger/guard | Mutation and FOV behavior | What it establishes—and does not establish |
|---|---|---|---|
| Cinematic aspect-store (`ApplyCinematicAspectStore` → `ObserveCinematicAspectStore` / `cinematics::ApplyAspectStore`) | A specific native Cinematic store hook; association is accepted only for a nonzero target object, valid Cinematic selection, `CinematicActive`, and not stopping. Correction additionally requires the hook, runtime enablement, and active correction gate. | Replaces the native fixed-16:9 aspect store with a validated aspect-only write to the associated Cinematic object. In inactive correction mode it writes the 16:9 native aspect. It does not write flags or itself apply HorPlus FOV. Cinematic FOV correction is a separate ENTER transaction. | Demonstrates a production aspect-store primitive at a known Cinematic boundary. It does not prove that the same direct write on a Gameplay receiver performs the necessary Gameplay synchronization, nor that the aspect store precedes the ENTER FOV decision. Current hook ordering must be preserved/verified rather than assumed to satisfy RuntimeCombine's desired order. |
| Gameplay `AspectRecalculation` (`ProcessGameplayAspectRecalculationWriter`) | Runs only when Gameplay owns the coordinator and the selected Gameplay mode is AspectRecalculation; uses validated writer reads and existing replay/restoration state. | Returns incoming FOV unchanged. Its existing state machine writes native 16:9 with flags `0x5` for constrained-ultrawide normalization, or native 16:9 with flags `0x4` at the automatic-update restoration point; it can re-arm after replay completion. | Demonstrates that Gameplay has explicit aspect+flags mutation and restoration semantics tied to writer events. It is not an arbitrary runtime-target synchronizer: it is built around a 16:9 restore/automatic-update sequence, and it provides no evidence that aspect-only store is sufficient for RuntimeCombine. |

The production writer operations expose safe read, aspect-only write, and
aspect-plus-flags write boundaries. Their existence makes offline execution of
the decision/state transaction feasible, but does not establish engine-side
effects of either write. In particular, the Cinematic hook replaces a native
store at a known Cinematic instruction boundary; invoking the same primitive
from a Gameplay writer callback would be a new placement/semantic choice, not
an already-proven equivalent operation. The established Gameplay
AspectRecalculation state machine must remain unchanged and must not be
silently reused as RuntimeCombine.

**Minimum candidate transaction to characterize (not approved for
implementation):**

```text
Gameplay coordinator + RuntimeCombine selected
→ capture a valid current runtime/viewport target aspect
→ safely read the current Gameplay receiver aspect/flags
→ if already at target, record “already synchronized” and do not mutate
→ otherwise perform only a production-validated aspect synchronization
→ read back/observe confirmation of the target domain
→ only after confirmation, evaluate/apply HorPlus using that target domain
→ publish result and provenance; otherwise do not apply an unsafe correction
```

The comparison does **not** yet identify the production-validated
synchronization operation or prove that readback alone confirms semantic
synchronization. That is the remaining design/evidence gate: characterize
whether the existing Gameplay aspect+flags transition has required side
effects, and whether a direct aspect-only write at a safe Gameplay event
boundary is sufficient. Until then the transaction above is a testable
contract proposal, not a production implementation specification. Keep
Gameplay OFF pass-through, AspectRecalculation, Cinematic/Dialogue ownership,
and recovery/handoff behavior as independent controls; no native aspect or
flags mutation is authorized by this source comparison.

### 1. Finish the current-code provenance audit — in progress

Trace every production writer/read/store of `RSI + 0x254` and `+0x259`,
including settings transitions, Gameplay AspectRecalculation, recovery,
cinematic aspect-store, and callback ordering. Confirm which operations can
target the live Gameplay receiver and which are only same-offset operations
on another object. Record exact source locations and whether each path runs in
the ordinary production profile.

**Gate:** a complete bounded writer inventory and the exact production order
from validated hook input through engine reads/writes to HorPlus output.

### 2. Resolve the aspect-field semantic contract

Inspect already-available exact-image 2.0.6 analysis and validated writer
instruction/call context. Determine whether `RSI + 0x254` is a source input,
cached client ratio, camera constraint, or another field; do not infer its
meaning from offset, flags, stable values, or the symbol name used by the mod.
If current exact-image evidence cannot establish the contract, document the
specific missing observation rather than substitute viewport ratio.

**Gate:** evidence that identifies the authoritative aspect for the Gameplay
HorPlus policy, or an explicit `unknown` finding with the minimum additional
observation required.

### 3. Assess single- versus double-HorPlus behavior

Reconstruct the bounded path:

`configured/native camera intent → upstream camera transform → writer XMM0 →
Gameplay HorPlus → writer output → downstream projection/FOV use`.

Use only exact-image evidence already available. Distinguish the writer input,
the value written by Camera Tweaks, and final visible projection. If static
evidence is insufficient, specify the smallest behavior-preserving runtime
capture that would discriminate the cases; do not claim either case from
numeric resemblance or from equal XMM0 values across the tested ratios.

**Gate:** double HorPlus is confirmed/excluded, or the verdict remains
`possible, insufficient evidence` with the exact missing link recorded.

### 4. Build production-equivalent characterization before policy change

If the source contract identifies a correctness defect, extend the existing
production transaction scenario to represent writer aspect, viewport change,
settings state, and actual policy output. Preserve a characterization result
for the currently observed behavior until the desired contract is justified.
Do not make a test-only authority setter or a separate policy model.

**Gate:** deterministic reproduction traverses the same production decision
path as the hook and demonstrates the established defect without assuming
viewport aspect is authoritative.

### 5. Minimal remediation, conditional on the evidence gates

Only after Steps 2–4 establish the authority and the single-transform
contract, implement the smallest production change at the owning boundary.
Preserve fail-closed behavior, source provenance, native pass-through and
settings/lifecycle transitions. Add negative coverage for unresolved/invalid
aspect and for unchanged viewport/writer aspect. If the authority remains
unknown, do not implement a guessed fix; record the bounded evidence gap.

## Validation and handoff

- Preserve all existing working-tree changes; this investigation is not
  authorization to clean, reset, or reformat unrelated files.
- Before any production edit, recheck the current diff and the applicable test
  and build entry points. Run only validation authorized for the eventual
  implementation; report the exact source/artifact/runtime scope.
- Never attribute output correctness to a Camera State snapshot alone. A
  production HorPlus result is not independent proof of final visible FOV.
- Final report must separate runtime observation, source-proven facts,
  exact-image evidence, inference, and unknowns; state whether remediation was
  justified and list any remaining runtime-only proof.

## Current next action

### Step 1 result — bounded production field inventory

- `ReplayManualTransition` safely reads the live writer receiver's `+0x254`
  and `+0x259`, then `ProcessCameraWriter` performs ownership/lifecycle
  routing. In Gameplay HorPlus, `ProcessGameplayHorPlusWriter` obtains a fresh
  read through `CameraWriterEngineOperations`; it evaluates `input FOV ×
  fresh writer aspect`, and the adapter commits the returned result to XMM0.
  Neither the ordinary HorPlus path nor the Overlay client-ratio reader writes
  the writer aspect in response to a resize.
- Production transaction paths that can write this field are event/state
  specific: pending HorPlus mode restoration, Gameplay enable/disable
  restoration, and Gameplay AspectRecalculation normalization/restoration.
  They write a saved/native aspect or `kNativeAspect`, not a continuously
  sampled client-window ratio. The atomic EXIT handoff writes native aspect
  when its existing recovery conditions authorize it.
- The cinematic aspect-store hook writes `targetObject + 0x254`; its target is
  derived from the cinematic setter's RAX, and code does not establish that it
  is the active Gameplay writer RSI. Its configured/Auto cinematic aspect
  contract is separate.
- Additional direct aspect writes found in `ReplayManualTransitionOriginal`,
  `ApplyGameplayAspectFixAtomic` and the one-shot normalization path are inside
  compile-gated diagnostic/research code, not the ordinary production writer
  policy. The combined diagnostic direct write is likewise conditional.
- The supported-build manifest validates the writer store
  `MOVSS [RBX+0x30], XMM0` and documents RSI as source, RBX as output, with
  source offsets `+0x254/+0x259`. This does not identify the native writer of
  the aspect property or prove that it must follow client dimensions.
- The original `CheckWriterAspectViewportCharacterization` was a stale-behavior
  characterization: it held writer aspect at `3.0` while varying viewport.
  It has now been replaced for Gameplay HorPlus policy validation by the
  production-transaction characterization in the addendum above. Native aspect
  remains independent and is not written from the viewport.

**Step 1 conclusion:** the runtime divergence is not caused by an Overlay
cache: the legacy correction consumed the live writer field. The internal
rollback candidate again supplies the fresh native writer aspect to Gameplay
HorPlus; validated client viewport remains an independent observation and is
never written into native camera state. Native engine projection authority
remains unknown and outside this policy validation. The inventory is bounded
to the current Gameplay writer, engine operations, cinematic aspect-store and
explicit test/diagnostic paths.

### Step 2 exact-image project/export check

The local executable SHA-256 was recomputed and matches the target image:
`61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
The installed Ghidra project has an existing read-only application-log export
for `/Stalker2-Win64-Shipping.exe (2.0.6)`. Its script output independently
records SHA-256
`61bc1e030740cebc30cf1dad0c86cf65e39e12ff0500225821d684181e08d56b`, image
base `0x140000000`, and a unique writer-signature hit. This validates the
export's exact-image identity; no 2.0.4/2.0.5 observations are used below.

The 2.0.6 disassembly export identifies:

- containing function `FUN_140a9f74c` (`0x140a9f74c`–`0x140aa06cd`);
- at `0x140a9f7bb`, `RBX = R8`; at `0x140a9f7be`, `RSI = RCX`;
- at `0x140a9f8b9`, `MOVSS XMM0,[RSI+0x230]`; then at `0x140a9f8cb`,
  `MOVSS [RBX+0x30],XMM0` on the non-`[RSI+0x262]&0x10` branch;
- immediately afterward, `MOVSS XMM0,[RSI+0x254]` and
  `MOVSS [RBX+0x5c],XMM0`;
- byte `[RSI+0x259]` is copied into output metadata at `[RBX+0x68]` by
  separate bit-mask operations. Bits `0x01` and `0x04` are copied
  independently; the decompilation also tests bit `0x02` in a later branch.
  This establishes use/copy behavior, not field names or higher-level meaning.
- one direct caller is recorded at callsite `0x1453a6ce8` in
  `FUN_1453a6cc6`.

Therefore, at this exact writer boundary, the FOV written to the output
structure is loaded from the receiver's `+0x230` field, while `+0x254` is
copied as a separate float into output `+0x5c`. The export does not establish
that `+0x230` is authored FOV, that `+0x254` equals current client/viewport
aspect, or how the output structure is consumed after the caller receives it.

The first follow-up export confirms the writer boundary but not the native
producer or downstream projection authority. A later bounded query attempt
against the same existing project used `-readOnly -noanalysis` and the explicit
saved JDK 23 path. The headless wrapper remained active without emitting
stdout/stderr or advancing the Ghidra application/script logs; the owned
invocation was cancelled and its process tree checked/cleaned. No Java/Ghidra
process or project lock file remained, and the analyzed project was not
modified. This is an invocation/access failure before new script evidence,
not evidence that the static paths are exhausted or that the remaining facts
are runtime-only. Continue static work through the existing database/export
and restore the bounded read-only query path; do not infer a lock cause from
the symptom alone.

**Step 2 remains open.** Continue resolving the runtime object's class/vtable
and the `[*param_1+0x638]` target. Trace the passed output record's `+0x30`,
`+0x5c`, and flag consumers toward any projection boundary, then join exact
receiver provenance for `+0x230/+0x254/+0x259`. Until both directions meet,
aspect authority and double-HorPlus remain unknown.

### Step 2/3 follow-up — exact-image caller and bounded candidate audit

The existing analyzed project was queried read-only using `-readOnly -noanalysis`;
the image identity gate again passed for SHA-256
`61bc1e030740cebc30cf1dad0c86cf65e39e12ff0500225821d684181e08d56b`.
No import or analysis was performed. Research outputs are:

- `02-Research/Ghidra/reports/gameplay-writer-206-targeted-20261006.log`
- `02-Research/Ghidra/reports/gameplay-writer-206-downstream-20261006.log`
- `02-Research/Ghidra/reports/gameplay-writer-206-fields-exact-20261006.log`

Exact-image writer and caller findings:

- In `FUN_140a9f74c`, the ordinary branch loads `[RSI+0x230]` into XMM0,
  stores that FOV-like value to `[RBX+0x30]`, then copies `[RSI+0x254]` to
  `[RBX+0x5c]`. The nearby byte `[RSI+0x259]` is copied bitwise to output
  metadata. This proves a structured copy, not the high-level meanings of the
  fields.
- `FUN_1453a6cc6` calls this writer at `0x1453a6ce8`; immediately afterward it
  calls an unresolved virtual target through `[*param_1+0x638]`, passing the
  same `param_3` output-record pointer. It then writes additional values at
  `param_3+0x950/+0x958` using fields including `param_1+0x25c`, `+0xbe4`,
  `+0xc0c`, `+0xbe8`, and `+0xbec`. This establishes order and dataflow into
  the indirect call, but not that the output record is a projection or that
  these later calculations are the final visible FOV.
- `FUN_1453a6898` stores its float argument to `[param_1+0x230]` and derives a
  second value using `tanf(argument * constants)` and other object state. A
  code reference near `FUN_1453a6cc6` is in an `.rdata`
  reflection/registration region, not a proven vtable; the nearby
  `0x14b47e...` references are `.pdata` unwind records, not a function-pointer
  table. Thus this setter is a relevant field-family candidate, but there is
  no call/object-identity evidence joining its receiver to the observed
  runtime Gameplay writer instance.
- Other exact displacement candidates are not identity proof. For example,
  `FUN_140a73518` copies `[param_1+0x250]` to `[param_1+0x254]`, while large
  routines `FUN_140a2dc14` and `FUN_140a30b46` write `+0x254` through nested
  members. `FUN_140acda84` writes constant `0x459c4000` to its `+0x254`.
  Their objects are not established as the live Gameplay receiver. The
  constructor `FUN_1400c14a7` zeroes a block spanning `+0x254`; that is
  initialization, not evidence of a resize producer.
- A broad displacement search yields many unrelated uses of the same numeric
  offsets (including stack slots and different base registers). Such matches
  cannot be treated as a global field xref set or used to assert a producer
  for the runtime RSI. No proof was found that client-window aspect writes the
  live Gameplay receiver during resize.

The output-record lifetime and downstream consumer remain unresolved: the
virtual target at slot `+0x638` has not been resolved for the relevant runtime
object, and no projection/final-FOV consumer of output `+0x30/+0x5c` is
code-proven. Likewise, the upstream origin of `[RSI+0x230]` remains unknown.

**Step 2 result:** `RSI+0x254` is demonstrably copied alongside the FOV-like
field, but its authority is **unknown**. Client viewport is not established
as its replacement. The candidate setter and same-offset routines do not
close producer provenance.

**Step 3 result:** XMM0 at the hook is directly loaded from `[RSI+0x230]`.
This does not prove whether that value was aspect-transformed upstream. The
post-writer virtual target and downstream FOV/projection path are unresolved.
Therefore verdict is **possible, insufficient evidence**; double HorPlus is
neither confirmed nor excluded. Numerical similarity to the configured FOV
is not used as evidence.

Minimum evidence still required by the plan:

1. Resolve the relevant runtime object's virtual function table and the target
   of `[*param_1+0x638]`; trace consumption of output `+0x30`, `+0x5c`, and
   copied flags.
2. Establish a code-proven path from a producer to the exact live receiver
   passed as RSI, including writes/derivation of `+0x230`, `+0x254`, and
   relevant `+0x259` state. Same offsets or nearby table membership alone do
   not meet this bar.
3. If the existing static database cannot resolve dynamic object identity or
   the indirect target, the minimum runtime evidence is a bounded read-only
   capture of that object's vtable and fields at the existing writer boundary,
   plus values at the resolved downstream consumer, across stable aspect
   settings with Gameplay correction enabled and disabled. No production
   hooks, memory writes, or game launch are authorized by this plan.

Steps 4–5 remain gated: preserve the characterization expectation and do not
change production policy until aspect authority and single-transform
semantics are established.

### Additional exact-image candidate findings

The completed read-only export `gameplay-writer-206-owner-callers-20261006.log`
also permits these narrower code-proven observations:

- `FUN_1432b9964` constructs an object whose first vptr is set to
  `PTR_FUN_148676480`. It initializes `+0x230/+0x234` with two `90.0f`
  values, `+0x254` with the float encoded by `0x3fe38e3b` (approximately
  1.77778), and masks `+0x259` to set bit `0x04`. This is constructor evidence
  for one field layout, not proof that runtime RSI is this exact class.
- `FUN_14333b566` constructs a parent/component pair. It creates a
  `CameraComponent`, stores it at parent `+0x2b8`, initializes that child at
  `+0x230`, `+0x254`, `+0x259`, and `+0x2d0`, and mirrors packed aspect/FOV
  state into parent `+0x2cc/+0x2d4`. This establishes a second concrete object
  family and parent-to-child path; identity with the runtime RSI is not
  established.
- `FUN_143db1262` conditionally copies parent-held state into
  `*(param_1+0x2b8)`: aspect from parent `+0x2cc` to child `+0x254`, FOV from
  `+0x2d0` to child `+0x230`, and one flag bit from `+0x2c8` into child
  `+0x259`. Its observed caller `FUN_1455859b8` obtains an opaque value from
  `FUN_142739ed2` and passes it as the second argument. The callee output and
  relation to runtime RSI remain unresolved.
- `FUN_146b64f58` and `FUN_146b7632e` write `+0x254` and flag bit 0 on the
  object at `param+0xf8`. Depending on settings/global selection, the aspect
  comes from a five-entry ratio table or from `FUN_1434df2f6()` followed by
  `FUN_143415562()` and width/height division. This is a real viewport-like
  aspect update path for that object family, but static evidence does not join
  its `+0xf8` object to the runtime RSI. The runtime hardware breakpoint's
  no-hit result remains bounded to its watch interval.
- `FUN_1466c360a` builds an output bundle from a camera-like object: it reads
  `+0x230`, `+0x254`, and `+0x259` and writes derived values/flag bytes into
  separate output records. `FUN_1466c3920` adds still more fields from offsets
  `+0xc7c/+0xc80/+0xc84/+0xbe4/+0xbe8/+0xc0c`. This proves additional
  transforms/copies for this path, not that it consumes the output record
  passed to the writer caller or that the data is the final projection.

The prior “nearby function-pointer table” wording is withdrawn. Neither the
reflection/registration region nor `.pdata` provided a valid vtable candidate
for the runtime `param_1`; the constructor vptrs above are legitimate table
starts but have not been joined to that runtime class or the caller's virtual
slot. Existing exports therefore do not resolve `[*param_1+0x638]` or the
downstream consumer of the writer output.

### Investigation execution rule

**Evidence gate is not a reporting gate.** An unresolved intermediate result
does not require stopping. Continue all remaining authorized static-analysis
work until the step's investigation space is exhausted or a concrete
runtime-only evidence boundary is demonstrated. Do not stop merely because a
candidate is unknown or an indirect target is unresolved; continue through
callers, constructors, vtables, RTTI, function tables, xrefs, initialization,
dataflow, and bounded related-function analysis. Only then classify the
remaining point as runtime-only, if appropriate.

### Current exact-image static verdict and remaining decision boundary

The available exports demonstrate a coherent *candidate* camera state
family: constructor initialization, parent-to-component copying, and a
settings-controlled aspect update route exist. However, no code-proven join
currently establishes that these are the particular live RSI object observed
at `FUN_140a9f74c`. Likewise, `FUN_1453a6cc6` passes the output pointer to an
indirect virtual method at object vtable offset `+0x638`; its dynamic target
and the record's later consumers are not included in the existing exports.
The script attempts made during this follow-up did not advance past the
headless launch boundary, so these are **not** yet proven static dead ends.

Accordingly, aspect authority remains **unknown** and double HorPlus remains
**possible, insufficient evidence**. No production policy change is
authorized. The later bounded read-only passes below supersede the earlier
statement that the static candidate paths had not yet been closed.

### Superseding exact-image follow-up — headless recovery and completed bounded static pass

The repeatable read-only workflow is now recorded in
`02-Research/Ghidra/run-gameplay-writer-206-followup.cmd` and the expanded
query is `02-Research/Ghidra/ghidra-scripts/AuditGameplayAspectAuthority206.java`.
It sets `JAVA_HOME=C:\Program Files\Java\jdk-23`, prepends that JDK's `bin`
directory to `PATH`, records `where java` and `java -version`, then invokes
the existing project with `-readOnly -noanalysis`. The successful runs used
root PIDs 17932, 30964, and 13628; all exited successfully, and a post-run process
check found no remaining `java`, `javaw` or `ghidra` process. The report
records JDK `23.0.2`, reaches `Processing read-only project file` for the
2.0.6 entry, passes the executable SHA gate, and ends with `AUDIT_COMPLETE`.
The successful invocation pins and verifies both `JAVA_HOME` and the child
`PATH` before launch. The local Ghidra `launch.bat` first probes `java.exe`
from `PATH`, then selects the valid explicit `JAVA_HOME` for its actual Java
command. The successful log proves this invocation used JDK 23 and proves
headless startup/project/script execution now work. The earlier hung
invocations produced no retained foreground output or exact environment
record, so their precise failure point cannot be reconstructed as fact; an
invocation-environment mismatch is plausible but not proven. No evidence
supports a surviving Ghidra process or project lock as the cause. The
successful log also shows the headless project traversal visiting other
program entries; the helper's exact SHA gate exits on each non-2.0.6 image
before doing the 2.0.6 query. No import or analysis was run, and the analyzed
database was opened read-only.

Reports:

- `02-Research/Ghidra/reports/gameplay-writer-206-followup-20261006.log`
- `02-Research/Ghidra/reports/gameplay-writer-206-followup-2-20261006.log`
- `02-Research/Ghidra/reports/gameplay-writer-206-followup-3-20261006.log`
- `02-Research/Ghidra/reports/gameplay-writer-206-followup-4-20261006.log`

New code-proven caller/vtable linkage:

- `FUN_1453a6cc6` is a direct caller of `FUN_140a9f74c` through the derived
  table `PTR_FUN_1484afbf0`. Its disassembly
  saves the input object from `RCX` to `RDI`, calls
  `FUN_1432b9784` (`0x1453a6cda`), restores `RCX=RDI`, then calls the writer at
  `0x1453a6ce8` and finally calls `[*param_1+0x638]` at `0x1453a6cf9` with
  the same object and output pointer. Thus the refresh helper receives the
  same `param_1` object later passed as writer RSI on that derived-family
  path. This does not establish that every object reaching the shared writer
  uses that caller path.
- Constructor `FUN_1432b9438` installs vptr `PTR_FUN_1484afbf0`. That table's
  slot 197 (`+0x628`) is `FUN_1453a6cc6`, and slot 199 (`+0x638`) is
  `FUN_1453a6e2c`. The class-registration path names
  `/Script/CinematicCamera` / `CineCameraComponent` and supplies a callback
  pointer at `0x1432b942f`. That callback is instruction-confirmed as a thunk:
  `MOV RCX,qword ptr [RCX]; JMP 0x1432b9438`, entering the constructor that
  installs the candidate vptr. The class-family, thunk, vtable installation,
  and slot mapping are code-proven. The later WinDbg capture instead resolves
  its runtime RSI to the separate base table `PTR_FUN_148676480`.
- `FUN_1453a6e2c` mutates view extras/flags and camera-internal fields; it
  does not read the earlier output FOV at `+0x30` or output aspect at `+0x5c`.
  Other executable calls through slot `+0x638` belong to other statically
  unresolved object families and are not treated as this target.

New code-proven per-call camera field update for the derived family only:

- On the `FUN_1453a6cc6` derived-family path, immediately before the writer,
  `FUN_1432b9784` updates the same object.
  It recomputes `[object+0x230]` by calling `FUN_1432b98c0`, then writes
  `[object+0x254]` from a calculation involving `[+0xbe4]`, `[+0xbe8]`, and
  `[+0xc0c]`; if `[+0xc78]` is positive, that value replaces the computed
  aspect. These stores occur on the direct native path before the writer copy,
  not only in a distant constructor or settings callback.
- `FUN_1432b98c0` computes the FOV-like field from camera lens/sensor-related
  fields using `atan`, focal-length-like state at `+0xc7c`, sensor-width-like
  state at `+0xbe4`, scale/constraint state and optional aspect override at
  `+0xc78`. Therefore `XMM0` at the hook is not proven to be an untouched
  authored scalar: immediately upstream, native code derives `+0x230` from
  camera state. This establishes a native camera FOV calculation, but does
  not establish that it is the same semantic HorPlus transform as the mod.
- `SetAspectRatio` reflection wrapper `FUN_145586334` writes `+0x254`;
  `SetConstraintAspectRatio` wrapper `FUN_1455864e8` writes the separate
  `+0x244`; `SetAspectRatioAxisConstraint` wrapper `FUN_1455863c8` writes the
  byte at `+0x258`. The copied byte at `+0x259` is distinct adjacent state.
  Constructor defaults include `+0x254 = 0x3fe38e3b` and `+0x259` bit `0x04`.
  These property names narrow the field contract to camera-component
  aspect/constraint state; they do not establish that it must equal the
  process client viewport ratio.
- Separate update routines `FUN_146b64f58` / `FUN_146b7632e` also update an
  object reached at manager `+0xf8`, selecting a configured ratio or client
  dimensions and changing bit 0 of `+0x259`. Their caller/object chain is not
  statically joined to the exact writer receiver above; these are a distinct
  candidate update path, not proof of a resize producer for runtime RSI.

Downstream and double-transform audit:

- `GetCameraView` reflection wrapper `FUN_1455860e4` allocates/parses an
  output record, invokes virtual slot `+0x628`, and returns through the
  reflection output. This is an API consumer, not proof of final renderer
  projection use.
- All 38 executable call sites of virtual `+0x628` and all 18 call sites of
  `+0x638` were inventoried from executable instruction bytes and matched to
  existing analyzed instructions/functions. Candidate view consumers such as
  `FUN_14573c77a` and `FUN_145dada2e` use view-derived transform/pose data;
  the bounded code does not prove they consume the writer record's `+0x30`,
  `+0x5c`, and flags together as a final perspective projection. The output
  pointer is passed to the resolved `+0x638` helper, which does not read those
  FOV/aspect members, then returned by the view API. No exact static path from
  that output record to final renderer projection was found in the bounded
  caller inventory.
- Native code does perform an upstream sensor/lens-to-FOV calculation before
  the Camera Tweaks hook. Whether that operation is equivalent to a prior
  HorPlus correction, whether downstream projection applies another
  aspect-dependent transformation, and whether mod HorPlus is consequently a
  double correction are still **unknown**. Verdict: **possible,
  insufficient evidence**; neither confirmed nor excluded.

Runtime object-family join and bounded resize observation:

- In the new WinDbg session the game image base was `0x7ff65c2e0000` and the
  live writer object's vptr was `0x7ff664956480`; subtracting the image base
  gives RVA `0x8676480`, exactly Ghidra table `0x148676480` at preferred base
  `0x140000000`. At this table slot 197 (`+0x628`) is
  `FUN_140a9f74c`, the hooked writer, and slot 199 (`+0x638`) is null. The
  table's data references point to constructor `FUN_1432b9964` and secondary
  initializer `FUN_14337fa14`; `FUN_1432b9438` calls the base constructor and
  then installs the distinct derived table `PTR_FUN_1484afbf0`.
- Therefore the captured runtime RSI belongs to the `PTR_FUN_148676480`
  family, not the derived family whose `FUN_1453a6cc6` path calls
  `FUN_1432b9784`. The earlier claim that this producer runs immediately
  before the captured runtime writer was incorrect and is superseded here.
- Runtime began at 32:9 with `+0x254=3.5555556`, `+0x230=90.655739`,
  `+0x259=0x04`. After client transitions through 21:9 (`3440x1440`) to
  16:9 (`2560x1440`), the same RSI `0x239a49b2740` still held those values
  at the next writer callback; `XMM0` remained `90.6557`.
- The conditional breakpoint on `FUN_1432b9784` with `RCX==saved RSI` did not
  hit during the observed interval. A 4-byte write watchpoint on this exact
  `RSI+0x254` also did not hit during the subsequent 32:9/21:9/16:9 changes.
  These are bounded observations, not proof that the field is never written;
  WinDbg thread/interval coverage still limits the negative result.
- The mismatch is now explained at the object-family boundary: the static
  per-call refresh belongs to a different derived object. The remaining
  question is which writer-family update path owns the captured object's
  aspect after construction/settings transitions. Existing candidates
  `FUN_146b64f58`/`FUN_146b7632e` write through a manager `+0xf8` pointer but
  have not yet been joined to this exact RSI.

Bounded Step 2/3 disposition:

- Step 2 static work has resolved the runtime object's exact vtable family and
  excluded `FUN_1432b9784` as its per-call writer refresh. Constructor defaults
  `+0x254` to `1.777777...`; a different manager path can assign configured or
  client-derived ratios, but its `manager+0xf8` object has not been proven to
  be the runtime RSI. The field is not established as process viewport
  authority. **Aspect authority: unknown.**
- Step 3 static work establishes a native upstream lens/sensor FOV derivation,
  but no final projection consumer or semantic equivalence to HorPlus.
  **Double HorPlus: possible, insufficient evidence.**
- The remaining gaps are the exact writer-family aspect updater/owner and
  downstream projection readback. The runtime object/vptr join is now proven;
  further work should follow only the base-family constructor, direct setters,
  and candidate manager `+0xf8` path, not unrelated same-offset references.
  No gameplay launch, memory write, hook, import, analysis or production edit
  was performed. Steps 4–5 remain gated; current characterization stays as-is.

## Authorized RuntimeCombine implementation candidate — 2026-10-07

The user has authorized an offline implementation candidate. This authorization
supersedes the earlier plan-only/no-code gate for this candidate; it does not
authorize runtime validation or establish that `RSI+0x254` is native viewport
authority. Existing `HorPlus` and `AspectRecalculation` semantics remain the
controls and must not be changed.

Candidate transaction contract:

1. In Gameplay coordinator with Gameplay enabled and mode `RuntimeCombine`, read
   the current writer aspect/flags and validated process client-viewport aspect.
2. If the writer aspect already matches the validated target within the
   existing aspect comparison tolerance, do not write it; treat the observed
   match as synchronized for this callback.
3. If it mismatches, issue only the existing safe aspect-only write operation;
   preserve the writer flags and immediately re-read aspect plus flags.
4. Apply HorPlus using the target aspect only when the read-back confirms the
   target. If target, source read, write, or confirmation is unavailable, leave
   FOV native for this callback (fail closed).
5. Preserve the native aspect field and flags as separate observations; do not
   synchronize flags, change AspectRecalculation, or alter Cinematic/Dialogue
   ownership/recovery/continuation policy.

The candidate includes the Gameplay mode enum/parser/name/cycle and persisted
configuration, Overlay selector/tooltip, localized strings in all 18 locale
catalogs, Camera State transaction telemetry, and production-equivalent tests.
Telemetry records before/target/after aspect, mismatch, write attempted/succeeded,
read-back validity, flags before/after, FOV input/output, confirmation, and
whether HorPlus was applied. Diagnostic Mode emits a change-deduplicated
`RUNTIME_COMBINE` line. Runtime behavior remains unvalidated until a separate
user-authorized game session; do not infer camera correctness from offline tests.

Required offline evidence before preparing the candidate ASI: full harness
suite, localization/resource/glyph audits, production build, and diff checks.
After producing the ASI, stop before launching the game and report its SHA-256
and the exact runtime matrix for the user to perform.

Candidate artifact prepared offline on 2026-10-07:

- `STALKER2CameraTweaksRuntimeCombineCandidate.asi`
- SHA-256: `41FFCC9C3B25667E56BEEAC048F6A0F40E670E9C4CC7AA454B28BF36CE7E922D`
- Production build: PASS.
- RuntimeCombine writer transaction, mode/parser, runtime-settings API, and
  presentation harnesses: PASS, including arbitrary `2.75` target, 16:9/21:9/
  32:9 policy cases, no-write-on-match, flags preservation, Gameplay OFF,
  invalid target, write failure, and successful-write/unconfirmed-readback
  fail-closed controls.
- Localization catalog/resource/glyph audit: PASS for 18 locales (191 keys).
- Full `test.cmd`: candidate-related harnesses and all builds passed, but the
  suite exits nonzero in the existing config persistence/template group
  (`config_persistence_harness`: broad failures across normal persistence,
  migration, template synchronization, and hotkey persistence). The failure
  reproduces independently in that harness; this change did not modify the
  underlying persistence implementation. Actual on-disk Gameplay.Mode
  round-trip therefore remains unverified by this run.
- `git diff --check` for unstaged changes is unavailable in this checkout:
  Git reports `fatal: this operation must be run in a work tree` although
  `git status` and `rev-parse` work. `git diff --cached --check` reports only
  existing trailing whitespace in the unrelated staged F1–F8 plan. A
  candidate-path whitespace scan found one pre-existing trailing space in
  `locales/uk.json` line 29, outside the RuntimeCombine additions; it was left
  untouched.
- No game/runtime validation was performed. Candidate is not declared a fix.

## RuntimeCombine first runtime session — Gameplay/ENTER pass, EXIT issue (2026-10-07)

Evidence subject:

- Runtime log: `E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\STALKER2CameraTweaks.log`.
- Loaded mod SHA-256: `41FFCC9C3B25667E56BEEAC048F6A0F40E670E9C4CC7AA454B28BF36CE7E922D`, matching the internal RuntimeCombine candidate above.
- Game SHA-256: `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
- User report from this run: started at 16:9, changed to 32:9; Gameplay looked correct and Cinematic ENTER was ideal, but EXIT was not.

### Confirmed RuntimeCombine Gameplay transaction and ENTER

- The process initially started with `Gameplay.Mode=HorPlus`; the log records a
  live mode transition `HorPlus -> RuntimeCombine` at `01:54:05.941`. Thus the
  successful RuntimeCombine observations below belong to the candidate after
  the user selected the new mode, not to startup configuration.
- Gameplay writer source `0x199c88b73c0` first used aspect `1.77778`, flags
  `0x4`, and FOV input `90.6557`. At `01:55:10.003`, the transaction observed
  target `3.55556`, detected mismatch, attempted and succeeded at an aspect-only
  write, read back `3.55556`, preserved flags `0x4`, and then applied
  HorPlus: `90.6557 -> 127.393`. A subsequent transaction confirmed the
  already-matching aspect without another write.
- At accepted Cinematic ENTER generation 1 (`01:55:42.646`), the pre-ENTER
  Gameplay writer observation was FOV `90.6557`, aspect `3.55556`, flags
  `0x4`. The selected Gameplay baseline was native `90.6557` and transformed
  `127.393`; the cinematic authored input was `90` and its selected output was
  `127.393`. The cinematic aspect store used `3.55556`. The first associated
  writer sample was transformed from `90.6557` to `127.393` by the active
  cinematic policy. This agrees with the user-reported ideal ENTER.

### Lifecycle chain and EXIT evidence

The observed lifecycle was not one simple ENTER/EXIT pair:

1. Generation 1 EXIT at `01:55:57.445` was followed by accepted ENTER generation
   2 about 4 ms later; it is not the final return to Gameplay.
2. Generation 2 EXIT at `01:55:58.711` was followed by generation 3 ENTER about
   4 ms later. Generation 3 exited about 3 ms after that with no associated
   writer/recovery owner (`CINEMATIC_RECOVERY_NO_OWNER`), then generation 4
   entered about 6 ms later. These are transient lifecycle events in the same
   chain, not evidence of a completed Gameplay return.
3. Generation 4 was associated to the same writer and held `127.393` through
   continuation samples. Its EXIT at `01:56:05.831` records
   `nativeTargetFov=90.6557`, snapshots a recovery-owner candidate from the
   cinematic association, invalidates the association, and clears a
   322-sample continuation trace. The textual EXIT diagnostic says Gameplay
   recovery was not owned/coordinator returned to Gameplay, while the immediate
   post-EXIT probe still reports `coordinator=CinematicExiting`; treat that
   diagnostic/state discrepancy as unresolved rather than assuming the text
   describes the committed post-transaction coordinator.
4. The first post-EXIT writer callback follows ~3 ms later. It has the same
   source, aspect `3.55556`, flags `0x4`, and input `127.393`, while its probe
   still reports `CinematicExiting`. Subsequent writer-input observations,
   still tagged `CinematicExiting`, descend from `127.359` through `90.6557`
   over roughly one second. The log records these as input/source observations;
   it does **not** record returned XMM0 for each of these callbacks.
5. No `RUNTIME_COMBINE` marker appears during the generation-4 EXIT interval.
   The next recorded RuntimeCombine policy transaction is much later at
   `01:56:27.867`: the viewport target is then `3.0`, the writer aspect is
   changed `3.55556 -> 3.0`, read-back confirms it, and FOV `90.6557` is
   transformed to `119.272`. This later transaction is not evidence that the
   EXIT interpolation itself was corrected.

### Source-supported causal path and remaining proof boundary

`ProcessCameraWriter` handles `CinematicExiting` before ordinary Gameplay
dispatch. The post-cinematic recovery branch has a dedicated transform path
only when `input.gameplayMode == HorPlus`. Other modes use the native-endpoint
path: until the native EXIT target is reached, they leave the callback's input
FOV unchanged; at the endpoint they complete recovery and return. Ordinary
Gameplay dispatch also explicitly returns without entering Gameplay policy
while the coordinator remains `CinematicExiting`.

Consequently, the log's RuntimeCombine mode plus the `CinematicExiting`
writer-input descent, absent RuntimeCombine policy telemetry, and later
Gameplay transaction are consistent with a concrete integration gap: the
cinematic EXIT transition moves toward native `90.6557`, while RuntimeCombine
does not re-establish the `127.393` Gameplay target during that recovery path.
This is the leading code-level explanation for the user-reported bad EXIT.
However, the runtime log does not capture returned XMM0 at each EXIT callback,
and the owner-snapshot record alone does not prove recovery-owner authority;
therefore the exact per-callback output and owner branch remain unconfirmed by
this artifact. The report is **runtime-observed input/state + source-proven
dispatch gap + user-reported visual failure**, not a complete runtime
output-level proof.

### Disposition

- RuntimeCombine aspect synchronization and Gameplay output at aspect `3.0`:
  confirmed by the later exact-hash runtime session recorded below.
- Cinematic ENTER target continuity at aspect `3.0`: confirmed by matching
  Gameplay target, accepted ENTER result, first associated writer output and
  the user's visual report.
- The previously observed EXIT/recovery gap is superseded by the later runtime
  evidence below: recovery output stayed at the transformed endpoint, completed
  at the native endpoint, and normal RuntimeCombine resumed in the same
  callback sequence.

## RuntimeCombine Cinematic EXIT recovery candidate — 2026-10-07

The user authorized a narrow recovery/handoff candidate after the runtime
Gameplay and ENTER pass. This supersedes the preceding no-fix disposition only
for RuntimeCombine's owned `CinematicExiting` path. It does not authorize a game
launch, change to HorPlus/AspectRecalculation policy, or a general recovery
redesign.

### Implemented transaction contract

- The owner- and writer-RSI-validated `CinematicExiting` transaction now routes
  enabled RuntimeCombine through the same safe aspect-only synchronization and
  immediate aspect/flags read-back used by ordinary Gameplay RuntimeCombine.
- Recovery interpolation uses the validated viewport target only after that
  synchronization confirms. It then reuses the existing source-bound
  HorPlus-recovery interpolation from cached transformed Cinematic endpoint to
  the RuntimeCombine Gameplay target. No FOV/flags/native-aspect write is
  added.
- When synchronization is unavailable or fails, a sample proven to lie on the
  owned native EXIT interpolation (including the cached Cinematic endpoint
  within the dedicated recovery-range epsilon) holds the already-confirmed
  Cinematic FOV and leaves the recovery owner/coordinator active. It does not
  evaluate HorPlus in an unconfirmed domain or complete the handoff. Samples
  outside the validated recovery segment retain native/pass-through behavior.
- At a confirmed native endpoint, existing recovery completion runs; the same
  callback falls through to ordinary Gameplay RuntimeCombine, which confirms
  the aspect and produces its normal HorPlus target before output commit.
- Gameplay disabled does not enter the RuntimeCombine sync/recovery transform;
  existing non-HorPlus EXIT handling remains in effect. HorPlus and
  AspectRecalculation recovery branches are unchanged.
- A float-boundary case at the first EXIT sample is covered locally: a
  source/flags/range-validated input within `recoveryInterpolationRangeEpsilon`
  of the cached transformed endpoint holds that endpoint without retiring
  recovery. The shared HorPlus recovery classifier and its thresholds are not
  changed.
- Added change-deduplicated Diagnostic Mode marker
  `CINEMATIC_EXIT_ROUTING`, reporting captured owner, effective post-EXIT
  coordinator and Gameplay mode. The always-written EXIT line also reports
  `coordinator_after`, `recovery_owner_captured`, and `gameplay_mode`; it no
  longer claims that ownership returned to Gameplay when next state is
  `CinematicExiting`.

### Production-equivalent evidence

`cinematic_lifecycle_transaction_harness` first had the desired EXIT
expectations added and failed against the old dispatch. After the implementation
it passes for:

1. first EXIT sample at approximately `127.393` retaining the confirmed target;
2. intermediate native sample `110` retaining target while recovery remains
   owned and coordinator stays `CinematicExiting`;
3. native endpoint `90.6557` completing recovery and producing the confirmed
   RuntimeCombine output `127.393` in that same callback;
4. aspect mismatch with unavailable write operation failing read-back closed,
   holding only the prior confirmed target, preserving aspect `2.38889` and
   flags `0x04`, and keeping recovery pending.

Existing `camera_writer_transaction_harness` and
`cinematic_lifecycle_transaction_harness` both pass. Runtime behavior remains
unvalidated; do not treat these offline scenarios as proof of the actual
callback XMM0 output or visible EXIT result in the game.

### Validation and artifact

- Test registry audit: 58 sources/58 compiled/58 registered and scheduled.
- Localization catalog/resource/glyph audit in both `test.cmd` and production
  build: PASS, 18 locales / 191 keys.
- `test.cmd`: exits nonzero at `config_persistence_harness`, whose existing
  broad persistence/template/hotkey cases fail while its atomic-preservation
  controls pass. The recovery change touches none of that code. A separate run
  of the compiled harness executables also found `localization_harness` missing
  glyph U+0148 and `placement_config_harness` persistence/load failures. These
  are outside the recovery patch; their baseline provenance is not established
  by this run and they are not claimed as passes.
- Production build with a distinct object directory and output name: PASS.
- Artifact: `STALKER2CameraTweaksRuntimeCombineRecoveryCandidate.asi`, SHA-256
  `BD7D96A668044C763EFBA23F55AD7840FA535714E22910B32B2A2BCEBA7A73BC`.
- `git diff --check` still fails in this checkout with
  `fatal: this operation must be run in a work tree`; repository status and
  `rev-parse --show-toplevel` remain available. The staged whitespace check on
  the touched paths produced no findings, and a direct trailing-whitespace scan
  of the three edited source/test files found none; this is not represented as
  a successful unstaged `git diff --check`.
- No runtime/game launch was performed. Candidate is prepared for a separately
  authorized runtime check; no runtime fix claim is made.

## RuntimeCombine integrated runtime acceptance — 2026-10-07

The user supplied a new runtime log and reported that the complete scenario
looked correct. This section supersedes the earlier candidate-only EXIT status;
it does not close the broader F9 investigation or establish untested ratios and
load directions.

### Artifact and scenario

- Loaded mod SHA-256:
  `BD7D96A668044C763EFBA23F55AD7840FA535714E22910B32B2A2BCEBA7A73BC`.
- Game executable SHA-256:
  `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
- Log SHA-256:
  `AE77A6C6BBF38192317326B82FDA0F0A345061C9089F9BAE31AA1C5E84A0CE42`.
- Scenario: `Load @16:9 → viewport 3:1 → Gameplay → Cinematic ENTER →
  Cinematic EXIT → Gameplay`.
- The process started with `Gameplay.Enabled=1` and
  `Gameplay.Mode=RuntimeCombine`.

### Runtime evidence

- At `02:51:16.622`, the same Gameplay writer source changed aspect from
  `1.77778` to validated target `3.0`. The write succeeded, flags remained
  `0x4`, and immediate read-back was valid and confirmed. The same transaction
  applied HorPlus to FOV `90.6557 → 119.272`.
- At Cinematic ENTER (`02:51:26.912`), the accepted target was `119.272`; the
  captured Gameplay baseline and effective Cinematic aspect were both in the
  aspect-`3.0` domain. The first post-ENTER writer retained that target.
- At EXIT (`02:52:08.877`), recovery owner capture succeeded and the coordinator
  correctly remained `CinematicExiting`. From `02:52:08.880`, the log records
  RuntimeCombine recovery transactions: writer input traversed from `119.272`
  toward native endpoint `90.6557`, while output remained `119.272` and the
  synchronized aspect/read-back stayed confirmed at `3.0`.
- At `02:52:09.866`, `CINEMATIC_RECOVERY_COMPLETED` was followed at the same
  timestamp by the Gameplay writer transaction: input `90.6575`, HorPlus
  output `119.274`, coordinator `Gameplay`, mode `RuntimeCombine`. The next
  writer callback returned `119.272` for input `90.6557`.
- The user reports that Gameplay, Cinematic ENTER and EXIT looked correct.
  This is user-observed visual evidence, distinct from the diagnostic log.

### Production build and evidence limits

- RuntimeCombine is in the ordinary production source graph, config/parser/
  persistence, overlay selector and Camera State diagnostics; no
  experimental-only compile gate is required.
- Ordinary `cmd /c build.cmd`: PASS. Canonical output
  `STALKER2CameraTweaks.asi`, 3,340,288 bytes, SHA-256
  `66D301551BAC31F041A6DCD744E0AF190A743526EEE817CA2B812DD88841EC75`.
  Compared with the runtime-tested artifact, the byte comparator reports only
  two differing timestamp fields at offsets `0x110` and `0x1FA494`; all other
  bytes match. The normal artifact was built but not installed or runtime-run.
- `test.cmd`: all 58 registered harness sources compiled, but the suite stopped
  at `config_persistence_harness`, which reports multiple persistence,
  migration, template and hotkey failures; its staging/replacement atomic-
  preservation controls pass. Baseline cause is not established here.
- Separately run relevant harnesses passed:
  `camera_writer_transaction_harness`,
  `cinematic_lifecycle_transaction_harness`, `gameplay_mode_harness`,
  `runtime_settings_api_harness`, `feature_presentation_harness`, and
  `camera_integration_harness`. `localization_harness` fails on missing text
  glyph `U+0148`; this remains distinct from the build-time localization audit,
  which passes for 18 locales / 191 keys, resource parity and embedded glyph
  coverage.
- `git diff --check` remains unavailable in this checkout and returns
  `fatal: this operation must be run in a work tree` despite working
  `git status`/`rev-parse`. A direct trailing-whitespace scan of the five
  documentation/evidence files changed in this promotion found no trailing
  whitespace.
- The canonical default remains `Gameplay.Mode=HorPlus`; making RuntimeCombine
  a normal selectable mode does not overwrite existing user settings.
- This establishes one integrated runtime scenario and exact artifact only.
  `Load @32:9 → viewport 16:9`, other arbitrary aspect values, broader
  resolution/recreation paths and all environment combinations are not
  established by this run.
- Native UE projection semantics remain unknown. This acceptance validates the
  mod's observed policy transaction and reported presentation, not a complete
  reverse-engineered projection model.
