# POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE

## Objective

Identify the native instruction/function that physically writes the interpolated
`126.87 -> 90` values into `CameraComponent +0x230` during the final
`CinematicExiting` recovery on the validated Steam 2.0.5 executable.

## Established evidence and current state

- The production cinematic ENTER path transforms authored FOV `90` to about
  `126.87`.
- On the final cinematic EXIT, the native consumer boundary receives target FOV
  `90` at `FUN_14318DCD4`.
- A separate native per-frame stream then updates `CameraComponent +0x230`
  from about `126.87` to `90` over roughly one second.
- The existing writer observation confirms `XMM0 == CameraComponent +0x230`
  throughout that stream, but does not identify the physical write RIP.
- The current 2.0.5 identity gate is the executable SHA-256
  `e7b481a97c02d80581fab0bece940214a88ebe30211088a00129845a039f9293`.

## Approved scope

- Research-only instrumentation build.
- Final `CinematicExiting` sequence only.
- Capture, if safely possible: elapsed time from final EXIT, write RIP,
  written FOV, CameraComponent pointer, immediate return/caller address and
  thread ID.
- Preserve existing production cinematic, gameplay, dialogue and telemetry
  behavior when the compile-time research flag is absent.

## Explicit non-goals

- No production behavior changes.
- No changes to cinematic ENTER override or EXIT recovery logic.
- No gameplay or dialogue changes.
- No timers, clamps, frame counters used for behavior, hard-coded FOV writes,
  virtual-slot calls or broad camera/renderer tracing.
- No guessed pointers, guessed offsets beyond the already evidenced `+0x230`,
  broad page guards or process-wide exception handling.

## Expected files or areas

- `src/experimental_cinematic_21_9_combined_fix_204.cpp`
- One research build script under `build-artifacts/research/`
- This task plan; a durable report only if the pass produces new evidence.

## Batches

### Batch 1 — instrumentation feasibility and implementation

- Inspect the existing writer hook, SafetyHook context and project helpers.
- Select only a bounded mechanism with a documented/safe contract.
- Add compile-time-gated observation code and a 2.0.5 identity/prologue gate.

Result: the 2.0.5 static audit enumerated 37 scalar `+0x230` stores. The
strongest candidate is `FUN_143DB2C6C`, store RVA `0x3DB2CE7`, with exact
bytes `F3 0F 11 80 30 02 00 00`. Its decompilation also writes `+0x254` and
`+0x259` in the same camera object path and obtains the FOV from transition
state `param_1 + 0x2D0`. A single compile-time-gated SafetyHook observation
was added at that exact store; no page guard/VEH mechanism was used.

Validation: identity PASS for 2.0.5; all other Ghidra programs rejected by the
identity gate; Ghidra project locks released after each tracked run; source
review and diff check completed.

### Batch 2 — research build

- Build the research ASI with the established Visual Studio 2022 toolchain.
- Verify output identity and that the normal production path remains unchanged
  when the flag is absent.

Result: build passed using the established Visual Studio 2022 toolchain.
Artifact: `build-artifacts/research/STALKER2CameraTweaks-PostExitCameraFovWriteOwnerTrace205.asi`.
SHA-256: `ADF1E660FAD3A9644F80DFFD5BE7A0A0A13184954F77D26AA331A0EEF757F75C`.

Validation: compiler/linker success and artifact hash. This is not runtime
validation.

### Batch 3 — user-run runtime validation

- Run one clean final cinematic EXIT with only the research build active.
- Correlate any physical-write events with the existing post-EXIT writer stream.

Validation: user-supplied runtime log and exact game identity. No runtime claim
without that evidence.

### Batch 4 — unconditional candidate diagnostic

- Remove the coordinator-state prefilter from the candidate callback.
- Use a separate bounded hit counter from the existing writer telemetry.
- Log every candidate-store entry with armed state, coordinator state, elapsed
  time, RAX/destination, previous value, incoming XMM0 value, return address and
  thread ID.
- Preserve compile-time research gating and all production behavior.

Result: the previous runtime run produced zero candidate callbacks during the
actual recovery stream, so `FUN_143DB2C6C` remains a strong static candidate but
is not runtime-confirmed. The unconditional diagnostic build is ready for one
clean reproduction.

Validation: source diff review, `git diff --check`, successful MSVC build and
artifact SHA-256
`629A8BBE45183B6353268597331DA6701E47954B15477A9131770A327ADE3EA1`.

### Batch 5 — four-candidate static audit

- Audit only the preliminary stores at RVAs `0x190211`, `0x41674B`,
  `0x6C6A75` and `0x4E92273`.
- Record base/source provenance, callers, neighbor fields, arithmetic and
  update-loop evidence.
- Do not build a runtime probe unless at least one candidate exposes a
  credible active per-frame camera/recovery path.

Result: identity `PASS`; all four candidates were classified as initialization,
configuration, structure-preparation or projection/configuration paths without
proven live CameraComponent provenance or interpolation state. No candidate
qualified for a multi-probe. Durable report:
`02-Research/reports/POST_EXIT_CAMERA_FOV_WRITE_OWNER_FOUR_CANDIDATES_205.md`.

Disposition: static four-candidate gate negative; branch deferred pending a new
concrete runtime/static anchor.

## Risks and rollback / safe-failure behavior

- Write-watch instrumentation can destabilize the game if implemented with an
  unsafe page guard or unbounded VEH. Such mechanisms are out of scope.
- The research feature must refuse installation on any identity, protection or
  instruction mismatch and leave the production hooks untouched.
- Rollback is compile-time removal/disablement of the research flag; no stable
  release file is to be overwritten by the research artifact.

## Stop conditions and phase gates

- Stop before source modification if no safe bounded write-owner mechanism is
  available in the current project/toolchain.
- Static/build success alone does not establish the writer owner.
- Pass only if a stable physical write RIP/function is observed during the
  final recovery stream.
- If multiple writers appear, classify their roles without expanding scope;
  otherwise defer the branch rather than broadening instrumentation.

## Expected final Git review

- Inspect status, relevant diff/diff-check and recent history using the
  repository's configured safe-directory handling.
- Compare actual paths with this plan; report completed, remaining, deferred,
  blocked and not-runtime-validated items.
- Do not clean unrelated pre-existing work.
