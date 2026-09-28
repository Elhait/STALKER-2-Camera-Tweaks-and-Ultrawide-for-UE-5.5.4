# Testing And Research Summary

## Current unified mod — v2.0.1 working-tree candidate (not released)

The unified production mod is `STALKER2CameraTweaks.asi`, intended to replace
the older `STALKER2UltrawideFix.asi` and `STALKER2GameplayAspectFix.asi`. Do not
load old and new files together. The repository's `v2.0.0` tag is the previous
release baseline. The current working-tree candidate includes the v2.0.1
camera/recovery work and a production Overlay backend migration from game
DXGI/D3D12 interception to an independent D3D11/DirectComposition presenter.
These changes are not committed, tagged or released. The complete categorized comparison
is in [`research/reports/V2_0_0_TO_V2_0_1_DIFF_REPORT_2026-09-28.md`](research/reports/V2_0_0_TO_V2_0_1_DIFF_REPORT_2026-09-28.md).

The current default configuration is:

```ini
[Gameplay]
Enabled=true
Mode=HorPlus

[Cinematics]
AspectRatio=Auto
FovMode=GameplayHorPlus

[Dialogue]
Zoom=Adaptive

[Diagnostics]
Enabled=false

[Overlay]
ToggleKey=VK_2E
Language=Auto
FontSize=13

[Hotkeys]
Enabled=false
GameplayCycle=F9
CinematicCycle=F10
CinematicFovCycle=F11
DialogueCycle=F12
```

`Gameplay.Mode` selects `HorPlus` or `AspectRecalculation`; HorPlus uses the
Gameplay FOV selected in the game's settings and adapts it to the current
aspect ratio. Cinematics offer automatic/native/forced framing and
`GameplayHorPlus` or `NativeHorPlus` FOV policies. Dialogue zoom supports
`Native`, `Adaptive`, `Reduced` and `Disabled`. Optional cycling hotkeys are
disabled by default and take effect at the next applicable lifecycle.

The INI is created automatically beside the ASI when missing. `Auto` follows
the runtime camera aspect and responds to resolution changes during the same
session. Settings can also be changed through the in-game overlay: `Delete`
(default) toggles it, while `Esc` is consumed while the overlay is active and
dismisses it after child interactions have had priority. Overlay Toggle works
independently of `Hotkeys.Enabled`. The overlay includes runtime status,
localized selector documentation/examples, hotkey rebinding and a read-only
Camera State view. Its 18 embedded language catalogs and offline glyph/resource
coverage are audited; offline checks do not establish runtime rendering for
every glyph, Arabic shaping, bidirectional text or RTL layout.

## Current validation snapshot — 2026-09-29, production DComp candidate

- The current production candidate is the canonical root artifact
  `STALKER2CameraTweaks.asi`, SHA-256
  `DF02F40BDEDA42666D2395F9558FA53F2996AFBAB8BFA54D1BE32995068E57A1`
  (2,739,200 bytes). The supplied camera runtime log records this exact mod
  hash and game executable hash `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
- The production Overlay uses its own D3D11 device and DirectComposition
  surfaces. It has no Camera Tweaks DXGI factory, game Present/Present1,
  ResizeBuffers/ResizeBuffers1 hooks, game backbuffer ownership or old DXGI
  fallback. `dxgi.lib` remains only for D3D11/DirectComposition interop.
- In the supplied exact-hash runtime session, the Overlay log reports
  `dxgi_factory_hooks=0 swapchain_hooks=0`, DComp device/surface initialization,
  compositor-clock availability, first frame, DPI 120 (scale 1.25), Auto locale
  synchronization to `uk`, startup hint creation/lifetime and a committed hint
  frame. The log also records mouse-position application through the game-HWND
  event bridge. These observations establish that those paths ran in this
  session; the log shows surface generation 1 and does not by itself establish
  a production resize/device-loss cycle.
- The exact-hash camera log records aspect 3.0 and Gameplay FOV 140, with
  HorPlus output 155.657. After cinematic EXIT, source-owned native recovery
  resumes at the same transformed target; subsequent ADS/zoom FOV changes are
  processed in Gameplay. The user reported this extreme-aspect/high-FOV test
  was visually smooth. Recovery was nearly immediate, so this run does not
  exercise the longer transformed-transitional-FOV mapping introduced in the
  latest recovery change.
- The new post-cinematic interpolation mapping has deterministic regression
  coverage for the recorded 3440×1440 / native-FOV-90 sequence, continuity in
  HorPlus output space, native baseline preservation, recovery convergence,
  and subsequent gameplay/ADS changes. This specific interpolation branch is
  **offline-tested only**; runtime confirmation is still needed.
- User runtime acceptance reports native Frame Generation toggling working
  with the integrated DComp Overlay, and the DComp prototype separately passed
  FG OFF→ON→OFF→ON and several resolution/window-mode transitions. The supplied
  exact-hash camera/Overlay logs do not explicitly record an FG toggle, so that
  acceptance is not attributed to a particular hash from those logs and is not
  a universal third-party-wrapper compatibility claim.
- The last recorded full `test.cmd` run passed all 47/47 inventoried harnesses.
  Localization/resource/font/glyph audits passed for all 18 embedded catalogs
  and 182 canonical keys. Arabic shaping, bidirectional/RTL layout and runtime
  rendering of every glyph are not established by those offline checks.
- The last recorded production `build.cmd` and `git diff --check` passed for
  the candidate above. These results are build/offline evidence, not runtime
  evidence for unexercised paths.
- SDR/HDR visual parity remains OPEN. The production color contract is BGRA8
  UNORM with premultiplied alpha and source-alpha blending; no HDR/scRGB
  transform or SDR-white mapping is applied. User feedback says DPI/colors seem
  improved but was not conclusive; no color/theme change is claimed.
- The agent did not launch the game for this validation snapshot. Runtime
  records and user observations were supplied from the user's test session.

### Historical pre-migration validation snapshot — 2026-09-28

- Full `test.cmd`: PASS at that earlier point. The runner compiled and executed all 54/54 inventoried
  harness sources, including the added DXGI callback/table, renderer lifetime,
  optional Overlay startup, startup-journal and post-cinematic HorPlus recovery
  contracts. The recovery fixture covers both transitional-to-native and
  already-native first post-EXIT samples, followed by ADS and later FOV changes.
- Localization/resource/font/glyph audits: PASS for all 18 embedded catalogs
  and 182 canonical keys. These offline checks do not prove runtime visual
  rendering for every glyph, Arabic shaping, bidirectional text or RTL layout.
- Production `build.cmd`: PASS for that earlier build. The canonical root output
  `STALKER2CameraTweaks.asi` was rebuilt; SHA-256
  `6C76FF71D530A362B2B14F3FC1BE5EBD9D30D899495206FF885DC7258452347F`, size
  2,766,848 bytes. This ignored build artifact is not part of the Git diff.
- User-reported runtime evidence: the bootstrap-enabled production build with
  the full graphics stack ran successfully five consecutive times and the
  Overlay worked. That evidence predates the later startup-journal addition
  and is not attributed to this hash. A later build
  (`8BFA8524C80B2D736D1F7ADF0DBFD48874D76DCC16DA1E5DDA9070940591E445`)
  exposed the post-cinematic recovery-gate regression described below.
- The `E804ECEA...` candidate was current in the 2026-09-28 snapshot above; it
  is historical and is superseded by the `DF02F40B...` artifact recorded in the
  2026-09-29 snapshot. Runtime observations for one hash must not be attributed
  to another candidate without matching identity evidence.
- `git diff --check`: PASS for the reviewed changes; Git emitted line-ending
  normalization advisories, not whitespace errors.
- The latest supplied runtime logs exposed a resize recovery regression:
  input activation and visibility toggles occurred, but no first Overlay frame
  was recorded. The Present gate excluded the renderer while its lifecycle was
  `Resizing`, preventing the next-Present resource rebuild. The gate now admits
  both `Ready` and `Resizing`, while still excluding terminally disabled state;
  regression assertions cover those states. This rebuilt hash has not yet
  received runtime confirmation.
- The agent did not launch the game; the later runtime evidence above was
  supplied by the user. See the
  [v2.0.0 → v2.0.1 diff report](research/reports/V2_0_0_TO_V2_0_1_DIFF_REPORT_2026-09-28.md)
  for the full inventory, and the
  [factory-table bootstrap report](research/reports/DXGI_FACTORY_TABLE_BOOTSTRAP_IMPLEMENTATION_2026-09-28.md)
  for coverage assumptions and runtime evidence boundaries.

## Research progression

The project evolved through these bounded evidence phases:

```text
Gameplay camera-state discovery
→ gameplay writer identification
→ dynamic signature resolution
→ camera rebuild / re-arm lifecycle
→ cinematic lifecycle discovery
→ shared authoritative camera state
→ cinematic aspect-writer provenance
→ live cinematic FOV consumption boundary
→ Hor+ FOV feasibility
→ combined cinematic correction
→ unified gameplay/cinematic coordinator
→ post-cinematic handoff investigation
→ 21:9 gameplay regression analysis
→ dynamic runtime Auto aspect policy
→ GameData dialogue-FOV semantic audit
→ dialogue configuration/ownership and target-assignment audit
→ WIDEBOY runtime-boundary reference audit
→ historical-2.0.4 dialogue boundary discovery
→ dialogue live-sample lifecycle classification
→ Adaptive and optical half-strength feasibility
→ EXIT discontinuity diagnosis and recovery anchoring
→ production dialogue integration and hotkey/persistence validation
→ cross-patch dialogue resolver validation
→ post-cinematic atomic handoff investigation
→ physical FOV setter and native bypass investigation
→ native bypass deferred after bounded static/runtime research
→ production combined atomic integration
→ historical 0.4.0 production resolver-contract audit on Steam 2.0.2–2.0.4
→ production runtime validation on Steam 2.0.5
→ production runtime validation on Steam 2.0.6
→ v1.0.0 release preparation
→ v2.0 overlay integration and 18-locale support
→ bounded v2.0 architecture and performance repairs
→ v2.0.0 tagged baseline
→ Overlay/DXGI startup-crash A/B investigation
→ bounded Overlay/DXGI/D3D12 safety repairs and FS-01–FS-05 regression contracts
→ independent shared factory-table bootstrap with supplemental export hooks
→ production-safe startup journal; v2.0.1 working-tree candidate
```

The detailed historical plans are preserved in the
[`research archive`](research/), while the active follow-up work remains in
the [`backlog`](backlog/).

## Historical Steam 2.0.4 evidence

The following sections preserve evidence collected from the Steam 2.0.4
executable. They are historical research evidence, not a current runtime
validation basis. Steam 2.0.5 and 2.0.6 have separate production runtime
records; older-build runtime support is not claimed without separate runtime
validation. See the [authoritative build support and evidence matrix](docs/SUPPORTED_BUILD_MANIFEST.md)
for per-build scope and executable/artifact identities.

### Gameplay

- The gameplay camera writer is resolved through a unique executable `.text`
  signature and validated by decoding `MOVSS [RBX+0x30], XMM0`.
- The generalized ultrawide predicate accepted the observed aspect above native
  16:9 and preserved that source aspect during the historical two-pass
  correction. v1.0.0 production uses the validated atomic apply instead.
- 21:9 startup, manual `21:9 → 16:9 → 21:9`, death/load camera rebuild and
  32:9 regression were user-tested successfully.
- The player's selected FOV is preserved.
- The separate weapon/viewmodel FOV issue after loading on 21:9 is a known
  game-side problem and is outside this fix.

### Native gameplay aspect reevaluation control

- A read-only UE4SS automatic dump captured the same live `CameraComponent`,
  `Stalker2.CameraManager` ownership references and `PlayerCameraManager`
  across a manual native gameplay aspect transition. Numeric object IDs may
  change after reloads; the evidence is tied to the same live objects within
  the tested session.
- The tested sequence was: `32:9` gameplay with the incorrect vertical
  framing, manual `16:9` with aspect constraint enabled and correct vertical
  framing, then return to `32:9` after the native recalculation.
- The observed states were:

  ```text
  A — 32:9 before transition:
      FOV=90, AspectRatio=3.555556, bConstrainAspectRatio=false

  B — 16:9 constrained transition:
      FOV=90, AspectRatio=1.777778, bConstrainAspectRatio=true

  C — 32:9 after transition:
      FOV=90, AspectRatio=1.777778, bConstrainAspectRatio=false
  ```

- `AspectRatioAxisConstraint` remained `MaintainXFOV` (`Axis=1`) and
  `bOverrideAspectRatioAxisConstraint` remained `false` in the captured
  states; no axis-policy transition was observed.
- The final correct 32:9 gameplay result therefore does not require the live
  camera `AspectRatio` to equal the physical display aspect. States A and C
  share `FOV=90` and `bConstrainAspectRatio=false` but have different camera
  aspect state and different visual results.
- The result confirms a native/settings-driven reevaluation sequence, not a
  single sufficient final property state. A storage-only property write is
  not equivalent to the native transition and is not promoted as a solution.
- This control intentionally excludes the cinematic CameraTweaks branch. The
  gameplay fix is not treated as the source of this native behavior; it is a
  separate production workaround that preserves the player's FOV while
  replaying the useful gameplay transition behavior.
- Confirmed: native A/B/C transition, same Camera/CameraManager/PCM
  ownership, authored FOV preserved at `90`, and no observed axis-enum
  transition. Unresolved: the exact native operation/evaluation event that
  performs the reevaluation, where the downstream view/projection rebuild is
  triggered, and whether an equivalent path can be invoked during a
  cinematic without the Hor+ FOV rewrite.

### Cinematics — historical 2.0.4 evidence

- Legacy 2.0.3 and historical 2.0.4 transition topology was reconstructed;
  current signature resolution is based on semantic instruction patterns, not
  fixed cinematic RVAs.
- The cinematic aspect store and ENTER/EXIT live-FOV consumer callsites are
  uniquely signature-resolved and fail closed on ambiguity or validation
  failure.
- The validated current boundaries are the aspect store equivalent of
  `RVA 0x6B7CB05` and live-FOV callsites equivalent to
  `RVA 0x2EE6936`/`0x2EE69A7` in the historical 2.0.4 image.
- On 21:9, runtime aspect `2.38889` produces correct cinematic framing and
  Hor+ FOV. On 32:9, runtime aspect `3.55556` produces Hor+ FOV about
  `126.87` from authored FOV `90`.
- Forced 16:9, 21:9 and 32:9 cinematic framing was user-tested. Forced 32:9
  at 2560x1440 correctly produced cinematic letterbox bars.
- `Auto` was user-tested without restarting through
  `16:9 → 21:9 → 32:9 → 16:9 → 21:9 → 32:9`; each cinematic aspect store and
  ENTER FOV boundary used the current aspect. Cinematic EXIT recovery into
  gameplay also passed at each tested aspect.
- Forced 21:9 uses the canonical 3440x1440 aspect `2.3888889`, producing
  cinematic FOV about `106.688` from authored FOV `90`.
- Native cinematic EXIT FOV recovery remains game-owned and untouched.
- Startup logs record uppercase SHA-256 identities for the loaded ASI and game
  executable. The historical 2.0.4 game identity was
  `2ECC5D19FE37F97E3F7F2467D652B299B5A47F010FA49FD803A49A4A6930A409`.

### Dialogue — historical 2.0.4 evidence

- The packaged `CoreVariables` reference established `DialogFOVDefault=70.0`
  as the native dialogue baseline. `CutsceneFOVDefault` remains separate and is
  not modified by this feature.
- Configuration/default registration and `DisplayFOV`/`CurrentFOV` parameter
  plumbing were audited but were not promoted as dialogue lifecycle owners.
- The WIDEBOY dialogue reference was used only to identify a plausible runtime
  boundary. The historical 2.0.4 equivalent resolved uniquely at hook boundary
  `RVA 0xD20F77`, where `XMM1` carries live native dialogue FOV samples.
- The boundary was proven to carry the native live stream, not a one-shot target:
  gameplay around `110` descends smoothly to `70` during dialogue and recovers
  to gameplay after EXIT. The boundary is not traversed during the tested
  ADS-only scenario.
- The former broad candidate searches for `DisplayFOV`, `CurrentFOV`, compact
  camera fields and downstream projection consumers did not identify a safer
  native target owner. They remain historical/deferred evidence, not production
  dependencies.
- Production dialogue policies are:
  - `Native` — pass through the game's original dialogue stream, targeting its
    native `70°` dialogue FOV.
  - `Adaptive` — preserve the native optical zoom strength relative to the
    actual gameplay baseline `G`, using projection-space geometry.
  - `Reduced` — apply half of the Adaptive optical zoom strength in projection
    space.
  - `Disabled` — hold the captured gameplay baseline `G` through dialogue.
- Adaptive and Reduced were validated at high gameplay FOV baselines near
  `90`, `110` and `120`; endpoint values matched the optical model and recovery
  returned to the actual captured gameplay baseline.
- The initial transformed EXIT path exposed the native EXIT sample jump. The
  validated EXIT anchor/recovery state now starts from the transformed dialogue
  endpoint and maps native recovery smoothly back to `G`, preserving native
  timing without camera writes or custom timers.
- Cinematic isolation resets dialogue transient state during cinematic active and
  recovery phases. The first dialogue after a cinematic captured the gameplay
  baseline rather than the cinematic FOV, and two sequential dialogues recovered
  without stale state.
- Historical production-candidate runtime validation on Steam 2.0.4 passed
  `Native`, `Adaptive`, `Reduced` and `Disabled`, including sequential cycles,
  cinematic-to-dialogue isolation, ADS-only specificity and configurable
  hotkey selection. This does not extend the current v1.0.0 runtime claim to
  Steam 2.0.4.

## Compatibility boundary

Gameplay, cinematic and dialogue boundaries use guarded signature resolution.
Steam 2.0.5 and 2.0.6 each have recorded production runtime sessions; the
Steam 2.0.2–2.0.4 static result covers historical v0.4.0 resolver contracts,
not the complete current resolver set, and does not claim runtime support.
Per-session game and mod identities and the validated feature scope are maintained in the
[authoritative build support and evidence matrix](docs/SUPPORTED_BUILD_MANIFEST.md).
A future executable identity still requires fresh resolver and runtime
validation.

The v1.0.0 release-candidate ASI hash documented for that release is
`D04A43E28DB5DFFD10D88B6F30BEF8FEC2A560E1CD9949FEA31FAF485DA0E7BC`.
However, the combined Steam 2.0.6 runtime record identifies its loaded mod as
`19F2F31C20BB5D47CD12D2D3D773774985A5D771E6F7F8730A6363983161DA72`.
The repository does not establish binary identity between those hashes, so the
runtime result is not attributed to the exact `D04A…` release-candidate file.
The diagnostic artifact is built separately and is not part of the production
release.

## Closed and deferred research

- Static interpolation/scalar-shape candidate ranking was closed after
  runtime rejection of unrelated candidates.
- Legacy transition-hub mapping and live-FOV consumption recovery are closed;
  the current live-FOV boundary is confirmed.
- Aspect writer provenance and immediate-patch feasibility are closed for the
  tested path.
- Post-EXIT atomic B/C scheduling was tested and closed as a production
  solution: it preserved mechanics but did not remove the visible seam.
- The v1.0.0 atomic gameplay apply and first-descending-sample
  `RecoveryStart` handoff passed production runtime validation on Steam 2.0.6;
  the old staged `0x5` replay is not used by the production path.
- Native cinematic FOV bypass research remains deferred. The game's native
  post-cinematic FOV recovery is preserved and is not rewritten by v1.0.0.
- Downstream writer/projection candidate searches were closed for the current
  evidence set without a promoted renderer consumer.
- Dialogue parameter plumbing and compact-field target-owner searches were
  closed or deferred after the concrete live boundary was established.
- The WIDEBOY-derived dialogue boundary was independently mapped and validated;
  its direction heuristic was not copied into production.
- Dialogue feasibility and production promotion passed with the four-policy
  lifecycle model, EXIT recovery anchor, cinematic isolation and ADS-specificity
  guard.
- Weapon/viewmodel ownership research remains deferred pending a new validated
  object or downstream projection anchor.
- Dynamic resolution changes during a running session are validated for
  `AspectRatio=Auto` across 16:9, 21:9 and 32:9. Configuration file changes
  still require a game restart.
- The native gameplay aspect reevaluation sequence above is validated from a
  bounded UE4SS runtime capture. The exact triggering native function and
  downstream projection owner were not identified; no compatibility or
  cinematic behavior claim follows from this control alone.

### Subtitle horizontal centering — deferred / rejected for production

- The subtitle displacement reproduces on Steam 2.0.5 without
  `STALKER2CameraTweaks`, including at `2560x1440` and `5120x1440`; it is
  therefore not attributed to the camera/aspect mod.
- UE4SS located the live `SubtitleView` widget. Its
  `SetRenderTranslation()` moves the complete subtitle composition — speaker
  name, punctuation, dialogue text and background — while leaving the
  dialogue-choice UI unaffected.
- Child-level `HorizontalAlignment`, `VerticalAlignment`, `Justification`,
  width override and fixed per-child offsets did not correct final placement.
- Fixed `SubtitleView` offsets and resolution/aspect-specific offset tables are
  rejected because rendered composition width changes with speaker name,
  dialogue length, wrapping and content.
- UE4SS Lua did not expose reliable post-layout `FGeometry` or viewport
  geometry. A bounded C++/Slate Batch 1 inspection found no Unreal/Slate
  headers, UObject/Slate bridge, viewport geometry interface or established
  safe ABI anchor in the current ASI project; no research probe was built.
- Production changes: none. No subtitle hook, UI write, Slate integration or
  heuristic correction is part of the release.
- Status: `SUBTITLE GEOMETRY RESEARCH — DEFERRED`; the fix remains rejected
  until an independent safe post-layout geometry interface is established.

#### UE4SS runtime evidence

The following observations were made in the Steam 2.0.5 game session with
UE4SS and are retained as research evidence only. All property changes were
temporary runtime experiments and were cleared by restarting or reloading the
game; none were added to the production ASI.

- The actor dump `1789298599-ue4ss_actor_data.csv` did not provide the live UMG
  instance. UE4SS Lua found exactly one live object:

  ```text
  SubtitleView_C /Engine/Transient.Stalker2GameEngine_2147482609:
  BP_SML_C_2147482553.SubtitleView_C_2147461272
  ```

- The live `USubtitleView` fields identified from the UE4SS headers were:
  `SpeakerDialogText`, `TwoPoint`, `NameBox`, `SubtitileBorder`,
  `SubtitileContainer` and `TextDialog`.

- The initial widget relationships were:

  ```text
  Overlay_1
  ├─ SubtitileContainer → SubtitileBorder → TextDialog
  ├─ NameBox → SpeakerDialogText + TwoPoint
  └─ separate dialogue-choice UI elements
  ```

- The live text values were confirmed independently:

  ```text
  TextDialog         = "Здоров!"
  SpeakerDialogText  = "Вітя Бусел"
  TwoPoint           = ":"
  ```

- Initial observed layout values included:

  ```text
  TextDialog.CommonTextObj.Justification = 0
  SpeakerDialogText.CommonTextObj.Justification = 2
  TextDialog.GetDesiredSize().X ≈ 85.64999
  SubtitileContainer.WidthOverride = 800
  SubtitileContainer.HeightOverride = 100
  SubtitileContainer slot padding = L0 T0 R0 B50
  NameBox slot = H3 / V1
  ```

  The observed `H3/V1` on `NameBox` corresponds to right/top in the tested
  Overlay layout; changing it to center did not change the final rendered
  position.

- Temporary child-level tests were performed and visually rejected:

  ```text
  TextDialog.CommonTextObj:SetJustification(2)
  SubtitileContainer.Slot:SetHorizontalAlignment(2)
  SubtitileBorder:SetHorizontalAlignment(2)
  TextDialog.Slot:SetHorizontalAlignment(2)
  SubtitileContainer.WidthOverride = 0
  ```

  The values changed or the calls returned successfully, but the rendered
  subtitle position did not follow them.

- `RenderTransform` fields reported zero translation, unit scale and zero
  angle. Direct field edits did not affect rendering. The callable
  `SetRenderTranslation()` did affect rendering:

  ```text
  TextDialog +500                  → moved only the dialogue text
  SpeakerDialogText +500           → moved the speaker text separately
  NameBox +500                     → moved the name and colon together
  SubtitleView +500                → moved the complete subtitle composition
                                      while dialogue-choice UI stayed in place
  ```

  The `SubtitleView` root test was the only confirmed whole-block control
  point. Fixed trial values such as `+500`, `+650` and a compensating `-150`
  on a child were diagnostic only and are explicitly rejected as a solution.

- Hiding `TextDialog` immediately removed `"Здоров!"`, confirming that it was
  the active rendered text. Hiding `SubtitileBorder` removed both the text and
  its background, confirming their parent relationship.

- `FindAllOf("FadeoutScreen")` returned no live instances during the tested
  dialogue. The subtitle was therefore not attributed to a separate active
  `UFadeoutScreen` layer.

- `GetCachedGeometry()` returned a wrapper, but the UE4SS Lua binding did not
  expose usable numeric absolute position/size or local-size methods. A broad
  parent/child traversal was attempted once and caused a game crash; it was
  abandoned and is not part of the accepted evidence method.

- Final UE4SS conclusion:

  ```text
  SubtitleView whole-block translation: CONFIRMED
  Child alignment/justification control: NOT EFFECTIVE
  Post-layout FGeometry via Lua: NOT AVAILABLE
  Fixed offset: REJECTED
  Production UI write: NONE
  ```

## Testing limits

## 2026-09-14 — Weapon Viewmodel FOV research boundary

- Reference WVF behavior is established: `.wvf` profile selection and
  viewport/FOV math write `/Game/_Stalker_2/Materials/MPC/MPC_FOV.MPC_FOV`:
  `TanFOV`.
- The `TanFOV → visible weapon/viewmodel framing` relationship was causally
  validated through one controlled UE4SS intervention with confirmed readback.
- The reference runtime bootstrap is established: `WVF_C` spawns
  `S2Dev_Event_Watcher_C` and `WVF_Actor_C`; the actor's profile path reaches
  the `TanFOV` write.
- Standalone ASI access remains unestablished. No validated,
  patch-resilient UObject/UClass/UFunction registry or invocation bridge is
  present in the current architecture. No guessed offsets, layouts, RVAs or
  `ProcessEvent` ABI were accepted.
- Therefore weapon/viewmodel FOV is deferred at the production boundary, not
  because the controlling mechanism is unknown:

  ```text
  Reference mechanism       CONFIRMED
  TanFOV causal ownership   CONFIRMED
  UE4SS MPC access          CONFIRMED
  Native ASI bridge         DEFERRED
  Repair timing/value       DEFERRED
  Custom Weapon FOV         DEFERRED
  Production implementation NONE
  Production changes        NONE
  ```

- The active WVF feature-development branch is closed. Reopen only if a new,
  independently grounded UE reflection/invocation bridge becomes available.

- Build success proves compilation and linking only.
- A signature match is not hook proof without decode and runtime evidence.
- Build support is scoped by the authoritative [build evidence matrix](docs/SUPPORTED_BUILD_MANIFEST.md); do not infer that the latest workspace/release-assets artifact was the binary used in an earlier runtime session.
- A recorded production runtime session validates the gameplay/cinematic
  handoff on Steam 2.0.6. This is session-specific evidence, not proof of the
  exact current candidate binary's identity. Older-build evidence remains
  static portability evidence only.
- The recorded Steam 2.0.6 production test scope covers dialogue runtime
  behavior; no runtime compatibility claim is made for older Steam builds.
- Dialogue runtime validation covers Native, Adaptive, Reduced and Disabled,
  high-FOV baselines, smooth ENTER/EXIT recovery, sequential cycles,
  cinematic-to-dialogue isolation, ADS-only specificity and F9/F10 policy
  selection. Runtime hotkeys apply to the next lifecycle, not an active one.
- In the recorded runtime sessions, `NativeHorPlus`, `GameplayHorPlus`, `Auto`,
  forced `16:9`, forced `21:9` and forced `32:9` were tested on the native
  5120x1440 display or in the Auto hot-switch sequence. Consult the evidence
  matrix for the exact session/artifact scope.
- The game's native camera restoration remains the recovery target. The mod
  now waits for a validated native gameplay sample, then applies the existing
  HorPlus transform without a fixed delay or hard-coded FOV.
- Subtitle horizontal positioning is a separate vanilla UI issue. It was
  reproduced without the mod, and no production compatibility or fix claim is
  made for it.

## v2.0.1 working-tree candidate status (not released)

- The current offline suite, localization/resource coverage and production
  build have passed as recorded in the 2026-09-28 validation snapshot above.
- The earlier reviewed runtime log confirmed a regression in the first Gameplay
  recovery gate: EXIT target `90.6557` was followed 3 ms later by a native writer
  sample already matching that target (`flags=0x4`, aspect `3`, same source),
  but Gameplay stayed in `CinematicExiting` until ADS supplied a departure and
  return. Requiring Dialogue's depart-then-return sequence for Gameplay was
  incorrect. That exact-hash run has since been user-tested and visually
  confirmed; the runtime details are summarized in the validation section.
- Gameplay recovery now independently validates a readable sample, gameplay
  flags, valid aspect, saved native EXIT target and camera source ownership.
  Retained native `GameplayBaseline` ownership permits immediate recovery on
  the first matching post-EXIT sample. A replaced source still requires the
  existing source-bound recovery evidence. Dialogue exclusion keeps its
  depart-then-return semantics and is not cleared by immediate Gameplay recovery.
- Transitional/ambiguous samples pass through without HorPlus or
  `GameplayBaseline` updates. The focused regression covers both
  `90 → 106.688 → EXIT → 106.688 → 90 → 106.688` (never `122.044`) and
  an already-native first post-EXIT sample that resumes immediately without ADS.
  It also covers the recorded `90.6557`/aspect `3` case, 16:9 and 48:9,
  source/flags/invalid-input rejection, subsequent ADS and later native FOV
  changes. The full suite passes with 54 harnesses and the production build
  passes. The exact-hash runtime log confirms the already-native-first-sample
  branch; the transformed-transitional-first-sample branch remains covered by
  offline regression, not by that runtime session.
- Current built root `STALKER2CameraTweaks.asi` SHA-256:
  `6C76FF71D530A362B2B14F3FC1BE5EBD9D30D899495206FF885DC7258452347F`.
- The production startup journal is lightweight by default and writes
  `STALKER2CameraTweaksStartup.log`; forensic startup tracing remains an
  explicit diagnostic opt-in. The current exact-hash candidate has been
  launched for the camera recovery scenario. Overlay first-frame rendering,
  startup-resize recovery, cursor behavior and fresh journal milestones are not
  established by the supplied camera log.
- Prior user runtime evidence is bounded to the bootstrap-enabled build that
  preceded the journal-only addition. Do not attribute those five successful
  launches to the current ASI hash without a matching runtime identity record.
- The v2.0.1 candidate is not tagged, released or packaged. This update did not
  perform Git/release/package actions. Do not load the unified ASI together
  with `STALKER2UltrawideFix.asi` or `STALKER2GameplayAspectFix.asi`.
- Preserve exact ASI and game executable identity in subsequent runtime
  evidence; the [supported-build evidence matrix](docs/SUPPORTED_BUILD_MANIFEST.md)
  remains authoritative for game-version claims.
