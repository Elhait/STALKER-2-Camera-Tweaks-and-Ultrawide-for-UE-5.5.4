# V4-01 — Unified Dialogue Policy Selection

Status: implemented and validated; complete `test.cmd` suite passed.

## Objective

Keep one effective Dialogue policy for a validated post-EXIT provider handoff,
its writer mapping, and the Dialogue lifecycle admitted from that handoff.
Preserve the existing policy formulas, Gameplay A/B behavior, Cinematic
mapping, and recovery authority/order.

## Confirmed current-source mismatch

- `ProcessDialogueBoundary` captures an `activePolicy` for an accepted Dialogue
  lifecycle and uses that snapshot for subsequent setter samples.
- `PostExitProviderActivation` currently captures generation, Dialogue
  settings epoch, component/receiver/thread identity, and native start/target,
  but no Dialogue policy.
- `TryMapPostExitProviderDialogueSample` resolves its Dialogue target from the
  writer's current `input.dialoguePolicy` on every sample.
- `RuntimeSettingKind::DialogueZoomPolicy` updates that selected policy
  without changing the Dialogue epoch. Thus a policy selection change between
  provider activation and its writer samples can change the mapping for the
  same activation, while identity/epoch checks still accept it.
- At provider Dialogue admission, `ProcessDialogueBoundary` currently captures
  the selected policy again. It can therefore start the admitted lifecycle
  under a different policy from the one used during its provider handoff.

This conflicts with the existing lifecycle-scoped `activePolicy` behavior.

## Minimal change

Extend the existing provider activation record with its selected Dialogue
policy. Capture it at the validated provider armer, pass it through the
existing writer evidence and provider Dialogue admission, and use it for all
mapping decisions belonging to that activation. A later UI policy selection
applies to the next accepted Dialogue lifecycle. Disabling Dialogue correction
continues to fail closed immediately through the existing enable gate.

Do not add a coordinator class, policy-selection state machine, native hook,
or independent snapshot owner. Keep `RuntimeState` as lifecycle owner and
`ProcessCameraWriter` as the output decision arbiter. Preserve the distinct
native setter and view-output stages.

## Expected files

- `src/dialogue/post_exit_provider_handoff.hpp`
- `src/plugin/dialogue_boundary_transaction.hpp`
- `src/plugin/runtime.cpp`
- Focused existing Dialogue/provider and camera-writer harnesses.

## Acceptance criteria

1. Changing the selected Dialogue policy after provider activation does not
   change that activation's writer mapping or the Dialogue policy captured at
   its provider admission.
2. A new provider activation captures the newly selected policy.
3. A Dialogue correction disable, source/receiver/thread/generation mismatch,
   settings-epoch mismatch, EXIT, or recovery cancellation retains the current
   fail-closed/reset behavior.
4. Gameplay `RuntimeCombine`, Cinematic mapping, and recovery calculations
   remain behaviorally unchanged.
5. Focused lifecycle/writer harnesses and the complete `test.cmd` suite pass
   on the modified working tree.

## Validation and non-goals

Run the focused registered harnesses and the complete `test.cmd` registry.
Review the changes against the pre-existing staged user work. No game launch,
runtime capture, ASI packaging, INI change, release metadata change, visual
claim, or U2 re-audit is part of this task.

## Implementation and validation record

- Captured Dialogue policy in the existing post-EXIT provider activation and
  used that same value for provider writer mapping and resulting Dialogue
  lifecycle admission. The existing correction-enabled gate still fails
  closed immediately if Dialogue correction is disabled.
- Added regressions where the selected UI policy differs from the activation
  policy at both writer mapping and Dialogue admission; both consume the
  activation snapshot.
- `camera_writer_transaction_harness.exe`: PASS.
- `dialogue_boundary_transaction_harness.exe`: PASS.
- Initial sandboxed `test.cmd` run could not create the hardlink fixture
  (`config_persistence_harness`, Windows error 5). The same operation passed
  outside that filesystem restriction; the NTFS volume and directory ACL
  allow hardlinks.
- Re-run of the complete `test.cmd` outside that restriction: PASS, all 62
  registered harness invocations, including `config_persistence_harness` and
  both modified harnesses. Process-tree cleanup was verified by the runner.
