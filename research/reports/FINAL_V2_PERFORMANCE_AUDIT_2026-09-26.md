# Final v2 Performance Audit

Date: 2026-09-26. Scope: current supported production source selected by
`build.cmd`, including combined overlay, embedded localization, camera,
cinematic and dialogue integration. Static audit only; no production changes,
optimization, game launch, Git, build, test rerun or release packaging.

**Status note:** Sections 1–8 record the original pre-repair audit snapshot.
Section 9 is the later authorized repair follow-up and supersedes the earlier
open status of PERF-01 and PERF-02. They are repaired and offline-validated;
PERF-C01 remains a measurement question, not an established performance
defect. Counts below are retained as the results of their respective
historical runs; the latest inventory/audit figures are recorded in Section 9.

## 1. Executive summary

The findings in this section describe the original **pre-repair** source
snapshot; they are not the current disposition after Section 9.

No established material frame-time regression or performance release blocker
was found. This is **not** a measurement-backed claim of negligible overhead.
The current implementation has two concrete, low-priority frequency mismatches:

| ID | Classification | Severity | Observation |
| --- | --- | --- | --- |
| PERF-01 | ESTABLISHED redundant work; practical cost unmeasured | P3 / non-blocking | Completed startup-hint publication does not prevent a full settings read on every eligible Present, including closed-overlay steady state. Visible frames also repeat full settings reads. |
| PERF-02 | ESTABLISHED redundant work; practical cost unmeasured | P3 / non-blocking | WindowProc does modifier-key queries and takes the rebind mutex for non-key messages before rejecting them. |
| PERF-C01 | STRONG CANDIDATE; not a confirmed contention defect | P3 investigation priority | Game camera publication and visible UI consumption share locks; some callback critical sections include synchronous logging. Actual contention/tail latency needs measurement. |

P3 here means a bounded improvement opportunity, not a demonstrated FPS loss.
There are no P1/P2 findings inferred simply from per-frame execution. No
cache, scheduler, lock-free replacement or broader refactor is justified by
this audit alone. F-01–F-07 remain closed with their documented limitations.

**Closed overlay:** no ordinary settings frontend, semantic projection,
localized labels, ImGui frame, draw submission or font rebuild after the hidden
and empty-notifications early return. It is not a zero-work path: discovery
association bookkeeping, notification drain/expiry, PERF-01 and WindowProc
interception continue. Hotkey dispatch has its own 50 ms wait loop regardless
of visibility. Camera observation/publication also continues for runtime
consumers; it must not be disabled merely because the UI is hidden.

**Open overlay:** immediate-mode drawing, localization/string presentation,
settings/semantic reads and conditional GPU frame-context reuse run per
rendered frame. Atlas/resource creation is lifecycle-driven, not steady-state.

### Evidence boundary

Reviewed current source, build definitions, active project contracts and the
closure report. Rechecked local production ASI SHA-256:
`1AFF9A14D5A55B9E08FE1D3E3418ABDC6CA5C39B046B5DD787620850CC7A843D`.
This is artifact identity only, not a new build or runtime result.

The inherited offline baseline at the time of the original audit is documented in
[`FINAL_V2_ARCHITECTURE_CLOSURE_AUDIT_2026-09-26.md`](FINAL_V2_ARCHITECTURE_CLOSURE_AUDIT_2026-09-26.md):
43/43 harnesses, zero XFAIL, localization/resource/glyph audit for 18 locales
and 162 keys, supported production/diagnostic/compatibility builds PASS.
Those checks were **not rerun here** and do not measure performance. Current
ASI has no final integrated runtime regression; historical binary runtime
evidence is not transferred. No timings, allocation rates, lock contention
rates, frame-time percentages or GPU stalls were measured.

## 2. Execution-frequency map

Frequency follows call sites and gates, not an assumed fixed engine callback
rate. Native callback multiplicity per frame is UNKNOWN.

| Class | Production path | Work and boundary |
| --- | --- | --- |
| Hot / potentially per-frame | `runtime.cpp:3357` ReplayManualTransition → `:3636` ApplyHorPlusGameplay | Guard reads, source/flags memory validation, context-change observation, aspect-restoration update, FOV transform, FOV observation and baseline publication. Disabled/mode/coordinator paths differ. |
| Hot / valid HorPlus observations | `runtime.cpp:3033` PublishCameraStateSnapshot, called at `:3403` | Dialogue-state sample, retained-neutral evidence tracking, fixed-value snapshot update, camera-state mutex, atomic sequence. Retention is sample-based, not UI-only. |
| Hot / native dialogue samples | `runtime.cpp:2480` TraceDialogueBoundary | Validation, dialogue mutex, source/target read, state-machine progression and conditional projection transform. The name does not make this diagnostic-only: production dialogue behavior lives here. |
| Hot / every hooked Present | `discovery_runtime.cpp:424` HookPresent | Swapchain lookup, shared discovery lock, association evidence updates/lookups, optional bounded diagnostic output, renderer call, original Present. |
| Hot / renderer entry, including hidden | `renderer_runtime.cpp:1145`, `:1044`; `runtime.cpp:4914` | Notification drain under notification mutex, expiry/time check; association/resource gates; startup settings snapshot; hidden/empty early return at `:1196`. Resize rebuild can precede it. |
| Message-driven, potentially high frequency | `discovery_runtime.cpp:182` OverlayWindowProc; `input_state.cpp:28` HandleRebindMessage | Modifier queries and rebind gate, toggle/settings read on WM_KEYUP; visible-only ImGui/input capture; original window procedure otherwise. PERF-02 applies while hidden too. |
| Periodic / visibility-independent | `runtime.cpp:4511` HotkeyLoop | Wait 50 ms on stop event, full settings snapshot, four asynchronous key reads, capture/consumed-key checks. Mutate/notify/persist only on accepted key edges. This is hotkey polling, not language polling. |
| Transition-driven | `runtime.cpp:1461`, `:2132` cinematic aspect/ENTER integration; `:2722` TryAtomicExitHandoff | Aspect application, selection/baseline capture, FOV transform/cache, coordinator/exclusion/restoration transitions. Aspect setter's actual runtime invocation frequency is not established here. |
| Overlay visible only | `renderer_runtime.cpp:1253` onward | Window/layout, localized controls, semantic/settings snapshots, presentation classification, diagnostic values; expanded camera details only under TreeNodeEx at `:1639`. |
| Visible UI OR active toast | `renderer_runtime.cpp:1200`, `:1222` onward | Font-setting/profile comparison, SetLocale, backbuffer index, conditional WaitForFrame, allocator/list reset, ImGui frame, draw recording/submission, fence signal. A toast intentionally renders while settings overlay is hidden. |
| Overlay-open / explicit Auto selection | `game_language_reader.cpp:163`; `discovery_runtime.cpp:185`, `:269` | Validated native reader on game window-procedure context, bounded FString copy/free, locale normalization/effective-state update. No Present/worker polling. |
| Settings-change | `runtime.cpp:4132` ApplyRuntimeSetting / `:4432` persistence; renderer controls | Typed mutations, locale/profile/size decisions, notifications and serialized INI update. Placement writes after observed move ends at `renderer_runtime.cpp:1286`. |
| Font/profile/size change | `renderer_runtime.cpp:893` RebuildFontAtlas | GPU lifetime synchronization, atlas invalidation/build/upload and rollback/fail-closed handling. No atlas rebuild for unchanged size/profile. |
| Initialization | `runtime.cpp:4651` Initialize; domain resolvers; `discovery_runtime.cpp:775` Initialize | Module/game hash, config/template loading, one-time API publication, scans/validation/hook installation, workers; DXGI export hooks and discovery initialization thread exits. |
| Initialization / resize / recreation | `renderer_runtime.cpp:691`, `:757`, `:780`, `:1012` | Heaps/fence/event/frame allocators/command list, backbuffers, ImGui/font setup, placement load; resize releases and deferred rebuild. Not per-frame resource creation. |

The frequency map was established before prioritizing findings. Initialization,
configuration persistence and occasional atlas costs are not hot-path defects.

### Closed steady-state call trace

After successful startup-hint publication, no active toast, no resize:

1. HookPresent takes `g_mutex`; FindSwapchain scans registered hooks, State
   counts/finds candidates, ObservePresent updates present evidence, State is
   read again and GetSingleCandidate finds/counts candidates. These are
   retained vector scans, not per-frame vector construction
   (`discovery_evidence.cpp:92`, `:124`, `:136`).
2. Renderer::UpdateNotifications constructs an empty incoming vector, drains
   the queue under `g_notificationMutex`, samples time and removes expired
   active notifications. An empty vector does **not** itself allocate.
3. Renderer checks resource/lifecycle identity and calls the full settings
   snapshot as an argument to TryPublish, even if publication already occurred.
4. Hidden + empty notifications returns before font checks, localization,
   swapchain QueryInterface, WaitForFrame, ImGui NewFrame or GPU submission.
5. HookPresent calls original Present. With diagnostics off, its detailed
   Present output branch is not entered.

In parallel, WindowProc and the hotkey worker still observe input. Camera
functionality retains its own observations/baselines/lifecycle work. Therefore
"practically negligible" remains a measurement target, not an established
result. Hidden UI with a startup/hotkey toast and resize recovery are explicitly
different from closed steady state.

## 3. Confirmed findings

### PERF-01 — Full settings reads outlive the once-only startup action

- **Severity / status:** P3, non-blocking. ESTABLISHED redundant read; expected
  practical magnitude UNKNOWN, likely small unless synchronization is contended.
- **Exact locations:** `src/overlay/renderer_runtime.cpp:1176`–`:1180`;
  `src/overlay/renderer_state.cpp:76`; `src/plugin/runtime.cpp:4255`–`:4275`.
  Visible read duplication additionally at renderer `:1201`, `:1299`, `:1305`
  and runtime `:4295`.
- **Frequency:** every eligible ready Present, hidden or visible. When visible
  and semantic state is ready, there are four full settings-read requests:
  startup, font settings, semantic snapshot's configured read, and UI settings.
  This count describes call structure, not measured allocations or elapsed cost.
- **Behavior/evidence:** TryPublish rejects already-published hints inside its
  body, but the Snapshot argument is evaluated first. ReadRuntimeSettings loads
  feature and locale/font atomics, constructs/copies locale-code string and takes
  the hotkey-config mutex to copy binding fields. The hint needs this information
  only while attempting first publication. The semantic configured read uses
  only the five feature settings although the full read also loads UI/hotkeys.
- **Why avoidable:** startup publication is terminal once accepted, yet its
  acquisition continues indefinitely. It adds a full read and shared-lock
  acquisition before the closed-overlay early return. The four visible reads
  amplify acquisition, but independently sampled semantic facts must not be
  falsely advertised as an atomic state.
- **Expected impact:** eliminates a continuous unnecessary settings/mutex path;
  no demonstrated FPS improvement. Short locale strings may use small-string
  storage; heap allocation on every snapshot is **not established**.
- **Safe future direction:** after measuring, put the already-published gate
  ahead of snapshot acquisition while retaining retry before publication. If
  visible acquisition proves material, pass a single per-frame configured view
  only where its ownership/freshness contract permits; no blanket state cache.
- **Preserve:** F-01 immutable publication/acquire-release boundary, startup
  readiness and once-only semantics, actual effective toggle key, independent
  observed/active facts, unavailable-before-ready behavior.
- **Future validation:** compare Snapshot call counts after hint publication;
  startup/readiness harness, semantic/settings harnesses, all 43 harnesses and
  localization audit/build after an authorized repair. Exact-artifact runtime
  confirms one startup hint, correct key and identical Auto/settings behavior.

### PERF-02 — Keyboard-only rebind preparation executes for non-key messages

- **Severity / status:** P3, non-blocking. ESTABLISHED redundant work;
  practical cost depends on real message rate.
- **Exact locations:** `src/overlay/discovery_runtime.cpp:190`–`:195` and
  `src/overlay/input_state.cpp:28`–`:35`.
- **Frequency:** every window-procedure message except the special Auto-sync
  message, including mouse/raw-input/other messages with overlay closed.
- **Behavior/evidence:** WindowProc queries Ctrl, Shift and Alt via
  GetAsyncKeyState (short-circuited when one is down), then calls
  HandleRebindMessage. That function locks `rebindMutex_` before discovering
  that non-key-down/up messages cannot participate in rebind/consumed-key logic.
- **Why avoidable:** non-key messages do not use modifier state or protected
  rebind state. Mouse/raw-input traffic can be much more frequent than actual
  binding changes, adding OS queries and lock acquisitions to an otherwise
  pass-through path. Actual message volume and contention are unmeasured.
- **Expected impact:** less closed-overlay input interception work, potentially
  relevant with high input-message rates; no quantified CPU/frame-time benefit.
- **Safe future direction:** measure message classes, then classify relevant
  keyboard messages before modifier queries and before entering the rebind
  lock. Do not weaken the lock for messages that access protected state.
- **Preserve:** WM_KEYDOWN/UP and WM_SYSKEYDOWN/UP, Escape cancellation, consumed
  key release, custom toggle key, modifier rejection, original-WndProc
  pass-through, visible mouse/raw-input capture and posted Auto synchronization.
- **Future validation:** input/rebind harness plus message-class regression
  covering mouse/raw input and keyboard/system-key consumption; all harnesses,
  audit/build after repair; one exact-artifact runtime input/rebind pass.

Neither finding establishes a material slowdown. They identify concrete work
whose frequency exceeds its semantic need, with bounded safe directions for
future review rather than speculative architectural changes.

## 4. Strong candidates and measurement-needed observations

### PERF-C01 — Shared camera/UI critical sections can transmit callback latency

- **Severity / status:** P3 investigation priority; STRONG CANDIDATE for tail
  latency under concurrent sampling, not confirmed contention or a faulty lock.
- **Locations:** `runtime.cpp:3033`–`:3172` (dialogue and camera-state locks),
  `:2501` onward (dialogue callback lock over state-machine processing),
  `:4351`, `:4393` (semantic reader); `camera/fov_observation.cpp:43`,
  `camera/gameplay_baseline.cpp:52` / `:83`,
  `camera/gameplay_aspect_restoration.cpp:53` (publication/read locks).
  Runtime Log is synchronous at `runtime.cpp:658`; logger is
  `spdlog::basic_logger_mt` at `:4662`.
- **Frequency / behavior:** camera samples publish observations/baseline and,
  in HorPlus, update presentation snapshot even when UI is closed. Visible UI
  reads some of these same locks each frame. Dialogue processing holds its
  lock through target validation, lifecycle logic and conditional logs.
  Camera-state semantic-change logging occurs inside the state lock when
  Diagnostics.Enabled is true. SnapshotSemanticsChanged excludes numeric-only
  movement and event sequence (`camera_state_snapshot.cpp:121`), so it does
  **not** log every writer observation merely because the sequence increments.
- **Concern / why potentially disproportionate:** synchronous formatting/sink
  work within a shared critical section can extend another thread's wait beyond
  the actual state copy. Publication locks alone are required contracts, not
  unnecessary work. No contention or material cost has been demonstrated.
- **Expected impact:** possible transition/diagnostic latency tails shared by
  game and render threads; magnitude and occurrence UNKNOWN. Diagnostics-off
  is the production performance baseline; enabled diagnostics intentionally
  has extra cost.
- **Safe direction if established:** first move only auxiliary formatting/sink
  work out of a critical section using an owned event-value copy, if ordering
  permits. Consider publication changes only if measured acquisition/hold time
  is material and freshness/retention remain explicit. No visibility-gated
  baseline publishing, collapsed snapshots or speculative lock-free bridge.
- **Preserve:** source tokens, native/transformed spaces, provenance/freshness,
  neutral sample retention, per-observation sequence, cinematic baseline and
  dialogue recovery/exclusion, safe ownership and callback ABI behavior.
- **Validation after future repair:** snapshot/baseline/restoration/dialogue
  harnesses, deterministic concurrent reader checks for changed contracts,
  complete baseline/audit/build; exact-artifact concurrent open-overlay camera
  transitions with diagnostics off and separately on. Compare lock wait/hold
  distributions and log ordering, not only average FPS.

### HYPOTHESIS / MEASUREMENT NEEDED — cinematic aspect callback multiplicity

`runtime.cpp:1474` samples the viewport for a log-source label, then
`:1479` resolves Auto through a second ReadClientViewportAspect call
(`:1183`, `:1200`). `:1483` logs each successful store without an unchanged-state
gate. These operations are ESTABLISHED; a hot-path problem is **not**. The
resolved setter may be a rare construction/lifecycle callback. No priority or
repair finding is assigned without its actual invocation rate.

If the concrete callback is unexpectedly frequent in the eventual integrated
run, count invocations/viewport queries/log emissions. Only then consider
reusing that invocation's single sampled viewport for application and reporting,
or logging actual transitions rather than duplicate stores. Preserve Auto
fallback, writable checks and replaced-instruction continuation. Do not cache
viewport across resize/window changes. No separate RE/runtime session is
requested solely to investigate this low-confidence possibility.

## 5. Inspected healthy paths / non-findings

### Camera, observations and state propagation

- Pure HorPlus and dialogue/cinematic math use scalar values and bounded
  calculations, no explicit heap/string/FS work (`camera/horplus.cpp`,
  `gameplay/horplus_gameplay.cpp`, `dialogue/dialogue_fov.cpp`,
  `cinematics/cinematic_fov.cpp`). Transcendental math is required transform
  work; no evidence supports memoizing adjacent dynamic FOV samples.
- Normal camera observations/baselines/restoration snapshots are fixed-value
  copies. Their different contracts are meaningful. Updating sequences and
  sample counters even for unchanged numeric FOV is not proof of wasted work:
  publication and stability evidence are sample-based.
- Per-hook SafeRead/IsWritable calls validate memory with VirtualQuery and
  bounds (`platform/win32/memory.cpp:26`, `:36`). Their cost is unmeasured but
  they are safety work, not presentation helper overhead. No page-validity cache
  or removed guard is recommended.
- AspectRecalculation retains source/context/restoration/native-pass-through
  flow (`runtime.cpp:1716` onward). Transition/handoff pending gates avoid
  applying transition logic when inactive. Camera recreation may change
  evidence between adjacent callbacks; stable-looking values do not authorize
  caching native reads.
- CameraIntegration and feature presentation are small pure projections of
  semantic evidence, done only for visible settings UI. They do not re-run
  native FOV transforms (`overlay/camera_integration.cpp:44`,
  `overlay/feature_presentation.cpp:23`). Viewport collection in semantic read
  uses current client geometry (`runtime.cpp:4418`); EnumWindows fallback on
  focus loss exists (`platform/win32/viewport.cpp:21`). No measured pathology.

### Overlay rendering and input

- Hidden/empty early return prevents ordinary frontend work. Notification
  checking remains intentional to allow startup/hotkey toasts when hidden.
  Pending notifications are capped at eight (`runtime.cpp:4904`), active
  notifications at three (`renderer_runtime.cpp:1052`). No unbounded toast queue.
- Discovery bookkeeping includes mutex/vector lookups every Present, but no
  per-frame signature scan, hook creation or default log stream formatting.
  With normal few swapchains/candidates, this is not established material cost.
  Registry size is not assumed universally bounded; sustained swapchain churn
  would need evidence before reporting growth as a performance defect.
- Visible controls use some temporary localized strings and placeholder
  vectors (`localization_manager.cpp:107`, `localization_formatter.hpp:10`).
  The long strings can allocate; exact rates are unknown. Immediate-mode label
  drawing is not inherently unnecessary. Fixed option arrays are stack/static
  metadata rather than dynamically reconstructed maps/vectors.
- Common selector width scans fixed options and 18 locale display names each
  visible frame (`renderer_runtime.cpp:1332`–`:1350`). It could be cached by
  font/style generation if profiling found it material, but current bounded
  CalcTextSize work does not justify a separate finding or new cache.
- Settings tooltips localize/measure option descriptions **after** hover and
  BeginTooltip gates (`renderer_runtime.cpp:206`). Expanded camera detail
  formatting is gated by TreeNodeEx. No eager construction of every tooltip or
  hidden expanded detail block was found. Status descriptions remain visible
  dynamic text, not removed for optimization.
- Hotkey worker's 50 ms event wait is intentional key-edge dispatch and stop
  responsiveness, not a busy loop. It does full settings reads, including locale
  it does not need; at this cadence no material cost is established and no new
  specialized API is recommended merely for purity.

### D3D12 lifecycle

- Initialize creates descriptor heaps, fence/event, one allocator per buffer
  and a command list (`renderer_runtime.cpp:724`–`:750`), buffer count capped
  at 16. Steady rendered frames reuse them and reset allocator/list rather
  than recreate them (`:1235`). Backbuffer resources/RTVs rebuild on init/resize,
  not ordinary frames.
- WaitForFrame (`:992`) returns without waiting when no prior fence or the
  context's fence is complete. It waits only for in-flight context reuse;
  reached for visible UI/toasts, not closed-empty steady state. Fence presence
  does not prove a stall. Preserve allocator/resource lifetime safety.
- WaitForGpu (`:881`) is used for font atlas replacement (`:919`), resize
  release (`:1018`) and shutdown (`:1690`); not every Present. These lifecycle
  barriers must not be removed for micro-optimization. Visible GPU submission
  plus fence signal is expected rendering, not a confirmed bottleneck.
- Deferred resize resource/ImGui backend work can occur while hidden because
  resources must return to a usable lifecycle state. No steady-state recreation
  or redundant continuously-triggered resize was found.

### Localization, fonts and Auto

- Catalog initialization parses/validates all 18 embedded catalogs once;
  Initialize's attempted flag prevents repeated parsing on resize
  (`localization_manager.cpp:44`). Text lookup uses transparent unordered-map
  lookup returning string_view before Text copies (`localization_catalog.hpp`,
  `localization_catalog.cpp:203`). No filesystem catalog load per frame.
- Visible/toast frames compare font size/profile and call SetLocale. SetLocale
  does registry/catalog selection again (bounded 18-element domain), but does
  not parse JSON, reload resources or rebuild atlas. That small repeated
  selection is a non-blocking observation, not evidence of material cost.
- Atlas rebuild only when size/profile differs (`renderer_runtime.cpp:1206`);
  unchanged guard also exists inside RebuildFontAtlas. Profile changes need
  atlas changes; changing between locales sharing size/profile need not rebuild.
- Fonts reference immutable embedded bytes with FontDataOwnedByAtlas=false
  (`localization_font.cpp:173`); selector glyph ranges initialize once.
  Font atlas memory/upload cost, especially CJK, is rare useful lifecycle work.
  Embedded size is not optimized or reported as a runtime defect.
- Auto native reader is confined to explicit selection / closed→open on
  WindowProc, not Present or the hotkey loop. Successful resolution is retained;
  failed validation can retry on later explicit opens, not continuously.
  FString copy is bounded to 64 wchar_t elements and game-owned data is freed
  only under the existing plausibility/ABI safeguards. No broad heap scan or
  language-state polling.

### Logging, atomics, configuration and startup

- RuntimeSettingsApi read performs acquire publication-state load and immutable
  callback dispatch (`runtime_settings.hpp:305`, `:333`), not per-read CAS or
  callback-bundle construction. No change to accepted F-01 is needed.
- Production source retains opt-in INI diagnostics (Enabled atomic), including
  FOV source tracing and semantic-change logs. With diagnostics off, gates
  precede auxiliary source reads/formatting (`runtime.cpp:2979`, `:3664`,
  `:1498`). Heavy compile-gated research hooks/workers are absent from default
  build. Build optional instrumentation flags are populated only for diagnostic
  profile (`build.cmd:31` onward); optional env flags alone do not enable them
  in production. This does not mean **all** diagnostics are compiled out.
- Detailed Present diagnostics are capped by observation windows armed at
  creation (32) and resize (16), and are gated before output/query work
  (`discovery_runtime.cpp:441`, `:576`, `:507`). Overlay lifecycle logger flushes
  synchronously and shares discovery mutex (`:86`); normal calls are lifecycle,
  visibility, font and failure events, not every Present.
- Some LogDiagnostic arguments are constructed before its Enabled test
  (factory/identity/resize paths), but these are lifecycle calls, not steady
  Present label preparation. No hot allocation defect inferred from them.
- Runtime logger constructs ostringstream only for invoked Log calls and uses
  synchronous spdlog basic sink. Production info does not flush_on every entry
  (`runtime.cpp:4664` sets error-level flushing), unlike overlay logger. Useful
  transition/error messages are not defects simply because they use strings
  and sink locks. PERF-C01 records lock-held logging separately.
- Config parsing/template/migration are startup work. Serialized atomic
  replacement, write-through and last-known-good preservation are rare settings
  persistence (`config_repository.cpp:16`, `:467`, `:508`, `:541`). No config
  file access was found in ordinary steady camera sample or hidden Present.
- Mod/game hashes are each computed at startup (`runtime.cpp:4675`–`:4676`);
  SHA buffering allocates there (`platform/win32/sha256.cpp:23`). Resolver
  signatures scan during installation; indexed EXIT is fallback, not repeated
  alongside successful legacy resolution. No successful rescanning per frame.
- Overlay discovery initialization thread returns after export-hook setup
  (`discovery_runtime.cpp:817`). Ongoing discovery is event interception, not a
  repeating scan worker. Diagnostic-only resolution/discovery workers are
  compile-gated (`runtime.cpp:4850`–`:4873`), outside default production.
- `/O1` compilation is established (`build.cmd:48`), not proof of poor
  runtime performance. No compiler-flag or binary-size optimization is proposed.

## 6. Smallest justified future measurement plan

No measurement is performed or game launch requested in this task. If human
review authorizes follow-up, fold this into the pending exact-artifact
integrated regression instead of creating broad research infrastructure.

1. Establish the ASI/game identities and diagnostics setting, comparable scene,
   resolution/aspect and camera configuration. Keep shader/loading warmup and
   startup toast outside the closed-empty steady sample.
2. Compare vanilla, current production with overlay closed/notifications empty,
   and the same ASI with overlay open. Vanilla-vs-ASI combines camera and overlay
   cost; it **cannot isolate** closed-overlay overhead. Use scoped counters for
   HookPresent/Renderer::Render/WindowProc versus camera callbacks to separate
   them. Open UI necessarily adds real draw/GPU work.
3. PERF-01: count eligible Presents, startup publications and settings Snapshot
   requests; aggregate time for settings acquisition and hotkey-config lock
   wait. Count requests by startup/font/semantic/UI/worker origin. SSO means
   snapshots must not be counted as heap allocations without observing them.
4. PERF-02: count window messages by keyboard/mouse/raw/other class, modifier
   queries and rebind-lock acquisition/wait. Compare passive closed input with
   mouse movement; preserve ordinary original-WndProc time separately.
5. PERF-C01: aggregate callback frequency and lock wait/hold distributions for
   dialogue, camera state and baseline in a representative camera transition
   with overlay closed then open. Separate logging time. Start diagnostics off;
   only one short enabled segment if needed to evaluate opt-in diagnostic tails.
6. Record counters/timers in memory and emit one bounded summary outside the hot
   paths; do not add per-hit file output that manufactures the bottleneck being
   measured. Conditional WaitForFrame count/duration can accompany the open
   segment, but no separate GPU profiling campaign is justified by source alone.

**Disposition rule:** if attributable costs stay below repeatability/noise and
locks are uncontended, keep accepted simple ownership and close candidates as
non-material. A repeatable cost/tail changes repair priority; propose only that
path's repair. No universal percentage threshold or synthetic timing is claimed.
The cinematic callback hypothesis is investigated only if real callback/log
counts in this session make it relevant.

## 7. Original pre-repair ordering (superseded by Section 9)

This was the proposed sequence before the repair follow-up was authorized.
Items concerning PERF-01 and PERF-02 are complete as recorded in Section 9;
they are not pending work. The remaining measurement question is PERF-C01:
investigate it only if lock waits or transition tails are repeatable, and scope
logging/critical-section duration before publication redesign. Other speculative
optimizations remain unprioritized without measured materiality.

## 8. Stop condition / handoff

Audit complete at the static source/report level. Only this report was added.
No source, configuration, installed game files, artifacts or packaging were
changed. No optimization, new instrumentation, test/build rerun, Git operation
or game launch occurred. The inherited offline baseline is reported as inherited,
not fresh performance validation. Material runtime performance and current ASI
integrated runtime behavior remain unmeasured/unvalidated respectively.

Stop here for human review. Findings do not reopen architecture F-01–F-07 or
authorize implementation/release. A subsequent measurement/repair scope needs
its own approval and evidence.

## 9. Approved bounded repair follow-up — 2026-09-26

The follow-up repair was explicitly authorized after the static audit. This
section supersedes the pre-repair stop-state above for the source and offline
validation recorded here; it does not change the audit's original evidence
boundary or establish runtime performance.

### PERF-01 — repaired; offline validated

- Startup settings are acquired only while the once-only startup-hint gate is
  unpublished and renderer/present prerequisites hold. Failed/unavailable reads
  remain retryable; after successful publication the gate prevents further
  startup-only snapshots.
- A ready visible/toast frame reuses one `RuntimeSettingsSnapshot` for the
  startup hint (when still pending), font/profile/locale update, and UI controls.
  A hidden empty-notification frame still returns before a non-startup settings
  read.
- The semantic projection deliberately retains its separate reads of only the
  five configured gameplay/cinematic/dialogue atomics. This preserves its
  independently sampled state contract and does not merge it with active or
  observed camera facts. The ordinary full UI snapshot remains separate from
  those observed semantic facts for the same reason.
- `renderer_state_harness` proves prerequisites, retry after unavailable
  settings, terminal publication, and no later startup read/retrigger. The
  complete harness suite passes below. No runtime cost/FPS claim is made.

### PERF-02 — repaired; offline validated

WindowProc classifies the four keyboard/system-key down/up messages before
modifier queries or `HandleRebindMessage`. Only key-down classes query
Ctrl/Shift/Alt. Non-key messages pass through without entering the rebind mutex;
`HandleRebindMessage` retains its own early classification guard. All keyboard
capture, modifier rejection, Escape, consumed-key, toggle, mouse/raw-input,
original-WndProc, and Auto-language paths remain intact. The input harness
covers all four key classes plus mouse, raw-input and character-message
rejection and unchanged active-rebind state.

### PERF-C01 — instrumentation ready; runtime measurement pending

No mutex ownership, lock order, publication strategy, callback logging behavior
or source freshness was redesigned. With `Diagnostics.Enabled=false`, the
instrumentation performs only the existing diagnostics gate and takes the same
mutex directly; it does not read the clock or update telemetry counters. With
diagnostics enabled, scoped lock measurement records wait and hold durations
using monotonic timestamps; aggregate atomics are updated after the measured
mutex is released. Synchronous `Log` duration is accumulated locally and
published after unlock, only when that log call occurs inside an instrumented
critical section.

In-memory telemetry covers gameplay-camera callback samples, dialogue samples,
camera-state publications, visible semantic-snapshot reads, lock acquisitions
for dialogue/camera-state/FOV-observation/gameplay-baseline/aspect-restoration
stores (including UI readers of those same stores), and cinematic aspect
callbacks, viewport queries and log-emission attempts. Wait/hold/log output
includes count, average, p50/p95 histogram upper bounds, and maximum. A first
sample-to-summary elapsed duration is included. No per-hit file logging,
polling, lock replacement or unbounded trace buffer was added. On the first
overlay open while diagnostics are enabled, the existing hotkey worker emits
one bounded `PERF_RUNTIME_SUMMARY` line to the runtime log outside those lock
paths. Summary data is cumulative from first instrumented sample to request;
this is a compact diagnostic snapshot, not a statistical sampling framework.

The focused performance telemetry harness verifies diagnostics-off silence,
diagnostics-on event/lock/log fields, elapsed field presence and one-shot
summary behavior. PERF-C01 remains **MEASUREMENT PENDING** until the integrated
game run evaluates repeatability/noise and actual wait/hold tails.

### Offline validation and remaining runtime procedure

- At this repair follow-up, the full repository runner recorded **44/44
  harnesses PASS**, 0 XFAIL; it included the startup gate, message-class and
  telemetry coverage. This is the historical result for that run.
- At this repair follow-up, localization/resource/glyph audit recorded
  **PASS**, 18 locales and 162 keys. This is the historical result for that
  run, not the latest catalog size.
- Production `build.cmd`, diagnostic `build-diagnostic.cmd`, and supported
  production compatibility alias `build-overlay-settings.cmd`: **PASS**.
  Compiler warnings observed were in vendored Zydis/ImGui files only; no new
  project-owned warning was emitted.
- No game launch, release packaging or Git operation was performed. Architecture
  contracts F-01–F-07 remain closed; no FPS/frame-time improvement is claimed.

### Later verification snapshot — 2026-09-26

After the repair follow-up, the current runner and localization audit scripts
were rerun in audit-only mode. Runner registration/compile/run-set audit:
**45 sources / 45 compiled / 45 executed**, sets equal, repository inventory
complete. Localization/resource/glyph audit: **PASS**, 18 catalogs and 182
keys, including parity, placeholders, UTF-8, embedded resources and glyph
coverage. These checks establish current registry/catalog consistency; they do
not replace a fresh execution of all harness binaries or runtime visual and
performance evidence. The 43/43 and 44/44, 162-key figures above remain valid
as historical run outputs, not current totals.

For the later integrated regression, first run a diagnostics-OFF baseline with
the same scene/configuration and overlay closed after startup toast expiry;
capture repeatable frame-time/CPU behavior externally. Then set
`[Diagnostics] Enabled=true` and restart the same built artifact for a separate,
short diagnostics-ON run. After warmup, keep the overlay closed while repeating
the representative camera and dialogue transitions. Open it once at the end of
that sample to request the one-line aggregate, then collect
`PERF_RUNTIME_SUMMARY` from `STALKER2CameraTweaks.log`. Compare the enabled run
separately because diagnostics/logging add overhead. Use elapsed duration,
callback counts, path acquisition counts, wait/hold percentiles/maxima and
lock-held synchronous-log time to decide whether PERF-C01 is below measurement
noise or merits a separately authorized targeted repair. Do not infer an FPS
gain from the offline results.
