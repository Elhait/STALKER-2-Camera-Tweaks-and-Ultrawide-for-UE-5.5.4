# Pinned reference: G-SYNC Indicator returns after menu close

Reference ID: ROOT_DETACH_GSYNC_RETURN_2026-10-12.
Pinned on user request; historical original experiment C, **not target release**.
This is an artifact identity, not a new public mod version.

Local binary: [STALKER2CameraTweaks.asi](STALKER2CameraTweaks.asi).
SHA-256: `78357B990DC705DD3973339E0EA4FE733E2484A2DCB1461F7C33F859BC2B624E`.
Copied byte-for-byte from build-artifacts/fp/C; hash rechecked after copying and
binary marked read-only to discourage accidental overwrite. Original is retained.
ASI is excluded from Git by the existing *.asi rule; this document records its
local preservation, not a Git-hosted/downloadable release binary. Keep its basename.

## Evidence and bounded result

STALKER2CameraTweaks.log identifies this SHA at 2026-10-12 15:46:57.421 local time.
Game EXE SHA-256:
`61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
User reported: G-SYNC Indicator disappears while the mod menu is open and returns
when it closes. Three root attach/detach cycles are recorded in the Overlay log.
The target remains allocated. Prototype observation counters remain in this ASI.

This reference does not prove actual VRR status, NVIDIA Performance HUD coexistence,
FG pacing or stability in every environment. HUD was not part of that first
reported test. Do not merge it with later 5D2A target-release observations.

Historical INI included for provenance, **not a request to replace current user
settings**. Its SHA-256:
`508F99B0D0EAD161E93955C32909C56CFA33E4C15D875EC468B0E56444CAFE4D`.
Preserved alongside binary: original three live logs and their hash manifest,
source_manifest.json and experiment_inputs.json from the original preparation.

Related production root-detach implementation BF5B...ABCD is a different binary;
its own test results do not replace the original C live evidence.

History: [target experiment and artifact reconciliation](../../../reports/DCOMP_TARGET_RELEASE_EXPERIMENT_2026-10-12.md).
Research: [persistent-target alternatives](../../../reports/DCOMP_PERSISTENT_TARGET_COEXISTENCE_RESEARCH_2026-10-12.md).
