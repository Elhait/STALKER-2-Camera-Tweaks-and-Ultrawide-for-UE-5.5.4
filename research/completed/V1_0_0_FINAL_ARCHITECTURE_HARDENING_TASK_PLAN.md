# v1.0.0 Final Architecture Hardening — Task Plan

## Objective

Close the five confirmed v1.0 architecture/release blockers and the three
approved presentation-quality improvements from the independent-review triage,
without changing validated gameplay, cinematic, dialogue or resolver behavior.
After this batch, freeze architecture and proceed only to final regression and
release documentation.

## Established evidence and current state

- The production feature behavior is validated on the supported Steam 2.0.5
  executable and must remain unchanged.
- `runtime.cpp` still owns substantial mutable state in an anonymous namespace;
  ownership boundaries are not sufficiently visible from the production type
  structure.
- The ASI lifetime model is implemented as process-lifetime usage with normal
  dynamic unload not claimed, but this contract is not prominent enough in
  production architecture documentation.
- `build.cmd` uses an author-specific Visual Studio path, `/std:c++latest`,
  manual source enumeration and test-named production defines.
- Generated objects and test/tool outputs are tracked or insufficiently
  ignored for a release-ready clone.
- Resolver definitions and validation exist, but the update/provenance workflow
  is scattered across source comments and research reports.
- `feature_status_harness` validates status semantics only; lifecycle and
  persistence harnesses cover their narrower observable contracts. Claims must
  match that evidence.
- `helper.hpp` remains in the production dependency graph for pattern scanning;
  its unsafe write helpers are not reachable from the stable runtime path.

## Approved scope

1. Make runtime ownership and lifetime explicit without requiring a concrete
   `plugin::Runtime` class solely for naming.
2. Formalize and document the supported ASI lifetime/loader contract.
3. Make the production build reproducible with a documented MSVC 17.14+
   toolchain and C++23-era language support through `/std:c++latest`, plus
   explicit production/test separation; CMake is optional.
4. Remove production dependence on test-named compile-time switches, promoting
   validated behavior to production configuration or isolating diagnostics.
5. Establish release-ready repository hygiene and generated-output policy.
6. Add a concise resolver/game-update workflow.
7. Correct harness and evidence claims to match actual test coverage.
8. Remove or isolate the `helper.hpp` production dependency if the extraction
   is bounded and behavior-preserving.

## Explicit non-goals

- No new user-facing features or behavior changes.
- No transactional `HookSet` rewrite without new contradictory evidence.
- No DI framework or fakes everywhere.
- No CMake migration for its own sake.
- No full `WorkerLifecycle` rewrite without a newly confirmed defect.
- No artificial coordinator/class hierarchy or LOC-driven extraction.
- No new PE/memory subsystem without a concrete confirmed defect.
- No reopening deferred Weapon Viewmodel FOV or other research branches.
- No full Batch 4 regression until this batch reaches architecture freeze.

## Expected files or areas

- `src/plugin/runtime.cpp`, `src/plugin/runtime.hpp`, and related ownership
  headers.
- `src/plugin/dll_entry.cpp` and lifecycle documentation.
- `build.cmd`, test build commands and build documentation.
- Production/test preprocessor boundaries in `src/`.
- `.gitignore` and explicitly approved generated-artifact paths.
- Resolver/signature documentation under `docs/` or `research/reports/`.
- Harness documentation/claims and relevant report wording.
- `src/helper.hpp` and `src/hooks/signature_scanner.*` only if bounded
  isolation is demonstrated.

## Implementation batches and validation

### A1 — Runtime ownership and lifetime contract

Inventory and extract only the state/control boundaries needed to make runtime
ownership explicit. Preserve callback behavior, hook order, worker semantics
and feature policies. Add the supported ASI lifetime contract to production
architecture documentation.

Validation: source/static ownership review, existing lifecycle and feature
harnesses, build and diff review. Normal dynamic unload remains unclaimed.

### A2 — Reproducible production build and test separation

Replace the author-specific toolchain assumption with a documented portable
toolchain discovery/configuration path, fix the language standard to C++23 and
separate production behavior from test/research switches. Keep the current
validated production behavior as the default.

Validation: documented clean-checkout build procedure, production build,
harness builds, compiler diagnostics and `git diff --check`.

### A3 — Repository hygiene

Define ignore rules and release/build-output boundaries. Remove generated
artifacts only within an explicitly reviewed cleanup scope; preserve source,
research evidence, Ghidra data and intentional release assets.

Validation: tracked-path review, clean checkout inventory and release-file
boundary review. No broad deletion or Git state-changing operation.

### A4 — Resolver workflow, evidence claims and helper isolation

Add concise update/provenance documentation, correct harness claims, and
attempt only a bounded removal/isolation of the legacy scanner dependency if it
does not alter scan semantics. Do not rewrite unsafe dead helpers unless the
dependency extraction requires it.

Validation: static dependency review, relevant harnesses, production build and
documentation consistency review.

### A5 — Architecture freeze review

Reconcile all five blockers and three approved presentation improvements
against the actual diff. Confirm no new feature, resolver semantics or runtime
behavior was introduced. Archive this plan only after all approved work is
validated.

## Risks and rollback / safe-failure behavior

- Runtime extraction can accidentally change callback state ownership or
  teardown order; stop at the first behavior/control-flow mismatch.
- Build changes can silently remove validated production paths; compare the
  preprocessor-expanded intent and run the existing harnesses.
- Repository cleanup can delete user research or release evidence; resolve and
  review exact paths before any removal, and do not use broad wildcards.
- Helper isolation must preserve scanner cardinality and signature semantics;
  if it requires a scanner rewrite, defer it.
- If any blocker requires reopening validated gameplay/cinematic behavior or a
  deferred research branch, stop and report rather than expanding scope.

## Stop conditions and phase gates

- Stop before implementation if ownership extraction requires a new runtime
  abstraction with no clear independent contract.
- Stop if the production build cannot be made portable without changing
  runtime behavior or relying on guessed toolchain paths.
- Stop repository cleanup when an exact target is uncertain; request a separate
  cleanup scope rather than deleting it.
- Stop A4 helper work if the bounded dependency extraction is not mechanical.
- After A5, declare `ARCHITECTURE FREEZE`; no third architecture review or
  cleanup batch is authorized before final Batch 4.

## Expected final Git review

Inspect status, relevant diffs, changed paths and recent history. Compare every
change with this plan and report completed, remaining, deferred, blocked and
not-runtime-validated items. Do not stage, commit, publish or upload without
explicit instruction.
