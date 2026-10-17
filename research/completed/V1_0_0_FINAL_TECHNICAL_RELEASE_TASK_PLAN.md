# v1.0.0 Final Technical Release — Task Plan

## Objective

Restore the intended production architecture that should have governed the
project from the beginning, rather than applying a cosmetic cleanup to the
v0.6.0 monolith. Preserve all validated observable behavior, run the final
regression matrix and synchronize release documentation. Mark v1.0.0 as the
end of the active feature-development phase and transition the project to
maintenance.

## Top-level invariant

> **The feature set is frozen. The architecture is not.**

`v1.0.0` may radically change internal structure, ownership, lifecycle and
build organization, but it must preserve the validated `v0.6.0` observable
behavior. A behavior change is a regression signal, not an automatic
improvement.

## Established evidence and current state

- v0.6.0 is the current feature-complete unified production release.
- Gameplay aspect correction, cinematic aspect/FOV correction, dialogue zoom
  policies, dynamic Auto behavior and the post-cinematic handoff are already
  runtime-validated on the supported Steam 2.0.5 executable.
- Resolver behavior is contract-bound to dynamic resolution, structural
  validation and fail-closed refusal.
- Weapon/viewmodel FOV research established the reference mechanism but has no
  validated standalone ASI bridge; it is deferred and must remain separate.
- Subtitle centering and other deferred research branches are not production
  implementation targets for v1.0.0.
- The independent audit identified structural debt and several findings that
  require confirmation against the actual control flow before classification;
  audit candidates are not automatically accepted as defects.

## Approved scope

- Fully split the production monolith into explicit ownership domains:

  ```text
  src/plugin/
  src/config/
  src/hooks/
  src/gameplay/
  src/cinematics/
  src/dialogue/
  src/platform/win32/
  ```

- Replace the historical `experimental_cinematic_21_9_combined_fix_204.cpp`
  production surface with descriptive production file names and modules.
- Move research-only `.cpp` files and compile-time experiments out of the
  production source surface.
- Replace scattered global state with named domain state objects and a
  central `plugin::Runtime` owner for configuration, hooks, threads and
  shutdown.
- Introduce a transactional `HookSet`/runtime-hook ownership model and commit
  hooks only after complete initialization succeeds.
- Separate Win32 memory, module, hashing and keyboard helpers behind checked
  platform implementations with declarations in headers.
- Unify resolver, instruction validation, telemetry, configuration and
  lifecycle patterns while preserving behavior.
- Resolve or explicitly reject audit findings through control-flow evidence:
  hotkey-thread shutdown, initialization rollback, native pass-through on
  failed writes, atomic INI replacement, loader-lock cleanup and helper safety.
- Clean the build into explicit production and research targets. The
  production `build.cmd` must not silently define research `_TEST` macros.
- Use a fixed `/std:c++23`, remove hard-coded toolchain paths where practical,
  and enable appropriate warning/static-analysis gates after the split is
  stable.
- Improve fail-closed behavior, comments, diagnostics and configuration
  handling without changing the supported feature set.
- Build the production ASI and run the approved regression matrix.
- Update version, release notes, README, Nexus/release text and development
  status after validation.

## Explicit non-goals

- No new user-facing features.
- No Weapon Viewmodel FOV implementation or native UE reflection bridge.
- No subtitle-centering implementation.
- No new experimental hooks, guessed RVAs, offsets or undocumented ABI use.
- No reopening of deferred reverse-engineering branches.
- No changes to the Ghidra project, research probes or research-only tools
  unless required to preserve documentation provenance.
- No release upload or Git state-changing operation unless separately requested.

## Expected files or areas

- Production C++ source and headers under the new domain structure beneath
  `src/`.
- Production build scripts and configuration templates only where required.
- Release metadata and documentation: `README.md`, `RELEASE_NOTES.md`,
  `GITHUB_RELEASE_BODY.md`, `NEXUS_DESCRIPTION.md` and related release files.
- Research archive and `TESTING_AND_RESEARCH.md` only for final status
  synchronization.
- No changes to historical binaries, research ASIs or the Ghidra project.

## Batches and validation

### Batch 1 — Behavioral contract and architecture inventory

Freeze the v0.6.0 behavioral contracts in tests/fixtures before moving code:
gameplay, cinematic, dialogue, resolver validation, configuration persistence
and lifecycle transitions. Map current production files, resolver ownership,
lifecycle coordination, configuration and telemetry. Classify each audit
finding as confirmed defect, architectural debt, intentional behavior, false
positive or research-only code.

Validation: read-only source review, current build/config inventory, contract
test inventory and Git status comparison. No behavior changes.

#### Batch 1 result

The inventory confirms the structural premise and does not change the
behavioral contract:

```text
src/*.cpp total                         64
main production TU                      1
main TU size                            2647 lines
flat research/legacy .cpp files         63
headers directly under src              2
production build output                 STALKER2CameraTweaks.asi
```

The current production command still compiles
`experimental_cinematic_21_9_combined_fix_204.cpp` directly and defines
`GAMEPLAY_FIX_ATOMICITY_TEST` and
`POST_CINEMATIC_GAMEPLAY_REPLAY_ATOMIC_EXIT_HANDOFF_TEST`. It also contains a
hard-coded Visual Studio toolchain path. These are confirmed architecture/build
debt, not yet changed behavior or confirmed runtime defects.

The main lifecycle inventory found the current `CreateThread` initialization,
hotkey and monitor threads, global hook/state ownership and cleanup paths in
the monolithic TU. The external audit's P0/P1 findings remain candidates for
control-flow classification in Batch 3; they were not automatically accepted
as defects.

Result: `SUCCESS` for the architecture inventory. Batch 2 is ready but has not
started; no production source or build behavior was changed in Batch 1.

### Batch 2 — Contract-preserving domain split

Perform the full structural rewrite into `plugin`, `config`, `hooks`,
`gameplay`, `cinematics`, `dialogue` and `platform/win32` ownership domains.
Replace the experimental production filename, separate research sources,
remove obsolete/dead paths where the contract proves them unnecessary and
preserve dynamic resolver contracts, two-pass aspect behavior, player FOV
preservation, cinematic policies, dialogue policies and fail-closed
validation.

Validation: production and research targets build separately; compiler
diagnostics, warnings, static checks, contract tests and diff review. Do not
claim runtime success from build success alone.

### Batch 3 — Runtime/lifecycle safety hardening

Validate the new `plugin::Runtime` ownership, transactional hook installation,
thread stop behavior, shutdown ordering, loader-lock-safe cleanup and checked
memory/configuration failure paths. Resolve the audit's P0/P1 candidates only
when the actual control flow confirms them.

Validation: targeted unit/synthetic tests first, then bounded runtime checks;
any unproven safety assumption remains blocked rather than being papered over.

### Batch 4 — Production regression validation

Run the existing production regression matrix on the supported executable:

```text
gameplay aspect correction
cinematic ENTER/EXIT
Auto aspect switching
Native cinematic policy
forced 16:9 / 21:9 / 32:9 policies
dialogue Native / Adaptive / Reduced / Disabled
dynamic resolution/aspect switching
post-cinematic atomic handoff
camera rebuilds including death/load
safe resolver refusal and telemetry
```

Validation: record executable identity, runtime logs and observed behavior.
Compare results against the v0.6.0 contract; any regression stops the batch.

### Batch 5 — Final documentation and release preparation

Update version metadata to v1.0.0, synchronize README/release descriptions,
replace stale weapon/viewmodel wording, add `DEVELOPMENT STATUS` and document
maintenance mode. Verify the release archive contains only intended files.

Validation: documentation consistency review, release archive inspection and
read-only Git review.

## Risks and rollback / safe-failure behavior

- Preserve the current v0.6.0 source/build artifacts before source edits.
- A large structural diff is expected and acceptable; behavioral drift is not.
- Keep refactor changes separable by batch; do not overwrite unrelated user
  work.
- If resolver identity, decode, lifecycle or runtime behavior changes
  unexpectedly, stop and restore the bounded refactor scope rather than
  expanding into research.
- Any ambiguous signature or ABI behavior must fail closed.
- A failed build or regression run blocks v1.0.0 release preparation.

## Stop conditions and phase gates

- Stop before source edits if the inventory reveals a contract contradiction
  requiring architecture redesign.
- Stop Batch 2 if the refactor requires a new feature, fixed address, guessed
  layout or deferred research implementation.
- Stop Batch 3 on any unresolved lifecycle or safety ambiguity.
- Stop Batch 4 on any regression against the v0.6.0 behavior contract.
- Do not update final release/status documentation until regression validation
  passes.
- v1.0.0 is complete only after source/build review, regression evidence,
  documentation sync and final Git review.
- Do not plan a follow-up cleanup release. A version after `v1.0.0` should be
  created only for a real maintenance reason, such as a game-patch
  compatibility update or a separately approved feature.

## Expected final Git review

Inspect `git status`, relevant diffs, changed paths and recent history. Compare
actual changes against this plan, preserve unrelated dirty work and report
completed, remaining, deferred, blocked and not-runtime-validated items. Do
not stage, commit or publish without explicit user instruction.

