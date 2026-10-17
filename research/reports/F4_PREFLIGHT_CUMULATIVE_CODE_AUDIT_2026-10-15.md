# F4 Preflight — Comprehensive Cumulative Code Audit

Date: 2026-10-15  
Mode: source and existing-evidence review; implementation not authorized  
Audit gate: **BLOCKED**  
Audit execution: completed for the production integration surfaces described below

Subsequent approved remediation: R1–R7 implementation and offline validation are
recorded in `F4_PREFLIGHT_REMEDIATION_IMPLEMENTATION_REPORT_2026-10-15.md`.
The blocked verdict below describes the audited baseline and is preserved;
it is not the status of the subsequently remediated tree. Live qualification
of the new candidate remains pending. Historical findings/evidence are unchanged.

## 1. Executive summary

The current tree has seven source-proven production defects, one additional
evidence-backed subclass integration risk, and a residual containment gap in the
secondary CTest entry point. No Critical defect was established. One confirmed
defect is High severity: exception unwinding can free BCrypt's backing buffer
before destroying the hash object that uses it.

The remaining confirmed defects concern Overlay startup/activation/input
readiness, partial graphics initialization, and preservation of compatible INI
values during canonical rebuilding. These are concrete paths through the
current implementation; they are not inferred from an earlier PASS or from the
reported startup freeze.

The latest focus change correctly stops treating `WM_KILLFOCUS` alone as
application deactivation. It does not repair the separate initialization window
in which an activation event can be missed, or reconcile a stale activity cache
when the user later requests the menu.

Recommended disposition: repair PF01–PF07 before the next qualification ASI
build/live session. Review PF08's subclass race and either address it or record
an explicit disposition. PF09 concerns secondary CTest execution; the canonical
Job Object runner already supplies containment and can be used for subsequent
authorized focused validation. No new broad architecture campaign is needed.

No source, tests, build scripts, installed files, release artifacts or prior
reports were edited by this audit. This report is the sole intended new file.
No build, harness, full suite, game, graphics fixture, ETW or PresentMon was run.

### Deduplicated matrix

| ID | Subsystem | Severity | Classification | Confidence | Qualification effect |
| --- | --- | --- | --- | --- | --- |
| PF01 | SHA-256 exception cleanup | High | Confirmed defect | Source + BCrypt lifetime contract | Blocks source preflight |
| PF02 | Overlay activity recovery | Medium | Confirmed defect | Explicit startup/event interleaving | Blocks reliable menu qualification |
| PF03 | Startup Auto-language handshake | Medium | Confirmed defect | Request precedes consumer installation | Blocks startup contract |
| PF04 | Menu display/input admission | Medium | Confirmed defect | Actual owner-command/readiness ordering | Blocks committed-menu admission contract |
| PF05 | Duplicate-section canonical rebuilding | Medium | Confirmed defect | Deterministic transformation trace | Blocks settings-preserving migration |
| PF06 | Legacy Gameplay enable migration | Medium | Confirmed defect | Loader/template precedence contradiction | Blocks settings-preserving migration |
| PF07 | Partial ImGui D3D11 backend cleanup | Medium | Confirmed defect | Failed-init/finalizer path | Blocks partial-resource cleanup contract |
| PF08 | Concurrent foreign subclass installation | Medium | Evidence-backed risk | Source exposes a race; competing injector schedule unverified | Requires focused disposition |
| PF09 | Secondary CTest containment | Medium | Confirmed infrastructure gap | Direct CTest commands lack project supervisor | Residual FP02 surface; do not use as a contained runner |

PF09 is a residual execution surface of the original FP02 concern, not an
additional independent production defect. PF01 differs from the original FP10:
raw-handle leakage was repaired; the resulting scoped owners have a separate
destruction-order error. PF05 differs from FP15: persistence now updates duplicate
assignments, but startup canonicalization can subsequently insert conflicting
defaults. Historical FP01–FP16 results are preserved rather than rewritten.

## 2. Baseline and review scope

### Identity and Git state

- HEAD: `e756442bcf9942923b4531fb513af893f6aecae7`.
- Git index SHA-256 at the review snapshot:
  `72FAC39B216993275F82BCB64A7032F1098FA30F8B3D09F105F97AED6404CD19`.
- The initial status exposed staged and unstaged work. At the later snapshot,
  the inspected current source matched its index representation; for example,
  `config_template.cpp` contained 1,229 lines in both. This is a description of
  observed state, not an assertion about who staged earlier work. The audit
  issued no stage/unstage/reset/checkout/clean command.
- Ordinary working-tree `git diff` returned `fatal: this operation must be run
  in a work tree`, although status/rev-parse/index reads worked. Cumulative
  staged diffs, current files and direct index/current-content comparison were
  used instead. The Git environment was not repaired or reconfigured.
- A read-only SHA-256 snapshot of 394 existing tracked source/vendor/header,
  test, build, locale and contract files was taken before report creation for
  end-of-task preservation checking. Subsequent Git reads disable optional locks.

The explicit production registry still contains **61 translation units: 54
project-owned and seven vendor**. The current JSON registry declares **59
harness targets and 62 ordered invocations**. These are static inventory facts,
not a fresh runner-audit or test PASS. R9's historical canonical result covered
104 adapter scenarios in OFF/ON, or 208 children, plus four Zoom fixture runs;
older 204-process counts must not replace that evidence identity.

Selected current-source SHA-256 identities (the findings use these working-file
contents, not HEAD alone):

| Input | SHA-256 |
| --- | --- |
| `src/platform/win32/sha256.cpp` | `66B56C3992F6B032F91570D3F77CB1E77D3F136DC265AB7E0CDF714DEBCEFF86` |
| `src/config/config_template.cpp` | `C9A8B1D7D253B3847CF0327E7D86BD1A6BF0B7083890375F7AC34FC5257959A5` |
| `src/config/config_repository.cpp` | `6A277B41E252A5DAFE537EB9F2E9D869EA13BF7FD7509C11C31CD04BF4ED8358` |
| `src/overlay/composition/composition_runtime.cpp` | `4AEBED2618C5A28B755B2C841760E0EE63E3CAC98830B24820C7C194875FEC4C` |
| `src/overlay/presenter/renderer_runtime.cpp` | `3FE135CB69C2E5B7F85A1F25F14F2B8738126A0B95148D4FF86AC3E07A2D3C3E` |
| `src/overlay/presenter/renderer_runtime.hpp` | `F73933B069ACD48A309CB792DE410437BCAD7B8E77BD9594830434783B1A1BCE` |
| `src/overlay/presenter/presenter_runtime_policy.hpp` | `3CEB5CB9CA6FFE245236D9A25B7E9801FFADEA64FCD6759543B29E612E2D58E6` |
| `src/plugin/runtime.cpp` | `BAC946DB879AF9F8A5115C36EA8AD7BC1C5A0118721A461E9AD4746D60735A4D` |
| `CMakeLists.txt` | `DFD46C44A1630758FD1CF80943A47FB621CE5146F715B1D34D1603018FFB2AC2` |
| `tests/test_harnesses.json` | `97917F4270A9166E1CC5E55B4555A124B842DAE22C41F2C2D86F970CA11F8410` |

### Reviewed production boundaries

| Area | Current files and paths reviewed | Depth/limit |
| --- | --- | --- |
| Entry/startup/reset | `plugin/dll_entry.cpp`, `runtime.cpp`, `worker_lifecycle.*`, `optional_overlay_startup.hpp`, `native_callback_admission.hpp`, `runtime_internal.hpp` | Actual entry, readiness, installer/failure/worker/reset paths; experimental branches distinguished from ordinary production |
| Settings | `runtime_settings.*`, `camera_correction_gate.hpp`, setting transactions in `camera_writer_transaction.hpp` | Typed mutation, admission, camera locking, producer ordering, persistence and readback |
| Native hooks | `hooks/signature_scanner.*`, `instruction_validator.*`, signatures, `hook_set.*`, production installers/adapters and local SafetyHook changes | Match refusal, bounded instruction windows, component commit, native continuation and executable retention; no exhaustive re-certification of upstream decoder |
| Camera | `camera/horplus.*`, `fov_observation.*`, `gameplay_baseline.*`, `gameplay_aspect_restoration.*`, `camera_state_snapshot.*`, `presentation_state.*`, `camera_state_publication.hpp` | FOV domains, baseline/source/freshness and snapshot publication |
| Gameplay | `gameplay/gameplay_camera.*`, `gameplay_state.*`, `aspect_policy.*`, `horplus_gameplay.*`, writer transaction header and runtime adapter | AspectRecalculation, HorPlus, RuntimeCombine, aspect readback, ADS/Zoom and recovery dispatch |
| Cinematics | `cinematics/cinematic_aspect.*`, `cinematic_fov.*`, `cinematic_selection.*`, `cinematic_initialization.*`, EXIT/recovery helpers, lifecycle transaction and runtime adapters | ENTER/EXIT selection, source/generation authority, continuation and Gameplay handoff |
| Dialogue | `dialogue/dialogue_state.*`, `dialogue_fov.*`, `dialogue_boundary_transaction.hpp`, runtime adapter | Candidate/active/exit/rearm, enable epochs, native pass-through and validated recovery |
| Configuration | `config/feature_config.*`, `config_repository.*`, `config_template.*`, `config_line_reader.hpp`, documentation generator | Complete startup/rebuild/persistence pipeline, duplicate/legacy/malformed input, newline/BOM and no-op paths |
| Platform | `platform/win32/memory.*`, `sha256.*`, `viewport.*`, `window.*` | Pointer checks, resource lifetime, game-window eligibility and viewport fallback |
| Overlay owner | `composition_runtime.*`, `owner_finalizer.hpp`, `stable_record_pool.hpp`, `optional_overlay_boundary.hpp` | Actual input callback → owner queue → renderer → finalizer, HWND discovery/rebind and wake paths |
| Overlay graphics | `renderer_runtime.*`, `imgui_d3d11_renderer.*`, `composition_presenter_state.*`, `renderer_state.*`, `presenter_runtime_policy.hpp` | Creation, draw/commit, attachment, show/hide, geometry/DPI, recovery and partial teardown |
| UI/localization | `overlay/presentation/*`, `overlay/localization/*`, `localization/locale_registry.hpp`, resource IDs/RC and 18 catalog inventory | Effective state projection, placement, font/catalog lifecycle, placeholders and Auto-language integration; translation quality and accessibility are not visually certified |
| Diagnostics | `diagnostic_runtime.*`, continuation/source traces, `performance_telemetry.*`, startup journal and runtime call sites | Runtime gate, observational ownership, bounded storage and exception containment; no new timing measurements |
| Build/release/tests | `CMakeLists.txt`, source/test registries, `build.cmd`, `test.cmd`, build/publish/package tools, process supervisor, runners and relevant harness assertions | Actual inputs, flags/resources, isolation, exit propagation, containment, package allowlist and identity limits |

This is a cumulative integration audit, not a claim of line-by-line verification
of all upstream ImGui/Zydis code or all historical research tools. Large
camera transaction headers were traced at their production entry, mutation,
source/epoch/recovery and publication boundaries. Historical prototypes are
outside the production registry and were used only as evidence where relevant.

### Existing evidence reconciled

Reviewed the current architecture/safety/resolver contracts; F1 audit and FP16
addendum; F2 R1–R9 execution record; F3 lifecycle report and packaging follow-up;
G1 finalization, G2/G3 gap, G4 regression and G5 compatibility reports; N01–N03
remediation and native evidence audit; and the consolidated N06 record.

G5's accepted live subject remains
`123D792A61A550892141DBEC12BB059A606BEA6A30B0C8B300491B6CAFC7DEF2`.
Read-only inspection found the repository-root and installed ASI both at
`B265DF72FF94A9BC05B8B9E57DA1C6B9F328530B19678D86098A4C6DDC64815F`,
3,587,072 bytes. These files were not replaced. Their identity does not prove
that the current source, including the last focus change, is in either file.
The preceding turn's truncated production-build output supplies no fresh PASS
for this audit.

## 3. Cumulative change impact

| Change group | Guarantees present in current source | Remaining issue found here |
| --- | --- | --- |
| G1–G5 | Separate display HWND/target; one graphics owner; input admission/heartbeat; owner finalization; no game Present/ResizeBuffers interception | PF02–PF04, PF07; PF08 needs a competing-subclass schedule |
| Latest focus change | `KeyboardFocusLost → Preserve`; real application deactivation still hides display and revokes input | PF02 is a separate missed-activation/reconciliation path |
| R1 | Canonical children enter kill-on-close Jobs before resume; finite wait/output budgets; negative exit handling | PF09 remains in the direct CTest path |
| R2/R3 | Optional owner exception boundary; truthful startup result; native admission/drain and correction rechecks | PF01's new scoped-buffer ordering; inherited N03/N05 limits remain |
| R4/F3 | Serialized ApplyAndPersist; checked complete reads; staged replacement; backup-before-recovery; compact comments; no-op persistence | PF05/PF06 occur during startup rebuild, not ordinary SetValue persistence |
| R5/R6/R7/R8 | Event-time modifiers; immutable language function pair; build-tree-local generated files/executables; own-HWND exclusion; bounded retained records | No new defect established in these repaired mechanisms themselves |
| R9 | Fixture opens callback admission before testing a native hook | No evidence justifying a new vendor patch or reopening the cache-flush hypothesis |

## 4. Confirmed production defects

### PF01 — BCrypt backing storage dies before the hash on exception

- **Severity/classification:** High / Confirmed defect.
- **Locations:** `src/platform/win32/sha256.cpp:93–95,109–110,129–147`;
  `tests/plugin/runtime_native_adapter_harness.cpp:1189–1200`.
- **Execution path/trigger:** after `BCryptCreateHash` succeeds with
  `hashObject.data()`, digest formatting at line 133 can throw. The existing
  test's checkpoint 3 at line 132 deliberately takes the same unwind path.
- **Mechanism:** `ScopedHashHandle hash` is declared before `hashObject`.
  Reverse C++ destruction therefore frees the vector first and only then calls
  the hash owner's destructor/`BCryptDestroyHash`. Microsoft's
  [BCryptCreateHash contract](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptcreatehash)
  requires that backing storage remain allocated until the hash is destroyed.
  Normal explicit `hash.Close()` has the right order; unwinding bypasses it.
- **Consequence:** violation of the native API lifetime contract, with possible
  freed-memory access/heap corruption during what should be contained startup
  failure. A real crash was not reproduced in this audit.
- **Why current guarantees miss it:** RAII closes the handle, but not before
  destroying the storage it borrows. The allocation fixture checks failure,
  digest preservation and exclusive reopening of the input file; it does not
  observe backing-buffer versus hash-destruction order.
- **Minimal fix:** declare storage before its dependent hash owner, or give one
  owner the buffer/hash with explicit destruction order. Preserve algorithm
  lifetime until after hash destruction. Do not remove allocation containment.
- **Focused regression:** instrument destruction order, or use a bounded child
  with guarded backing storage, and inject at checkpoint 3 and formatting
  allocation. Assert hash destruction occurs while the buffer is live, plus
  the existing digest/file-handle assertions, OFF/ON.
- **Regression risk/relationship:** low localized ownership change; follow-up
  to R2b/FP10, not a repetition of the repaired raw-handle leak.

### PF02 — Activity sampled before input-hook installation can remain stale

- **Severity/classification:** Medium / Confirmed defect in the recovery contract.
- **Locations:** `renderer_runtime.cpp:810–820,1337–1358,1588–1599`;
  `composition_runtime.cpp:667–675,712–740,988–998,1205–1211`;
  `presenter_runtime_policy.hpp:20–47`.
- **Execution path/trigger:** Renderer samples a non-game foreground window at
  initialization. The game becomes active while graphics/font initialization
  continues, before `InstallInputHook` at line 1207. Its activation/focus messages
  reach the original WndProc. The installed hook subsequently sees Delete.
- **Mechanism:** `hostActive_` remains false. Opening sets requested visibility
  and InputState focus, but never reconciles host activity against current host
  state. Rendering/reconciliation continues to use that cached false value.
  Only later `WM_SETFOCUS`/`WM_ACTIVATEAPP(TRUE)` can repair it. `WM_ACTIVATE`
  and an independent foreground transition source are not handled.
- **Consequence:** requested menu/toast can remain physically hidden while the
  game is already foreground. No further activation message is guaranteed if
  the game stays active. This is a concrete possible path to the observed
  `host_active=0`, `display_visible=0`, foreground-equals-game state; the exact
  message sequence in that captured game run remains unknown.
- **Why current guarantees miss it:** the focus fix prevents one wrong future
  deactivation; it does not reconcile an earlier missed activation. The current
  focused test asserts the semantic enum mapping, not actual hook-install/open
  ordering or recovery of a stale false cache.
- **Minimal fix:** establish an owner-thread reconciliation point after hook
  installation and before showing a requested menu, using a defined host
  activation predicate. Handle the relevant window/application activation
  transitions consistently. Preserve deactivation/minimize revocation; do not
  unconditionally force `hostActive_=true`.
- **Focused regression:** hold initialization after the initial inactive sample,
  activate the host before installing the subclass, then open without another
  focus message. Assert actual display/input readiness; also cover Alt+Tab,
  `WM_ACTIVATE`, genuine deactivation and minimize/restore.
- **Regression risk:** Medium; visibility policy must continue to keep the
  unowned display out of unrelated foreground applications.

### PF03 — Startup language request can be consumed before its handler exists

- **Severity/classification:** Medium / Confirmed defect.
- **Locations:** `renderer_runtime.cpp:858,869–870,1540–1570`;
  `composition_runtime.cpp:922–927,1205–1211`;
  `game_language_reader.cpp:206–209`; `renderer_runtime.hpp:91–98`.
- **Execution path/trigger:** Auto locale is unsynchronized. InitializeImpl
  posts `AutoLanguageSyncMessage` while preparing startup UI. The GameHwnd
  thread processes it before Renderer initialization returns and before the
  owner installs `OverlayWindowProc`.
- **Mechanism:** `PostMessageW` can succeed although the receiving game WndProc
  has no language-result handler. The request is marked outstanding. Subsequent
  preparation waits for synchronization and does not resend. With no accepted
  result, the startup hint stays pending and the scheduler can become idle
  because `startupAutoLanguageSyncRequested_` is already true.
- **Consequence:** startup hint never appears and locale synchronization does
  not complete until an independent action issues a new request. A successful
  post is not an acknowledgment of the consuming handler.
- **Why current guarantees miss it:** language publication and locale/presenter
  policy fixtures assume a request has reached the appropriate consumer; they
  do not run this production initialization ordering. Failed input-hook attach
  at line 1211 leaves the same consumer unavailable and has no attach retry.
- **Minimal fix:** make startup synchronization contingent on an installed,
  current GameHwnd input binding, then post once that prerequisite is committed.
  Define failure/cancellation of the handshake without an unbounded retry.
- **Focused regression:** an actual host WndProc consumes messages during
  renderer setup. Prove no synchronization request can be lost before subclass
  publication; cover post failure, attach failure, stale rebind generation and
  the existing English fallback.
- **Regression risk/relationship:** Medium; preserve GameHwnd native-reader
  affinity, Auto semantics and notification lifetime after accepted display.
  Independent of R5's repaired function-pair publication race.

### PF04 — Toast visibility can admit menu input before a menu frame commits

- **Severity/classification:** Medium / Confirmed defect.
- **Locations:** `composition_runtime.cpp:494–515,667–675,795–830,1303–1325`;
  `renderer_runtime.hpp:67–86`; `renderer_runtime.cpp:1741–1748`.
- **Execution path/trigger:** a startup or settings toast already has an attached
  visual and visible display HWND. The owner accepts an opening toggle.
- **Mechanism:** SetVisible marks the menu requested/dirty. Before the subsequent
  Render, ProcessOwnerInputCommands calls ReconcileOverlayInputReadiness.
  InputPresentationReady tests requested visibility and the toast's existing
  display/attachment, but has no evidence that the requested menu was drawn and
  committed. Input can therefore become active on a toast-only surface.
- **Consequence:** game input is consumed while the requested interactive menu
  is not yet visible. A stalled/failing next draw exposes the interval; the
  heartbeat supplies eventual fail-open only when relevant callbacks occur.
- **Why current guarantees miss it:** surface-generation readiness and window
  visibility are not proof of the current menu request's accepted frame. Pure
  admission/visibility predicates do not exercise toast-to-menu production order.
- **Minimal fix:** bind input readiness to acceptance of the current menu-visible
  request's rendered/committed state. Revoke that acceptance on close, host
  invalidation, terminal state and resource replacement. Keep the existing
  owner and fail-open callback path.
- **Focused regression:** open while a toast is visible, pause/fail the menu draw
  before Commit, and assert game input remains unconsumed. Capture may begin
  only after that menu frame commits; include close/reopen and stale requests.
- **Regression risk:** Medium; avoid turning ordinary harmless redraw dirtiness
  into unnecessary input loss, and preserve Delete while capture is unavailable.

### PF05 — Per-occurrence section repair inserts defaults over compatible values

- **Severity/classification:** Medium / Confirmed defect.
- **Locations:** `config_template.cpp:1031–1069,1100–1127`;
  `config_repository.cpp:194–207,358–390`;
  `tests/config/config_persistence_harness.cpp:809–850`.
- **Trigger/example:** a valid split configuration contains
  `[Overlay] / Enabled=false`, followed by another `[Overlay]` containing only
  `PositionX=100` and `PositionY=200`.
- **Mechanism:** repairSection creates a fresh `found` vector for each section
  occurrence and appends every absent managed default to that occurrence. The
  second occurrence gains `Enabled=true`, so the loader's later assignment
  overwrites the earlier compatible false. The same applies to modes, hotkeys,
  locales and other managed values distributed across repeated sections.
- **Consequence:** startup silently changes settings despite the F3 preservation
  contract. The resulting file may be perfectly idempotent with the wrong values.
- **Why current guarantees miss it:** R4a fixed SetValue persistence across
  duplicates. Its duplicate fixture disables synchronization; the placement
  fixture likewise exercises placement persistence, not full canonical rebuilding.
- **Minimal fix:** resolve managed presence/precedence across the complete
  logical section before inserting defaults; either merge occurrences while
  preserving accepted values/custom keys, or insert defaults only when absent
  globally. Do not reset values to obtain canonical formatting.
- **Focused regression:** pass split and conflicting repeated sections through
  the real synchronize → load pipeline; compare effective settings before/after
  and then bytes/mtime/identity on a second synchronization. Include false,
  nondefault modes, hotkeys, Auto/manual locale and placement.
- **Regression risk/relationship:** Medium because migration precedence is
  involved. Preserve R4a's complete-read rejection and multi-occurrence writes.
  This is a startup transform defect, distinct from repaired FP15 persistence.

### PF06 — Features.GameplayAspectFix=false is lost when defaults are appended

- **Severity/classification:** Medium / Confirmed defect.
- **Locations:** `config_repository.cpp:324–336`; `config_template.cpp:788–911,
  1017–1028,1138–1145`.
- **Trigger/example:** a recognized legacy INI contains
  `[Features] / GameplayAspectFix=false` and no canonical Gameplay.Enabled.
- **Mechanism:** the loader explicitly supports that legacy value. Template
  synchronization migrates Cinematics and Dialogue before defaults, but has no
  equivalent Gameplay enable migration. It appends a new Gameplay section with
  Enabled=true after Features. Loading the rebuilt file therefore ends at true.
- **Consequence:** an intentionally disabled Gameplay correction is enabled
  during upgrade. The input is recognized legacy syntax, so corruption recovery
  is not an explanation or an authorized reset.
- **Why current guarantees miss it:** migration round-trip cases cover
  Cinematics, Dialogue and the old Overlay hotkey; there is no matching
  legacy-Gameplay-before/after equivalence assertion.
- **Minimal fix:** resolve the legacy Gameplay enable input before inserting
  canonical defaults, with explicit precedence for a valid canonical setting.
- **Focused regression:** Features-only true/false, mixed canonical/legacy values
  in both orders, invalid legacy input and repeat-run no-op. Assert compatible
  settings survive; combine with PF05's logical-section repair where appropriate.
- **Regression risk/relationship:** Low to Medium; this is an existing migration
  omission exposed by reviewing the full pipeline. Introduction by the recent
  comment-format change was not established.

### PF07 — Failed backend initialization bypasses backend resource cleanup

- **Severity/classification:** Medium / Confirmed defect.
- **Locations:** `renderer_runtime.cpp:1083–1089,1480–1491,2636–2668`;
  `imgui_d3d11_renderer.cpp:169–179` (`Initialize`), `CreatePipeline`, `Shutdown`;
  `composition_runtime.cpp:1503–1524`.
- **Execution path/trigger:** ImGuiD3D11Renderer::Initialize stores its device and
  immediate context, then pipeline/font creation fails after partial allocation.
  BuildImGui shuts down the Win32 backend and returns before setting imguiReady_.
- **Mechanism:** DetachImGui sees an existing ImGui context but only calls the
  D3D11 backend's Shutdown when imguiReady_ is true. It destroys the context and
  clears flags without releasing the partially initialized backend. The Renderer
  object is process-resident and is not deleted by the one-shot finalizer.
- **Consequence:** backend device/context and any created pipeline resources
  remain resident; OwnedResourcesReleased reports false and can retain the
  display window as well. The report is truthful, but avoidable partial resources
  are left behind even when DComp detach succeeds.
- **Why current guarantees miss it:** success readiness is used as a proxy for
  resource ownership. Finalizer aggregation tests do not inject this real backend
  failure and observe its references.
- **Minimal fix:** call the backend's idempotent Shutdown for every attempted or
  owned initialization, independently of imguiReady_. Preserve context/font
  lifetime while invalidating its texture, then destroy the ImGui context.
- **Focused regression:** fail pipeline/font creation after device/context
  acquisition and after individual pipeline allocations. Exercise the actual
  BuildImGui → owner finalizer path and assert owned references are empty and
  display cleanup proceeds. Run as one bounded isolated child when authorized.
- **Regression risk:** Low to Medium; preserve one owner thread and the separate
  intentional retention policy for failed DComp detach/commit.

## 5. Evidence-backed risk and residual infrastructure gap

### PF08 — Temporary native continuation can skip a concurrent foreign subclass

- **Severity/classification:** Medium / Evidence-backed risk.
- **Locations:** `composition_runtime.cpp:1022–1058` and `869–884`.
- **Trigger:** another graphics/input injector subclasses the game HWND between
  Camera Tweaks' GetWindowLongPtr observation and SetWindowLongPtr replacement.
  The game processes a message before the returned actual predecessor is stored.
- **Mechanism:** the property initially publishes the observed predecessor.
  Before current-binding admission is published, the callback's pass-through
  branch already forwards through that record. If the true predecessor changed,
  a callback in this interval can bypass the intervening subclass. Stable record
  retention protects memory lifetime, not atomic installation of a native chain.
- **Consequence:** possible missed foreign-overlay/input processing during
  installation. No current NVIDIA/ReShade failure is attributed to this schedule.
- **Coverage limit:** prior foreign-subclass fixtures establish retained
  continuation after replacement, not a replacement between observation,
  installation and publication of the returned predecessor.
- **Recommended approach:** establish installation/removal serialization on the
  GameHwnd thread or an equivalent proven publication contract, preserving the
  nonblocking callback/owner boundary. Review that bounded contract before
  introducing a cross-thread wait or new mutex.
- **Focused validation:** force the intervening subclass and callback at the
  publication window; record the exact chain and prove each required predecessor
  is invoked once. Consider together with PF03's binding-ready handshake.
- **Disposition:** investigate with a deterministic fixture or explicitly accept
  the bounded compatibility risk; it is not a proved production crash.

### PF09 — Direct CTest commands bypass the contained canonical runner

- **Severity/classification:** Medium / Confirmed infrastructure gap, residual FP02.
- **Location:** `CMakeLists.txt:389–404` (the five direct `add_test` registrations).
- **Path:** invoking the generated CTest entries launches the adapter/Zoom
  executables directly, without process_tree_supervisor and without a project-set
  TIMEOUT. A caller may supply a CTest timeout, but that still is not the project's
  verified Job Object process-tree contract.
- **Consequence:** this secondary entry point does not guarantee a finite total
  runtime or verified cleanup of descendants. The canonical test.cmd path does
  use the supervisor and is not being declared regressed.
- **Coverage:** R1's canonical exit/cancellation/descendant fixtures do not cover
  invoking these generated CTest entries directly.
- **Minimal fix:** route CTest entries through the same bounded supervisor and
  preserve arguments/OFF/ON isolation, or explicitly disable the unsupported
  direct execution entry point. A TIMEOUT property alone does not prove cleanup.
- **Focused validation:** use the existing marked child/grandchild failure probe
  through a CTest entry; require nonzero propagation and confirmed process exit.
- **Disposition:** address before recommending CTest as a safe validation route;
  canonical supervised focused tests remain available for remediation.

### Inherited risks and uncertainty kept separate

| Item | Source/evidence boundary | Current disposition |
| --- | --- | --- |
| Full startup/failure cleanup | InitializeThread, guarded reset, real game installers/workers | N03 remains **Partially mitigated**; this report does not close it |
| Live object lifetime/concurrent native consumers | Checked pointer access and local continuation fixtures do not pin game objects or execute every engine consumer | N04–N06 remain **Open** under their existing definitions |
| Native callback/reset traffic | NativeCallbackAdmission, stopping rechecks, N01 resident executable bundles | Preserve R3's focused result and N05 Open; no dynamic unload claim |
| Owner/driver liveness | composition_runtime.cpp:179–199,1426–1431; DComp/D3D calls | Owner waits/driver completion remain unverified; do not add unsafe forced graphics teardown |
| Earlier post-toast freeze | Existing ETW/PresentMon and user observation | Mechanism remains unproven; PF01–PF09 are not asserted to explain that pause |
| Current artifact compatibility | G5 accepted an earlier exact ASI | New source needs its own bounded F4 qualification after repair |

N01/N02 remain Fixed within their recorded scope. SafetyHook's retained stub,
trampoline and allocator storage is intentional protection for possibly executing
native continuations; it is not relabeled a leak or successful teardown here.
The R9 fixture failure remains resolved as an admission precondition defect.
The absence of FlushInstructionCache is not asserted causal or a newly approved
vendor fix. Historical artifact mismatches remain recorded; this audit does not
require rebuilding an obsolete historical ASI to qualify the next current one.

## 6. Cross-system transition/invariant matrix

Legend: **Source-supported** = traced in current code within stated assumptions;
**Historical test** = existing tests/reports only, not rerun here;
**Violated** = a concrete counterexample above; **Unverified** = evidence limit.

| Transition | Required invariant | Current source/test evidence | Assessment |
| --- | --- | --- | --- |
| DllMain → initializer | Work is delegated; loader-lock detach avoids joins/reset | dll_entry.cpp; runtime.cpp:6806–6879 | Source-supported; dynamic unload unsupported |
| INI synchronization → load | Compatible effective settings survive canonicalization | Checked reads/atomic replacement; PF05/PF06 | **Violated** for split sections/legacy Gameplay false |
| Missing/malformed INI → recovery | Complete staging; backup precedes replacement; failed backup/write preserves original | config_template.cpp:696–711,1172–1219; F3 focused evidence | Source-supported; no fresh I/O execution |
| Valid INI → repeat startup | No textual change means no replacement | output/original comparison; F3 identity assertions | Supported for tested shapes; PF05 shows idempotence alone is insufficient |
| INI → diagnostics → graphics/core readiness | Startup-only configuration settles before camera-core-ready presentation | runtime.cpp:6474–6513,6806–6838; owner gates RefreshPresenter | Source-supported; Overlay worker/HWND may already exist before presentation readiness |
| Diagnostics OFF/ON → camera output | Instrumentation observes, never selects correction policy | Gates and shared transactions; historical actual adapters | Source-supported on reviewed paths; current full neutrality not rerun |
| Initial activity → subclass installation → menu request | Missed events cannot leave a permanently stale display gate | PF02; semantic focus test covers only mapping | **Violated** recovery guarantee |
| Auto locale → startup toast | A posted request reaches an installed current binding; accepted result precedes hint | PF03; existing language policy/publication tests | **Violated** startup handshake |
| Toast-visible → menu requested → input | Input starts after a committed frame for this interactive request | PF04; actual owner command/readiness path | **Violated** menu-frame admission |
| Game deactivation/minimize → input revocation | Callback fails open immediately; owner hides display | composition_runtime.cpp:911–918,988–997; owner activation/geometry | Source-supported for delivered handled events; PF02 is separate |
| Geometry/DPI/rebind → surface | Owner placement; new surface drawn/committed before publication | renderer generation transaction; G2/G3 | Source-supported; physical multi-monitor behavior not newly tested |
| Game Present/ResizeBuffers → Overlay | No native game swapchain/device/backbuffer ownership | Production registry and private DComp surface path | No interception in this architecture; driver composition interaction unverified |
| Cinematic ENTER/EXIT → Gameplay recovery | Selection generation and accepted source govern handoff | lifecycle/writer transactions; N04–N06 limits | Source-supported at adapter boundary; engine consumer chain unverified |
| Dialogue/ADS → Gameplay baseline | Active lifecycle, source and freshness prevent numeric-only ownership | Dialogue candidate/rearm and camera publication paths | No new contradiction found; native object/thread assumptions remain |
| Runtime mutation → disk → UI feedback | Ordered ApplyAndPersist; failed save does not falsely claim persistence | Settings API serialization, checked persistence, error notification | Source-supported; placement uses the same file serialization |
| Callback reset → hooks/state | Close admission; drain; stop workers; gate reset; preserve executable lifetime | runtime.cpp:753–805,1440–1454; N01 retention | Supported reset contract; N05 remains Open |
| Partial graphics creation → finalizer | All acquired references released independently of success flag | PF07 | **Violated** partial-backend ownership |
| SHA formatting failure → native cleanup | Hash dies before borrowed backing buffer | PF01 | **Violated** native lifetime |
| Tests → process exit | Root/descendants bounded and verified | Canonical supervisor; PF09 secondary CTest bypass | Canonical source-supported; secondary gap |
| Build → published ASI → ZIP | Qualified binary identity and exact inventory retained | Isolated graph, atomic publish, package input/entry hash | Packaging equality source-supported; qualification identity still pending |

## 7. Test coverage and qualification gaps

The present tests provide useful but bounded evidence:

- Actual camera adapters exercise context mutations, owner identity,
  source/generation/refusal, correction-gate serialization, reset admission,
  settings and OFF/ON behavior. They do not collectively execute the game's
  original displaced instructions and subsequent engine consumers.
- N06-M01 preserves separate game-derived Zoom IN/OUT fixtures and both branch
  outcomes. Its accepted offline scope is retained.
- The focused focus test in composition_presenter_state_harness asserts
  KeyboardFocusLost/Deactivated/Gained/Activated mapping. It does not instantiate
  current Renderer + real subclass install order. It cannot establish PF02–PF04.
- Config persistence covers read/write failures, duplicates in setters,
  Cinematic/Dialogue/hotkey migrations, malformed backup, BOM, compact markers,
  Boolean descriptions and no-op bytes/identity/time. PF05 needs synchronization
  of split sections; PF06 needs the supported legacy Gameplay key.
- SHA allocation checkpoints currently test return/output/file ownership, not
  the borrowed-buffer lifetime in PF01.
- Generic finalizer/graphics fixtures establish selected cleanup steps and
  truthful retained outcomes. They do not cover PF07's actual partial backend.
- Localization has explicit 18-catalog/resource/font registration and validation;
  no fresh catalog audit, visual language check or glyph rendering was performed.

No harness result is promoted to a current production-artifact PASS. Tests
recommended in this report are future remediation acceptance criteria, not
executions performed during this audit.

## 8. Build/release review

The current source registry remains explicit; runtime.cpp is included once by
actual-adapter harnesses, with the other 59 production inputs supplied separately.
Generated INI headers, resource output and harness executables are binary-tree
local. Production C++ uses `/std:c++latest`, `/EHsc`, `/W4`, `/utf-8`, static CRT
and the reviewed Release optimization. The native adapter's explicit NDEBUG is
separate from ordinary assert-based harnesses, whose registry audit rejects a
global NDEBUG injection. No effective compiler invocation was re-measured here.

The normal user artifact is **one STALKER2CameraTweaks.asi**, with passive
diagnostics selected by the startup INI switch. The optional diagnostic-profile
wrapper is not a requirement for enabling normal diagnostics.

The publisher uses an isolated built input and atomic destination replacement.
The package builder accepts exactly ASI, README.md, LICENSE.md and
THIRD_PARTY_NOTICES.md, checks exact ZIP names and hashes the packaged ASI against
its input. Current installation text treats INI as runtime-owned user data.

Two qualification limits remain:

1. The packager proves equality with its supplied ASI, not that the supplied ASI
   came from the source/test subject approved for F4. F4 must record that identity
   explicitly before packaging and verify the final ZIP against it.
2. The existing historical v3.1.0 ZIP still contains README.txt and an INI, with
   a 3,555,840-byte ASI. It predates the new allowlist and is not a package of the
   current tree. It was only inspected, not rewritten. Future qualification must
   create a new approved package rather than mistake this historical file for it.

The historical package is not counted as a new production-code defect; the F3
follow-up explicitly left historical archives intact.

## 9. Reviewed areas with no additional finding

Within the bounded source review, no additional concrete contradiction was found
in these mechanisms beyond their separately recorded evidence limits:

- Canonical HorPlus/projection formulas and finite/range guards; no basis to
  change camera mathematics or the shared semantic FOV tolerance.
- RuntimeCombine aspect-only write/readback confirmation before deriving its
  Gameplay target, and confirmed Cinematic endpoint retention during recovery.
- Cinematic selection/generation association, source-based recovery authority,
  indexed/rejected EXIT routing and continuation policies.
- Dialogue candidate promotion, active-policy retention, correction disable
  epoch and post-Cinematic exclusion/rearm.
- Native signature cardinality, production instruction/register contracts,
  refused resolution and staged Cinematic component commit.
- Resident RuntimeState ownership, optional Overlay/core separation, native
  callback admission and post-lock stopping checks in correction adapters.
- N01 executable/stub/allocator retention and independent protection-restore
  attempts; no new instruction-relocation defect established.
- Event-time rebind modifiers, bounded queues/terminal failure on command loss,
  immutable language function publication and bounded retained binding records.
- Private D3D11/DComp ownership, permanent target on Overlay HWND, attachment
  without target churn, click-through display, surface generations and DPI cache.
- Atomic INI staging/replace, checked incomplete reads, malformed backup failure
  preservation, single-value persistence and accepted-but-unsaved feedback.
- Catalog key/placeholder validation design, locale IDs and separation of
  effective runtime values from display strings; visual translation/RTL quality
  remains outside source-only certification.
- Runtime diagnostic gates, trace bounds, optional exception-contained logging
  and startup-only Diagnostics configuration. No measured new performance
  regression is claimed from static code alone.
- CMake source/resource dependency graph, tree-local outputs and canonical
  runner's nonzero/crash/timeout propagation. PF09 is the separate CTest surface.

This list means no additional finding was established in these reviewed paths;
it is not a guarantee against untested game/driver behavior.

## 10. Dependency-aware remediation proposal

Implementation is **not started**. Proposed independently reviewable batches:

| Order | Batch | Findings | Acceptance and preserved guarantees |
| ---: | --- | --- | --- |
| 1 | Correct native backing-storage lifetime | PF01 | Destructor-order assertion + existing OFF/ON ownership case; preserve digest failure and file/crypto ownership |
| 2 | Canonical rebuild preserves logical values | PF05, PF06 | Full synchronize/load equivalence, duplicate/legacy precedence, malformed backup and second-run identity; preserve minimal persistence and compact docs |
| 3 | Commit binding before startup requests | PF03; assess PF08 in the same boundary | Deterministic pre-install message consumption and foreign-subclass interleaving; preserve native-reader affinity and nonblocking callbacks |
| 4 | Reconcile actual host activation | PF02 | Startup activation gap + open + Alt+Tab/minimize recovery; preserve owner-only display work and immediate input revocation |
| 5 | Admit input after the requested menu frame | PF04 | Toast-to-menu delayed/failed Commit and close/reopen; preserve event-driven rendering and heartbeat fail-open |
| 6 | Release partially initialized graphics backend | PF07 | Actual pipeline/font fault paths through owner finalizer; preserve DComp uncertainty retention and one-shot truthful outcome |
| Separate infrastructure step | Contain secondary CTest invocations | PF09 | Existing process-tree/crash probe through the CTest route; preserve scenario arguments and serial execution |

Use the existing canonical Job Object supervisor for authorized focused tests.
Fixing PF09 is required before executing the direct CTest route, not a reason to
withhold unrelated canonical contained checks. No repeated full suite per batch
is proposed. Once repairs and focused acceptance are complete, one approved
canonical regression and subsequent intended ASI build can qualify the aggregate.

Changes must retain the current single RuntimeState and Overlay owner, native
ABI/store contracts, camera mathematics, failure-safe pass-through, resource
retention when native reachability is uncertain, and the F3 settings-preservation
contract. None of these findings justifies reversing G1–G5 or B1–B7 decisions.

## 11. Remaining live-only requirements and gate decision

After source repairs and the separately authorized offline/build gate, the next
live qualification should use one identified current ASI and the already agreed
bounded scenarios: startup toast/menu visibility and input, activation/restore,
camera lifecycle/recovery and the user's graphics-injector/HUD/FG combination.
Diagnostics OFF/ON use the same ASI. Historical G5 observations remain valid for
their recorded artifact and do not automatically transfer.

Actual presentation mode, VRR/FG behavior, driver completion, game-object lifetime
and engine-consumer behavior cannot be established by this source audit. The
earlier startup pause remains an observation with an unproved mechanism. No new
ETW, PresentMon or repeated graphics experiment is requested by this report.

**F4 preflight: BLOCKED.** The audit itself is complete at the stated source and
integration depth. PF01–PF07 contradict current lifetime, visibility/admission or
settings-preservation contracts and require repair before recommending the next
qualification build/live test. PF08 needs a focused disposition; PF09 must be
resolved or its execution surface excluded from the qualification procedure.
N03 remains Partially mitigated; N04–N06 remain Open. No remediation is authorized
or implied by publication of this report.

### Audit preservation check

After report creation, all 394 snapshotted existing input files had identical
SHA-256 values, and the Git index still had the exact SHA-256 recorded above.
No analysis helper, compiler, test child or capture process was launched; the
bounded repository-read commands completed. Consequently there is no owned
graphics fixture or recording session to finalize for this audit. This does not
claim anything about unrelated processes or prior driver/system state.
