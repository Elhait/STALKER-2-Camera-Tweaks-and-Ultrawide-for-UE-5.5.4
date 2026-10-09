# Runtime.cpp decomposition — B1 results

Date: 2026-10-08. Scope: plugin-private binding/layout only. B2 and later batches were not started.

## Change

- Added `src/plugin/runtime_internal.hpp` with an opaque `plugin::detail::RuntimeState` declaration and typed `GetRuntimeState()` reference.
- Kept the concrete 159-field `RuntimeState` definition in `src/plugin/runtime.cpp`; moved only its type name into `plugin::detail` so the private binding has a stable type.
- Kept the sole resident allocation in the same translation unit and at the same initialization point: `g_runtime = *new RuntimeState{}`. All existing aliases still reference that allocation.
- Added actual-adapter assertions that the private binding, `CameraCorrectionGate`, and `HookSet` resolve to the existing owner.
- Added a test-only second translation unit that calls the private accessor, proving the link-level binding returns the same owner rather than a per-TU instance.
- Updated the implementation plan to record the user's B1-only approval and preserve V0-A/B and V0-C as gates for dependent batches.

No callback, algorithm, installer, startup/reset function, state field, hook, or build/test registry entry was moved or added. `Initialize`, `ResetAllRuntimeResources`, and lifecycle order remain in `runtime.cpp`.

## Ownership and state comparison

The saved B0 source copy was compared with the B1 source. After normalizing the explicit namespace qualifiers and whitespace, the complete `RuntimeState` block is identical. The original array-size expression was replaced with an equivalent `sizeof(std::uint8_t) * 4`; the state layout remains four bytes. Source checks confirm one `RuntimeState` allocation, one `CameraCorrectionGate` field, and one accessor definition. The actual adapter checks passed.

## Validation

| Check | Result | Evidence |
|---|---|---|
| Full `test.cmd` | PASS, exit 0 | `build-artifacts/runtime-decomposition/b1-test-final.log` |
| Actual native-adapter matrix | PASS: 42 scenarios × Diagnostic Mode OFF/ON × ordinary/supported-diagnostic harness profiles = 168 fresh fixture processes | Same test log; separate-TU owner probe linked and passed |
| Production build | PASS, exit 0 | `build-artifacts/runtime-decomposition/b1/STALKER2CameraTweaks-B1.asi`; SHA-256 `F571F73889622E88B7C7BD29F9607F16B041A8738B3A7B785EC83935BBAC7D7A` |
| Supported diagnostic build | PASS, exit 0 | `build-artifacts/runtime-decomposition/b1/STALKER2CameraTweaksDiagnostic-B1.asi`; SHA-256 `AEE44C9DBB1F7EDB9C0679C14561BB7C49C8C2A5CF5ED98125F552346D2AB043` |

Builds emitted the already observed vendor Zydis C4201 warnings; neither build failed. Artifacts and object files are isolated under the B1 evidence directory and the root ASI was not overwritten.

## Limits and next gate

No game was launched and no ASI installed. Live hook installation, startup/reset interleavings, compositor behavior, and runtime camera lifecycle are not established by these offline checks. V0-A and V0-B remain partial and gate their dependent later batches; V0-C orchestration remains a gate for B7. B2 is not authorized by this result.
