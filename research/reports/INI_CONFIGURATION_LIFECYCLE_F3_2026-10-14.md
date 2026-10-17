# F3 — INI Configuration Lifecycle & Persistence

**Status:** Complete — focused offline validation passed  
**Scope:** `STALKER2CameraTweaks.ini` startup synchronization and Overlay persistence  
**Validation boundary:** focused `config_persistence_harness` only; no full suite, production ASI build, game launch, or installed-ASI change.

## Outcome

The INI now behaves as a persistent settings file rather than a file that is regenerated as part of every startup. A missing file is created from the canonical template. An existing file is repaired only when its managed structure differs from the canonical contract. A valid file is not replaced. Overlay persistence avoids writes for unchanged values and limits an actual update to the requested assignment(s).

The user explicitly clarified that user-authored notes/comments are not retained. Accordingly, startup repair removes arbitrary comment lines and regenerates canonical module documentation. It preserves valid setting values, unknown keys, and custom sections; comments are treated as module documentation, not user-owned data.

## Root cause

Three independent write/rewrite causes were present in the audited paths:

1. Template synchronization compared normalized output against the original line vector, but repair was not fully idempotent. Blank lines accumulated around managed entries, and the special `[Overlay].Enabled` block relocation counted documentation lines without accounting for the explicit managed-block boundary lines. Repeated synchronization could therefore produce another textual difference and replace the file again.
2. `PersistConfigValues` and `PersistOverlayPlacement` always reached atomic replacement after `SetValue`, even when the effective value already matched. `SetValue` also reformatted unrelated assignments in the target section.
3. `LoadFeatureConfig` created an empty file before template synchronization. If canonical generation failed, that empty file could be left behind instead of leaving the path absent.

## Changes made

- `src/config/config_repository.cpp`
  - `SetValue` updates only the value span of matching assignments and leaves unrelated assignment formatting untouched.
  - `PersistConfigValues` and `PersistOverlayPlacement` compare the resulting lines with the original and return success without replacing the file on a no-op.
  - Missing-file creation is delegated to the canonical template writer; a failed creation no longer leaves an empty placeholder.
  - Persistence reads and writes retain the input newline convention and whether the file ended with a newline.
- `src/config/config_template.cpp`
  - Existing comments are replaced with canonical documentation; explicit managed-block markers identify canonical description blocks.
  - Repair removes unstable blank-line accumulation and relocates the Overlay Enabled documentation by its marker, not by an assumed number of comment lines.
  - Missing-file generation and repairs use the existing staged write and atomic replacement strategy. Existing line endings and final-newline state are retained for repaired files.
- `src/config/config_line_reader.hpp`
  - The checked reader can report the file's newline style and final-newline state while preserving the existing incomplete-read rejection behavior.
- `tests/config/config_persistence_harness.cpp`
  - Added coverage for exact no-op bytes/file identity/timestamp, isolated persistence, LF and CRLF formats, no final newline, comment removal with canonical documentation restoration, and read-only failure preservation.
  - Existing R4a/R4b persistence and migration contracts remain in the focused harness.

## Before / after

| Scenario | Before | After |
|---|---|---|
| INI absent | Empty placeholder was created before template synchronization; failed synchronization could leave it behind. | Complete canonical INI is staged and atomically installed; failure does not leave an empty placeholder. |
| Valid INI at startup | Repeated canonicalization could change spacing/managed documentation and trigger replacement. | Canonical output is idempotent; no write preserves bytes, timestamp, and file identity. |
| Missing/outdated managed description or supported key | Repair could repeatedly reformat sections or fail to converge. | Canonical documentation and defaults are restored while valid values, unknown keys, and custom sections remain. Arbitrary comments are removed per user clarification. |
| Overlay changes an existing value | Setter reformatted other assignments and could replace the file even for an unchanged value. | Only the requested value span changes; an unchanged effective value returns without replacement. Newline style and final-newline state are retained. |

## Focused validation

- Target build: `config_persistence_harness`; Ninja exit code **0**, no timeout, process-tree cleanup verified. Ninja's `vctip.exe` helper was handled by the supervisor's exact-image cleanup policy.
- Harness: exit code **0**, no timeout, no surviving descendants; all emitted cases passed.
- Covered assertions include: missing-file canonical generation; canonical initial layout; byte/identity/timestamp stability on a no-op startup and persistence; CRLF update and LF/no-final-newline update; duplicate placement and multi-value transaction behavior; staging, replacement, and read-only failure preservation; legacy Overlay toggle, Cinematics, and Dialogue migrations; stale/modified documentation repair; arbitrary comment removal and stable repeat startup; unknown values, keys, and custom sections; existing R4a/R4b read, write, ordering, and feedback contracts.
- Temporary harness directory was removed. Post-run checks found no `ninja`, `cl`, `config_persistence_harness`, or `vctip` process remaining.
- No full `test.cmd`, production ASI build, game launch, or installed-ASI modification was performed.

## Preserved contracts and limitations

- Existing INI keys/defaults, migration precedence, camera/FOV behavior, and startup-only versus runtime-changeable semantics were not intentionally changed.
- R4a read-integrity and duplicate-key behavior and R4b ordered persistence/failure feedback remain in place.
- Unknown keys and custom sections are preserved. User comments/notes are intentionally not preserved, as explicitly directed by the user; canonical documentation is restored instead.
- If the file is valid but uses a noncanonical newline convention, no-op synchronization leaves it untouched. If repair is needed, that newline convention and final-newline presence are preserved.
- This focused result is not a substitute for F4 final qualification. No claim is made about live runtime behavior or the final release artifact.

## Changed files

- `src/config/config_line_reader.hpp`
- `src/config/config_repository.cpp`
- `src/config/config_template.cpp`
- `tests/config/config_persistence_harness.cpp`
- `research/plans/FINAL_RELEASE_HARDENING_REMEDIATION_PLAN_2026-10-13.md`
- `research/reports/INI_CONFIGURATION_LIFECYCLE_F3_2026-10-14.md`

## F3 release-ownership follow-up

The release model now treats the INI as a runtime-created user file rather than a deployed package asset. The explicit archive allowlist is production ASI, README, license and third-party notices; the package builder validates the exact ZIP inventory and rejects any additional entry, including an INI. Vortex copy metadata and current installation instructions no longer deploy the INI.

Known older layouts continue through the existing migrations without resetting compatible values. For structurally malformed or unrecognized content, synchronization first creates a timestamped, non-overwriting sibling backup. Recovery retains syntactically valid assignments under valid sections—including custom sections—while malformed fragments are omitted; if backup or atomic replacement fails, the original remains in place. No schema-version key was introduced.

Focused validation for the release-ownership follow-up: `config_persistence_harness` rebuilt with Ninja and passed, covering malformed recovery with byte-identical backup, unknown foreign sections, UTF-8 BOM legacy migration without unnecessary backup, compatible Gameplay/Cinematics/Dialogue/Diagnostics/Overlay/Hotkey values, and repeat-run idempotence. The process supervisor confirmed cleanup. A bounded packaging smoke produced a temporary ZIP containing exactly four allowlisted entries, zero INI entries, and an ASI SHA-256 matching its release input; the temporary ZIP was removed and the packaging process tree exited cleanly. No full suite, production ASI build, official archive rebuild, installed-ASI change, or game run was performed. Historical ZIP artifacts are not rewritten here; they predate this packaging rule and are not evidence for the next release archive.
