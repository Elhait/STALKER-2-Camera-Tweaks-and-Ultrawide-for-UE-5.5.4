# Canonical ASI filename startup investigation

## Active disposition — startup nondeterminism (2026-09-28)

This section supersedes the earlier filename/entry-bypass interpretations and
their proposed next trace runs below. They remain historical investigation
records, not current causal findings.

The authorized current task is read-only root-cause architecture analysis.
Its separate report is
[Overlay startup nondeterminism](../reports/OVERLAY_STARTUP_NONDETERMINISM_ROOT_CAUSE_2026-09-28.md).
No production repair, build, game launch, Git mutation or packaging was performed
in this step. The only edits are the report and this plan.

Confirmed observations:

- The same trace-v2 artifact, SHA256
  `90E34AC4D0262B8D47AA751475339C2653CE0FE78F111576D430C6DE14030019`,
  has canonical-name FAIL and PASS outcomes; a later canonical launch failed
  again. Basename is not a deterministic causal condition.
- Latest FAIL is PID 3828, processCreateUtc `134350185655092188`:
  arm `00:36:05.861`, core ready `00:36:07.378`, toggle `00:39:47.566`.
  All three system exports were armed; core Gameplay/Cinematics/Dialogue are
  AVAILABLE; no factory callback was recorded and Overlay did not activate.
  Trace capacity 128/event count 113 does not indicate buffer overflow.
- PASS and FAIL core-ready snapshots can both route system exports through
  local DXGI. PASS nevertheless reaches our callback from local DXGI. Entry
  replacement alone does not establish downstream bypass.
- PASS raw logs are no longer available after an earlier mistaken deletion
  without backup. Their recorded timings/results remain in this plan/chat;
  they are not a fresh raw-log comparison. Preserve subsequent evidence.

Material provenance correction:

- Actual trace process birth times are `00:17:46.2039504` (alternate PASS),
  `00:27:37.9796341` (canonical PASS), `00:36:05.5092188` (latest FAIL),
  converted from GetProcessTimes FILETIME using the local timezone.
- Supplied ReShade initialization/first-factory entries precede those process
  births by several seconds. Therefore the earlier same-process comparisons
  of ReShade redirects versus our arm are withdrawn. A different/parent game
  instance is plausible, but its identity is not established.
- ReShade 6.8.0's logger uses a fixed path with CREATE_ALWAYS and read-only
  sharing; bracketed IDs are thread IDs, not PIDs. Same-run wall-clock proximity
  is insufficient to assign that file to the ASI process.

Confirmed source properties:

- Our synchronous early arm is inside a detached DllMain-created worker,
  not a handshake before plugin loading returns.
- SafetyHook originals capture the entry route at setup. ReShade/MinHook
  separates trampoline capture from queued patch publication; their manager
  locks do not serialize a transaction with our manager.
- Our factory-table registry is reached only after a covered factory export
  callback. There is no independent seed or recovery of already-created
  swapchains. `hook enabled` therefore does not prove useful dispatch coverage.
- Log contents are not read to control discovery. Log I/O can perturb timing;
  deleting files before one PASS does not establish causality. Core logger
  initialization failure is a separate source possibility, not an explanation
  for this FAIL with camera components AVAILABLE.

Current inference, not a proved writer event: foreign original capture before
our enable followed by publication afterward can yield wrapper-to-old-route
FAIL, while capture after our enable preserves our callback and yields PASS.
Missing the useful pre-arm creation is a secondary possibility. Exact foreign
saved targets and historical writer stacks remain unknown.

Next architectural candidate (proposal only): bounded independent factory-table
bootstrap through the existing registry, so export callback delivery is not the
sole discovery prerequisite. Preserve all delayed renderer/input activation
and FS-01–FS-05 contracts. Shared-table coverage, module/table lifetime, nested
observation deduplication and pre-existing-swapchain limitations are mandatory
gates. This is not proof of universal coverage or authorization to implement.

Source plus traces suffice to start a separately authorized bounded batch;
another mechanical trace run is not a prerequisite. Debugger capture remains
necessary for exact historical attribution or an unresolved coverage gate.
No Sleep, polling, filename workaround or blind reattachment is proposed.

## Authorized scope

The user reports that identical ASI bytes, loaded as the only mod ASI, activate
Overlay under `STALKER2CameraTweaks-FS01-FS05-candidate.asi` but not under
`STALKER2CameraTweaks.asi`. Treat basename/loading chronology as the changed
variable. Preserve early observation, pending-target validation, delayed
renderer/input activation and FS-01–FS-05. The required eventual production
contract is successful activation under `STALKER2CameraTweaks.asi`.

This step instruments the current path; it does not repair or redesign it.
No sleeps, new interception hooks, renamed-binary workaround, game launch,
release/package actions or Git mutations.

## Evidence and limits

- Installed dsound.dll identifies as Ultimate ASI Loader x64 9.7.4; local
  d3d12.dll is OptiScaler 0.9.5-pre4 (8dac650), dxgi.dll is ReShade 6.8.0.
  The directory also contains UE4SS's dwmapi proxy and the S2AE winmm proxy.
- UAL v9.7.4 source FindFiles uses FindFirstFileW/FindNextFileW and LoadLib,
  without an ASI basename sort or a CameraTweaks-specific branch in that path:
  https://github.com/ThirteenAG/Ultimate-ASI-Loader/blob/v9.7.4/source/dllmain.cpp
- Local OptiScaler.ini specifies LoadAsiPlugins=auto (documented default false)
  and describes a -loadlate filename suffix. Neither reported basename has it.
  Matched OptiScaler source (8dac650, OptiScaler/dllmain.cpp:210–269) iterates
  the configured plugin directory without sorting and defers names containing
  -loadlate; it contains no CameraTweaks-specific basename branch in this path:
  https://github.com/optiscaler/OptiScaler/blob/8dac650/OptiScaler/dllmain.cpp
  Actual effective plugin loading and the ASI load caller remain unestablished.
- Our DllMain creates InitializeThread asynchronously. Synchronous early-arm
  happens inside that worker before camera initialization, not before DllMain
  returns to the loader. The relative order versus native factory creation is
  currently unproved. Current Overlay logs contain hooks-armed/core-ready but
  no factory observation; this is a coverage gap, not proof of no factory call.
- ReShade.log records delayed system DXGI hooks and CreateDXGIFactory1 activity;
  existing Overlay records have no clock, preventing exact correlation.
- In the supplied trace, `CreateDXGIFactory` was already an E9 relative jump
  before our hook was armed; our arm changed it to another E9, and core-ready
  recorded a third E9 at the same export address. The same pattern occurred
  for Factory1/2. This confirms an earlier hook and a later entry rewrite, but
  does not identify either destination's owner or prove whether the later chain
  bypasses our trampoline. The process had exited, so target memory cannot be
  recovered from that log.
- ReShade v6.8.0 installs delayed hooks after its LoadLibraryExW trampoline
  returns. Its factory wrapper uses its saved hook/trampoline lookup. A later
  patch of the system export may therefore not observe that route; actual
  ordering/bypass in this process is still a hypothesis, not a confirmed cause:
  https://github.com/crosire/reshade/blob/v6.8.0/source/hook_manager.cpp
  https://github.com/crosire/reshade/blob/v6.8.0/source/dxgi/dxgi.cpp

## Instrumentation plan

Opt-in CAMERA_TWEAKS_STARTUP_TIMELINE=1 adds only startup telemetry and retains
the canonical build output name. With the flag absent, instrumentation is
compiled out. Expected edits: build.cmd; diagnostic startup timeline helper;
plugin entry/startup/one-shot key probe; existing discovery export hooks;
focused diagnostic fixture and this plan.

Capture DllMain entry and its bounded loader-call stack, worker entry, arm
begin/completion, per-export availability/entry bytes before and after enable,
core-ready and first observed factory entry/return. Use QPC and wall-clock
anchors outside DLL notification callbacks. Relevant subsequent module load
notifications copy bounded names/base addresses into a fixed buffer only;
no callback logging, allocation, locks or calls into another DLL. TSC in such
notifications is auxiliary evidence, not a clock for production decisions.

The first canonical-name runtime trace confirmed all three system DXGI factory
entry points contained our jump immediately after arm and different jumps at
core-ready, while the callbacks were never observed. A toggle snapshot still
showed those later jumps. The exact writer and whether its chain preserves our
trampoline remain unknown. Instrumented trace schema v2 now follows up to four
read-only E9/EB/FF25 jump links at the existing pre-arm, armed, core-ready and
one-shot toggle snapshots, recording target module ownership or executable
memory-region details. It does not follow arbitrary instructions and does not
poll Present. First-write time remains bracketed by snapshots; an exact write
timestamp would require an external debugger data breakpoint or intrusive
monitoring, so no continuous production probe is added.

Baseline module snapshots identify modules already present when captured;
they do not invent past load times. Register notifications at worker entry,
explicitly documenting the earlier blind interval. Drain outside loader lock
after arm/core-ready and once on the configured toggle key, even if Overlay
never activates. No recurring logging, discovery polling or presentation work
is added. Truncation, capacity overflow and registration failure are explicit.

## Acceptance and validation

- No lifecycle decisions, originals, hook targets, camera settings, cleanup,
  readiness gates or filename semantics change.
- Diagnostic fixture covers bounded buffer publication, overflow, first-event
  limits, module notification ABI/copying and trace output; execute relevant
  existing callback/startup/lifetime fixtures and build the canonical ASI.
- Inspect the new diff and whitespace; runtime chronology remains pending user
  validation. Preserve the previous root ASI in ignored build-artifacts before
  producing the instrumented canonical ASI.
- User runs the canonical filename with the same graphics stack, presses its
  toggle once and returns startup/Overlay/core/ReShade logs. If factory creation
  bypasses our existing hook, debugger breakpoints at the recorded actual
  exports are the bounded next evidence step; absence in our trace alone must
  not be described as an absent native call.

Instrumentation adds bounded work and can perturb the measured startup race.
A successful instrumented launch alone is not a repair or a confirmation of the
uninstrumented canonical release contract. ReShade logs should be preserved for
the same process/run; do not correlate a previous run's wall-clock records.

## Historical disposition — superseded by active disposition above

Runtime trace v2 received for canonical basename; an export-route change is
observed, but decisive downstream dispatch and exact writer timing remain
unresolved. No production behavior repair has been attempted.

Runtime trace disposition (2026-09-28):

- Before our arm, all three system DXGI exports already jumped through private
  relay memory to `gameoverlayrenderer64.dll` (Steam Overlay).
- Immediately after our arm, the system exports jumped through the SafetyHook
  relay allocation at `0x7FFE129A0000` to callback addresses in this ASI.
- At `camera_core_ready`, all three system exports instead jumped through
  private relay memory to the game's local `dxgi.dll` wrapper. Its destinations
  lie inside that loaded module; the directly decoded entry chain no longer
  shows our callbacks, without establishing downstream trampoline dispatch.
  This was initially interpreted as explaining missing `FACTORY_ENTER`.
  Subsequent PASS evidence disproves that inference from entry bytes alone;
  downstream callback reachability cannot be inferred from this snapshot.
- At the later toggle snapshot, the wrapper path had an additional private
  relay into the local `d3d12.dll` proxy (OptiScaler). The corresponding module
  load notification occurs after the core-ready snapshots, so this later path
  is not evidence identifying the earlier export writer or downstream bypass.
- The trace establishes the export route and its wrapper destination, not which
  component performed the write or why the ASI basename changes the observed
  result. First rewrite time remains bracketed by arm and core-ready snapshots.
- No same-run ReShade.log was supplied for this trace; no precise ReShade event
  time is attributed to this process.

- Full test.cmd passed 52/52, including localization (18 catalogs/182 keys),
  embedded resource and glyph/font validation.
- Latest focused startup fixture passed, including real Windows DLL load/unload
  notification registration, ABI and bounded copied payload checks.
- Existing DXGI callback regressions also passed with timeline instrumentation
  enabled. Notification callback disassembly contains no foreign-module calls.
- Canonical trace-enabled build and flag-off production-check build passed.
  Flag-off binary differs from the preserved previous production binary only
  in four PE timestamp bytes; executable code is unchanged.
- git diff --check passed. Existing unrelated/FS repair changes were preserved.
- Trace schema v2 follows up to four E9/EB/FF25 destinations at existing
  snapshots and records their owner/region metadata. Canonical-name build
  passed; SHA256 90E34AC4D0262B8D47AA751475339C2653CE0FE78F111576D430C6DE14030019.
  The prior trace artifact is preserved in
  build-artifacts/startup-basename/previous-timeline-v1/STALKER2CameraTweaks.asi.
- No game launch, game-directory write, Git mutation, release or packaging.

Next decision: determine whether to implement a proxy-aware hook activation
that reaches the observed factory route while preserving existing ownership
and failure-isolation contracts. To explain why basename changes the outcome,
compare a successful run's chain/load chronology or capture the first writer
with a debugger data breakpoint; do not treat the filename difference itself
as the mechanism. An exact first-write time remains an external-debugger task.

## Controlled basename comparison artifact

Prepared an exact copy of the trace-v2 canonical binary under the previously
successful basename for the requested A/B. Both files have length 2,764,800
and SHA256
`90E34AC4D0262B8D47AA751475339C2653CE0FE78F111576D430C6DE14030019`:

- Canonical: `STALKER2CameraTweaks.asi`.
- Comparison copy: `build-artifacts/startup-basename/candidate-trace-v2/STALKER2CameraTweaks-FS01-FS05-candidate.asi`.

No game-directory writes or game launch were performed. Runtime comparison
remains user-run. For a clean comparison, run the copy as the only mod ASI in
the same graphics configuration, then return the new startup, Overlay and core
logs. Compare module chronology, pre-arm/armed/core-ready chains, and whether
`FACTORY_ENTER`/Overlay activation occurs. Preserve the original canonical
trace-v2 run as the failing baseline.

## Successful alternate-basename trace comparison (2026-09-28)

The user supplied startup, Overlay, core and ReShade logs from the successful
run using the byte-identical alternate-basename trace-v2 ASI. The core log
reports mod SHA256
`90E34AC4D0262B8D47AA751475339C2653CE0FE78F111576D430C6DE14030019`, matching
the preserved canonical trace artifact.

Confirmed in the successful trace:

- `early_arm_complete` is at `00:17:46.559`; `camera_core_ready` is at
  `00:17:47.719`. The three system DXGI exports have the same kind of later
  wrapper route at core-ready as in the canonical failure trace: private relay
  to local `dxgi.dll` entries. Thus that export-entry snapshot alone does not
  establish whether our callback remains reachable downstream.
- Despite those core-ready bytes, `CreateDXGIFactory1` enters our callback at
  `00:17:51.348` and returns `S_OK` with a factory pointer. The recorded caller
  return address resolves to the game's local `dxgi.dll`. Overlay logs then
  show factory observation/interface hooks, successful Present observations,
  renderer activation after two successful presents, and visibility changes.
  Overlay is therefore confirmed working in this run.
- ReShade.log shows a `CreateDXGIFactory1` redirect at `00:17:43.768`.
  Correction: this precedes trace PID 28604's birth at `00:17:46.2039504`.
  It must not be attributed to that process or used to establish its arm order.

Updated interpretation: a later rewrite of the system DXGI export entries is
not by itself sufficient to explain the canonical failure. In the successful
run, the entries have the wrapper route at `camera_core_ready`, yet a later
call from local `dxgi.dll` still reaches our hook. The distinguishing
mechanism is downstream dispatch/trampoline topology or its initialization
ordering, not merely the bytes at the system export checkpoint. The trace
supports an ordering/trampoline-capture hypothesis, but does not yet prove
which component captured which target or why the ASI basename changes that
topology. No production code was changed.

The bounded next evidence step is a canonical-name run with contemporaneous
ReShade.log retained and the same trace enabled; if it still misses the
callback, use a debugger data breakpoint on the relevant export/relay write
and capture the writer stack. Compare the downstream target chain at the
first factory call, not only system-export snapshots. Do not add startup
delays or implement a proxy-aware production repair until that mechanism is
established.

## Canonical-name runtime succeeds (2026-09-28)

The user supplied a later successful run using the canonical
`STALKER2CameraTweaks.asi` with contemporaneous startup, Overlay, core and
ReShade logs. The core runtime identity reports the same mod SHA256
`90E34AC4D0262B8D47AA751475339C2653CE0FE78F111576D430C6DE14030019` and the
same game SHA256 as the prior trace-v2 runs.

Confirmed for this run:

- ReShade initialized at `00:27:34.353`, logged delayed system-DXGI hooks at
  `00:27:34.524` and a first Factory1 redirect at `00:27:35.236`.
  Correction: all precede trace PID 17696's birth at `00:27:37.9796341`.
  They do not establish same-process ordering relative to arm `00:27:38.328`.
- At `camera_core_ready` (`00:27:39.875`), system factory exports again show
  the wrapper route through local `dxgi.dll`, matching both prior PASS and
  FAIL snapshots. `CreateDXGIFactory1` then entered our callback at
  `00:27:40.449`, with its caller in local `dxgi.dll`, and returned `S_OK`.
- Overlay observed factories and successful Presents, activated its render
  target after two successful Presents, and emitted visibility changes. The
  canonical filename therefore satisfies the runtime activation contract in
  this run.

This adds a canonical-name PASS alongside the earlier canonical-name FAIL,
using the same trace-v2 hash. Consequently basename alone is not a sufficient
or deterministic explanation of the outcome. The successful candidate and
successful canonical traces have the same coarse export snapshots; the former
ReShade/arm comparison is withdrawn by the process-provenance correction above.
The exact factor that differed in the
earlier failure remains unknown; startup timing/race or unrecorded downstream
state remain hypotheses, not findings. No production code was changed.

Disposition: retain the canonical PASS as evidence that the required filename
can work with this graphics stack, but do not call the earlier failure fixed
or claim reliability from one successful launch. Further runtime comparison
is useful only if it can identify what changed across the canonical FAIL and
PASS (for example, a contemporaneous ReShade log plus a captured downstream
target/writer event). Do not change hook design or add delays based solely on
the filename correlation.
