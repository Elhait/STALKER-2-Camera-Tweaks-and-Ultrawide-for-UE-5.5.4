# Camera State: authoritative FOV references and observation stages

Status: IMPLEMENTED — OFFLINE VALIDATED. Production publication, exact writer
correlation, UI and all 18 locale catalogs are complete. Full test.cmd 62/62 PASS
(1 explicit hardlink subcase SKIP, error=5), including graphics cases. Standard
Release ASI built and provided from the project only. Observed in-game display confirmation is now recorded for PID 26108; no game files were replaced. Evidence and artifact identity:
`research/evidence/camera-state-fov-semantics-2026-11-16/production-validation.md`.

## Scope

Camera State projects existing production decisions and captured observations.
It does not select policies, reconstruct unseen original FOV or become a second
baseline authority. Preserve GameplayHorPlus, Reduced, cinematic mapping,
recovery, admission and all existing correction formulas.

## User-visible contract

| Fact | Meaning and authoritative source |
| --- | --- |
| Gameplay FOV | The admitted native numeric Gameplay reference in GameplayBaselineStore, with its source, observation sequence, valid/current/usable state and reference freshness. Not a claimed exact copy of the game settings slider. |
| Neutral Gameplay FOV | Existing stable sample qualified by zoom evidence, kept separately in diagnostics. Its unavailability does not hide the admitted Gameplay reference. |
| Native FOV | Directly captured original/provider value BEFORE the particular mod intervention being described. Must state its boundary and owner. It is not automatically original for every earlier/later camera operation. |
| Writer Input FOV | Actual input to this writer call after game preparation and any upstream mod correction. Never infer pristine provenance merely from NativeRegisterInput. |
| Planned Output FOV | Output selected by the same production writer transaction; remains a plan until independently read back. |
| Effective FOV | Output read back at a named correlated post-store/post-helper boundary, with its verification status. This is not proof of final rendered projection. |

Each fact retains validity, source/receiver identity, observation/transaction
sequence, lifecycle ownership, provenance and freshness appropriate to its
source. Do not present independently sampled fields as one atomic transaction.

### Gameplay reference trust

Publish the existing baseline independently of zoom-neutral qualification. An
active admitted reference can be retained across confirmed Cinematic/Dialogue
ownership; explicitly distinguish Retained from a live current sample.
Receiver replacement or an existing baseline-trust invalidation makes the old
reference unusable even when its numerical value is unchanged. Never revive it
from UI history. Do not mechanically invalidate for every settings epoch:
Dialogue policy epoch alone does not retire Gameplay currency, as established
by the validated baseline-trust contract. A relevant epoch can invalidate only
according to its production owner and current admission rules. The baseline
currently owns no native receiver-lifetime epoch; do not invent one in the UI.

### Original/provider evidence and freshness

Use actual captures at each boundary: authored Cinematic ENTER input, raw
Dialogue callback/provider input before changing the setter argument, and
qualified Gameplay observation where its origin is established. A captured
ENTER target (e.g. authored 90) is a boundary reference, not a live per-frame
native blend sample. Keep it Retained/Historical as appropriate. A provider
target 70 and callback sample along the transition are different facts.

For Cinematic 2, neither writer input 153.931 nor Gameplay baseline 130.311
establishes an original per-frame value before all mod intervention. If no
direct appropriate original capture is available, Native FOV is Unavailable
for that meaning. Do not invert formulas, reuse +0x234, interpolate a fictional
original trajectory, or relabel the corrected native-camera field.

### Effective output and Diagnostics OFF

The current snapshot often publishes planned output while real post-store
readback lives in the diagnostic observer path. Do not silently promote the
former to verified Effective FOV. Reuse matched existing readback when available;
otherwise show Unavailable for Effective and preserve Planned Output. Do not
add hooks, always-on reads, alter policy or make correction depend on telemetry
to force this field to appear with Diagnostics OFF. Missing, conflicting,
out-of-order or mismatched readback never confirms the planned output. Even a
matched readback establishes only that observation boundary and ordering scope.

## Implementation batches

1. Inventory direct publishers/partial updates and UI consumers for existing
   gameplayFov/nativeFov/currentOutputFov. Preserve the neutral observation as
   its own fact while connecting Gameplay FOV to existing baseline readback.
2. Add red regressions at current publication/overlay entry points: no zoom
   callbacks with a usable admitted baseline; retained reference across modes;
   receiver replacement/loss of trust; irrelevant Dialogue policy epoch versus
   relevant production invalidation. Assert values and provenance/freshness.
3. Extend Camera State facts and publisher inputs narrowly. Route existing
   original captures and writer input to distinct fields. Partial Dialogue/
   Cinematic publications must not overwrite unrelated original evidence with
   corrected input or refresh stale references. No new state owner.
4. Connect correlated existing readback to verified Effective where available;
   test planned-versus-actual mismatch, absent readback, out-of-order events,
   Diagnostics OFF/ON, and receiver/lifecycle changes. Correction outcomes must
   remain identical with/without diagnostics.
5. Update Current State, camera details and localized labels/tooltips together.
   Explain the field's boundary/reference status; avoid an unqualified live
   Native label for retained authored targets. Preserve original/localized key
   compatibility where needed and expose Unavailable honestly.
6. Run focused publication/integration/localization tests, then the applicable
   canonical suite. Build standard Release only at the release stage; provide
   link and never replace files in the game directory. Runtime confirmation
   targets display semantics; do not repeat settled Gameplay A/B or FOV math.

Expected files: camera_state_snapshot/publication, existing runtime publication
adapters and readback observers, typed overlay snapshot/integration/detail
views, locale catalogs, and their registered harnesses. Inspect actual call
sites before editing; the list is a bounded surface, not permission to rewrite
all those modules.

## Acceptance

- Admitted Gameplay reference appears without requiring ADS/zoom activity.
- UI never marks stale/foreign baseline as current/usable; no new baseline
  trust algorithm or settings-epoch namespace is introduced.
- Original boundary captures, prepared writer input and verified output are
  distinguishable in Gameplay, Dialogue and both Cinematic transitions.
- Unknown original values and absent readback remain Unavailable; captured
  authored references are not fabricated live samples.
- Planned output remains identifiable independently of actual output.
- Existing camera behavior and diagnostics neutrality remain unchanged in
  production transaction regressions. Screenshots alone are not proof of
  correction behavior, and offline checks do not establish rendered FOV.

Risks: confusing retained reference with live input, mixing partial-publication
owners, accepting stale readback, weakening zoom evidence or imposing arbitrary
epoch invalidation. Keep explicit negative tests for these boundaries.

## Accepted refinements and actual publication inventory

Planned and Effective must share writer transaction identity, output/receiver
identity and named observation boundary; snapshot event sequence cannot serve
as a substitute. A stale readback N-1 cannot verify planned N. Consumer-specific
baseline usability must be sourced from production; UI describes a retained
admitted reference rather than inventing a generic Usable flag or a live sample.

| Path | Current source/destination | Missing link |
| --- | --- | --- |
| Full writer publication | runtime PublishCameraStateSnapshot -> PublishCameraStateSnapshotLocked -> evidence.gameplayFov/nativeFov/currentOutputFov | No production baseline input; gameplayFov is currently the zoom-neutral fact |
| Partial writer publication | ProcessCameraWriter result -> PublishCameraFovTransaction -> PublishCameraFovTransactionLocked | Input tagged native without upstream distinction; output is planned, not readback |
| Partial Dialogue publication | callback incoming XMM6/output XMM1 -> same partial FOV publisher | Setter boundary shares native/output fields with writer boundary; source is callback, not necessarily receiver |
| Primary post-store | ObserveCameraWriterPostStore -> CameraStateWriterEvent queue | Actual readback and transaction identity stay in diagnostics; no Camera State publication |
| Post-helper | ObserveCameraWriterPostProcess -> same event queue | Same gap; verification boundary must remain explicit |
| Overlay | semantic snapshot -> camera integration / current-state and detail views | Reads merged fields; no distinct writer-input/planned/effective roles |

`CameraFovTransactionPublication.sequence` is assigned from snapshotEventSequence.
It has no writerTransactionId, output-object identity or readback-boundary input.
The existing CameraFovFact source/sequence alone cannot pair planned/actual.
This is an interface gap, not a passing correlation test.

### Red-regression batch (no production changes)

Extend the existing registered camera_state_snapshot_harness through current
production baseline projection and publication functions. Independently assert:
admitted reference visible without zoom; confirmed Dialogue retains that same
reference as Retained; known corrected writer input cannot create original FOV;
later corrected Cinematic input cannot replace a captured original as Current.
Do not insert a test-only publisher or add production fields merely to compile
future-state tests. Existing neutral-reference checks still characterize the
old semantic role; migrate them to the distinct neutral field during repair,
not by relaxing their zoom requirements.

Planned/Effective N versus N-1, absent/mismatched readback and boundary matching
need the subsequent publication interface extension before executable coverage
is possible at that entry point. Keep these criteria open; do not simulate a
test-only transaction model and call the production path covered. Receiver/
epoch controls likewise need baseline evidence connected to publisher inputs;
existing baseline/store negatives remain useful but do not establish UI trust.

### Verified red-stage result

Built and ran camera_state_snapshot_harness under the bounded process-tree
supervisor. Exit=1, cleanup verified. Exactly four new expectations fail:

- admitted_gameplay_reference_must_be_visible_without_any_zoom_event
- confirmed_dialogue_keeps_admitted_reference_visible_as_retained_not_live
- upstream_corrected_writer_input_must_not_create_original_native_fov
- prepared_cinematic_input_cannot_replace_original_capture_as_current_native

Production baseline admission, same-source confirmed-Dialogue baseline
retention, and direct original capture controls pass. The previously existing
snapshot checks still pass. The canonical harness remains intentionally red;
no skip/xfail or unregistered alternate path hides the new failures.

Only the registered test and this plan were edited in this stage. No production
fix, ASI build, installation or full-suite run was performed. Actual
Planned/Effective correlation coverage remains pending the real publication
interface described above; it has not been claimed as tested.

## Production batch disposition

Batches 1–5 complete: real publishers use the admitted baseline, separate neutral
observation and original captures from prepared input, and correlate named readbacks
using the existing writer identity. UI/locales and glyph subsets are updated.
Batch 6 offline portion complete: focused regressions and full canonical suite PASS;
ordinary Release build PASS; process cleanup verified. Targeted in-game display
confirmation remains follow-up, not an offline PASS. No correction-policy deviation.
Font subset regeneration was required by new localized text and is covered by the
existing font/catalog harness. Legacy neutral tests keep their original zoom guards.
No task-log closure entry or automatic game installation was performed.

## Observed runtime confirmation — PID 26108

User completed Cinematic 1 -> Dialogue -> Cinematic 2 -> Gameplay -> standalone
Dialogue and reports ideal visuals and overlay. Current artifact hash matches the
production build. Both ENTERs select the same usable Gameplay baseline / 153.931
output; confirmed Dialogue reaches 149.071; standalone Dialogue reaches 148.946.
All 9520 observed transactions have matched store/helper evidence, zero recorded
losses or mismatches. Native captures remain distinct from prepared input.
Historical on post-cinematic Gameplay / ordinary Dialogue reflects the existing
production trust contexts, not missing publication. Do not promote it to Current
or change admission to satisfy a display expectation.

Runtime display follow-up: complete for this observed run; Diagnostics OFF remains
offline-covered. Evidence: research/evidence/camera-state-fov-runtime-2026-11-16-213855/assessment.md.
No production changes, new build, installation or repeated test run in this stage.
