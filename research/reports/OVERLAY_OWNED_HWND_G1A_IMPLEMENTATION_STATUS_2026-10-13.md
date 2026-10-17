# Overlay-owned HWND — G1-A implementation status

Date: 2026-10-13  
Status: **Partial; G1-A acceptance is not met. G1-B is not started.**

## Implemented in the authorized G1-A slice

- Added a monotonic callback admission gate with an in-flight count. Once
  terminally closed, it rejects new callbacks; the last admitted callback can
  observe that the drain condition has been reached.
- Replaced the mutable, mutex-protected game-WndProc binding vector with an
  immutable per-HWND property record. Records are retained after publication
  so a foreign subclass chain cannot reference reclaimed continuation memory.
  The current record is separately published atomically; stale records can
  forward only through their saved native continuation.
- Changed the GameHwnd WndProc path to try-acquire the Renderer mutex and pass
  the current message to the saved game procedure when the lock is unavailable
  or terminal admission is closed. The callback does not synchronously remove
  the subclass or call Renderer shutdown on exception.
- Added a manual-reset terminal wake event before starting the Overlay worker.
  The exception boundary closes admission, attempts nonblocking bridge
  revocation, signals the event and posts only a supplemental queue wake.
- Added try-only bridge activation/deactivation and queue insertion. A lock
  failure or queue saturation drops capture, fails the current event open and
  causes the owner-side drain to publish ImGui Reset/Focus-lost state.
- Mouse and raw-input messages are consumed only when the bridge accepted the
  event; failed delivery passes through to the original game procedure.
- Rebind message handling, toggle, focus cancellation and commit paths now use
  try-lock operations where touched by the game-window callback.

## Validation performed

- Focused `input_state_harness`: **PASS**, process exit code 0. It covers
  terminal admission/count/drain, concurrent close, try-deactivation and queue
  saturation/reset behavior.
- Localization/resource preflight integrated with the CMake production target:
  **PASS**, 18 locale catalogs and embedded resources.
- Isolated production-profile build target in
  `build-artifacts/cm/test-suite`: **PASS**, including compilation of
  `composition_runtime.cpp`; no ASI was copied to the repository root,
  installed or run in game. The first direct CMake attempt without the MSVC
  environment failed because `Windows.h` was unavailable; rerunning through
  the configured Visual Studio developer environment succeeded.
- Build emitted two existing vendor SafetyHook/Zydis C4201 warnings. No new
  warning was emitted from the changed project-owned translation units.
- Full `test.cmd`, diagnostic build, G0/G1 integration fixtures and game
  runtime were not run in this G1-A slice.

## Why G1-A remains incomplete

The production callback is not yet a fully nonblocking GameHwnd boundary.
After successfully acquiring the Renderer mutex with `try_lock`,
`ProcessOverlayWindowMessage` can still perform synchronous settings
`Apply`/`Persist`/`Snapshot`, notification publication and logging/file flush
operations on the game thread. The try-lock changes do not make those calls
nonblocking. Some rebind state transitions also remain split across separate
operations rather than one atomic command/admission contract.

The terminal event is created and signaled, but the current owner loop still
uses `GetMessage` and does not wait on or consume that event. Therefore event
signal alone is not yet an owner-finalization guarantee; queue posting remains
only a hint until the separately designed event-aware owner pump/finalizer is
implemented. This does not invalidate fail-open game input, but it prevents
claiming terminal cleanup delivery.

The new continuation property and cross-thread subclass installation compiled
but were not exercised in a production WndProc fixture. HWND reuse,
concurrent foreign subclass replacement, WM_NCDESTROY chain behavior and
callback drain under a stalled owner remain unverified.

## Changed files for this slice

- `src/overlay/composition/composition_runtime.cpp`
- `src/overlay/input/input_state.cpp`
- `src/overlay/input/input_state.hpp`
- `tests/overlay/input_state_harness.cpp`
- `tests/runner/localization_catalog_audit.ps1` — adjusted the existing
  source-contract check to recognize accepted-only mouse-button wake
  scheduling.

The working tree contained unrelated user edits before this slice. They were
preserved; no attempt was made to reset, stage, or clean the tree. A Git diff
could not be obtained in this environment (`git diff` reported that the
operation must be run in a work tree), so this report lists the files directly
edited for G1-A rather than claiming a complete repository diff review.

## Artifact identity

The isolated production-profile candidate at
`build-artifacts/cm/test-suite/bin/STALKER2CameraTweaks.asi` has SHA-256
`4B5D2A9CCEB006186480993C50A97090CD8E5D97DABFECA69C10205AB4682C25`.
It is a local build output only, not the approved experimental/live artifact.

Source SHA-256:

| File | SHA-256 |
| --- | --- |
| `src/overlay/composition/composition_runtime.cpp` | `343C3F87758B481BE29603569901DC1BAD9739581507E796B3DD6FB462089D57` |
| `src/overlay/input/input_state.cpp` | `E07CE5275FEE118F8C5F0EAA383E354588FF70F45C407F2301B7EE03FC0917FC` |
| `src/overlay/input/input_state.hpp` | `BDDA8F4F78DA6364EEEF277383CEFDFF5136A298F26116115E8E53CA633A41DB` |
| `tests/overlay/input_state_harness.cpp` | `27004220E7ABBCFD9598C7D48DAEAB5F1B50F71871A803A7C510D7902D05C428` |
| `tests/runner/localization_catalog_audit.ps1` | `EE1107FB064A04E15051D61FAF16D1BC2A5F8C222FBE54F757F50280543475D2` |

## Disposition

G1-A is **partial**, not PASS. Do not start G1-B or replace/install an ASI.
The remaining game-thread synchronous operations and lack of an event-aware
owner consumer must be resolved within an explicitly approved follow-up
boundary before the G1-A acceptance gate can be claimed.
