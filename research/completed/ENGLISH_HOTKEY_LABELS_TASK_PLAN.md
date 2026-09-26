# English Keyboard Labels Task Plan

## Objective
Make every displayed hotkey name deterministic and English regardless of the active Windows keyboard layout.

## Established Evidence And Current State
- `config::HotkeyName` uses `GetKeyboardLayout` and `MapVirtualKeyExW` for OEM punctuation and `GetKeyNameTextW` for fallback names; those APIs may reflect the active layout.
- Bindings are identified and persisted by Win32 virtual-key code (`VK_XX`), so display-name resolution can be made independent without changing binding identity.
- Existing persistence tests verify OEM key round-trips but do not assert canonical English labels.

## Approved Scope
- Change only hotkey display-name resolution and deterministic tests.
- Use fixed English/US-style labels keyed by virtual-key code, with a stable `VK_XX` fallback.
- Preserve capture, key support, conflict rules, persistence, runtime routing, and overlay wording behavior except for the resolved label.

## Explicit Non-Goals
- No changes to binding capture or supported-key policy, including standalone Ctrl/Shift behavior.
- No changes to configuration format, actions, overlay input policy, or camera/gameplay logic.
- No game launch or runtime injection.
- No Git commands or Git state changes, per explicit user instruction.

## Expected Files / Areas
- `src/config/feature_config.cpp`: deterministic key-name mapping.
- `tests/config/config_persistence_harness.cpp`: exact-label and round-trip assertions.
- This plan, archived under `research/completed` after successful validation.
- `backlog/TASKLOG.md`: factual implementation/validation record.

## Batches And Validation
1. Replace layout-sensitive and OS-localized display lookups with stable English labels for supported key families; extend deterministic identity/persistence/display tests. Validate with `test.cmd`.
2. Build the overlay settings target with `build-overlay-settings.cmd`; confirm the display path no longer consults active-layout or localized key-name APIs. Do not launch the game.

## Risks And Safe-Failure Behavior
- A label may be ambiguous for unusual OEM keys/layouts. Use explicit English labels or stable `OEM xx`/`VK_XX` fallback rather than querying the active layout.
- The underlying VK identity and config representation remain unchanged, so rollback is limited to the display resolver and its tests.

## Stop Conditions / Phase Gates
- Stop if implementation would require changing key identity, capture, persistence, or supported-key policy.
- Stop if deterministic tests or overlay build fail; report the failure without claiming runtime validation.
- No runtime/game validation is authorized or necessary for this presentation-only change.

## Expected Final Review
- Compare changed paths and outcomes to this plan using direct file inspection only. Git review is intentionally omitted because the user explicitly prohibited Git interaction.
