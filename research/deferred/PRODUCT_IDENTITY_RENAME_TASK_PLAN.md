# Product Identity Rename Task Plan

Status: ACTIVE — dialogue lifecycle invalidation defect fixed and bounded
regression passed. Rename promotion is resumed.

## Objective

Rename the current v0.5.0 product identity from `STALKER2UltrawideFix.*` to
`STALKER2CameraTweaks.*` and use the canonical public name:

`STALKER 2 Ultrawide and Camera Tweaks for UE 5.5.4`

The rename must be complete for the current production artifact, configuration,
log, build/package flow and current public documentation while preserving
historical names where they are factual evidence.

## Established evidence and current state

- The current public product is expanding beyond the original ultrawide-only
  scope to gameplay, cinematics, dialogue FOV, future weapon/viewmodel FOV and
  projection-related work.
- The current production technical identity is now
  `STALKER2CameraTweaks.asi`, `STALKER2CameraTweaks.ini` and
  `STALKER2CameraTweaks.log`.
- The current production candidate includes the validated dialogue FOV-context
  invalidation fix; runtime regression passed on Steam 2.0.4.
- The current release preparation artifacts and reports refer to the old
  technical identity and must remain historical evidence unless they describe
  the new current product state.
- The existing repository URL and historical release names are not part of the
  rename.
- No v0.5.0 package has been published to GitHub or Nexus.

## Approved scope

- Freeze the canonical public and technical names above.
- Inventory every old-name reference and classify it as current production,
  current documentation, build/package input, migration guidance, or historical
  evidence.
- Rename current production source-derived filenames and runtime-created names.
- Update the build and packaging flow to produce the new ASI, INI, log and ZIP
  names.
- Update current README, release notes, GitHub release body, Nexus description,
  release playbook and current research summary where they describe the current
  product.
- Add file-removal guidance for users upgrading from both
  `STALKER2UltrawideFix.*` and `STALKER2GameplayAspectFix.*`.
- Do not migrate settings from either previous INI file; retain only current
  INI template synchronization for the newly named configuration.
- Build the renamed production candidate and establish new binary/archive
  hashes.
- Run a bounded rename/migration smoke regression.
- Perform a final read-only Git review against this plan.

## Explicit non-goals

- No gameplay, cinematic, dialogue, weapon/viewmodel or Panini behavior changes.
- No resolver, hook, state-machine or FOV mathematics changes.
- No global replacement in historical reports, old task plans, runtime logs,
  old release records or Git history where the old name is a factual reference.
- No repository URL/slug rename.
- No GitHub/Nexus publication, tag, push or commit as part of this task.
- No deletion of old release artifacts or research evidence without separate
  approval.

## Expected files and areas

- Production source and build scripts that define the current ASI/config/log
  names and output paths.
- Current public documentation and release-facing text.
- `release-assets/` current whitelist and archive output.
- Historical research/release reports only where a current-state statement must
  be distinguished from preserved historical evidence.

## Batches

### Batch 1 — Inventory and classification

- Search the repository and workspace for old technical names.
- Record exact paths and classify each reference.
- Confirm current production build entry point and package whitelist.
- Stop before edits if a reference cannot be classified safely.

Batch 1 result:

- Current production runtime names are defined in
  `src/experimental_cinematic_21_9_combined_fix_204.cpp`.
- Current production build/package references are in
  `build-artifacts/test-scripts/build-experimental-cinematic-21x9-combined-fix-204.cmd`
  and `release-assets/`.
- `build.cmd`, `src/gameplay_aspect_fix.cpp` and their related candidate scripts
  are legacy gameplay-only/test paths, not the current unified production
  identity; they remain historical/test references.
- Current public documentation and the release playbook require new-name
  updates. Historical reports, task plans, logs and old release archives retain
  old names as factual provenance.
- No unclassifiable current production reference was found.

Batch 1 gate: PASS.

### Batch 2 — Production identity rename

- Update only current source/build/package names.
- Keep old names in explicit migration/removal instructions.
- Preserve historical names in research evidence and old release records.
- Validate that the source contains no accidental dependency on the old current
  filename outside migration or historical documentation.

Batch 2 result: PASS for the current unified source and production build
script. Runtime-created ASI, INI, log and logger names now use
`STALKER2CameraTweaks`.
Legacy INI migration was removed from the unified source; current INI template
synchronization remains enabled.

### Batch 3 — Current documentation and release materials

- Update current product-facing names and installation instructions.
- Keep the canonical public name consistent across README, Nexus, release notes,
  GitHub release body and current project summaries.
- Do not rewrite historical evidence merely to match the new branding.

Batch 3 result: PASS for current public/release documentation. Historical
reports and legacy/test instructions remain unchanged except where current
package or migration wording required an update.

### Batch 4 — Build, package and identity validation

- Build the renamed production ASI from the final source state.
- Confirm the new INI and log are created/read using the new names.
- Rebuild the release-assets whitelist and archive with the new names.
- Record new ASI and archive SHA-256 values.

Batch 4 result: PASS for build and package preparation.

- New ASI SHA-256 after dialogue lifetime invalidation and production rebuild:
  `90CFF6F4B641F365496179EBF21490EDDD07781E72F3CCE0A47592E4B570F92F`
- New INI SHA-256:
  `5E653614FD3E3D00F359DDF37335635084EC25C70FC52345ACCEFE9730DC680A`
- New archive:
  `release-assets/STALKER2CameraTweaks-UE5.5.4-v0.5.0.zip`
- New archive SHA-256 after dialogue lifetime fix:
  `CBBDAB826A1C67AA56E24C7C8135E83861ECD3B6F734C56A628DEDB095A9B373`
- Clean extraction and exact five-file set: PASS.

### Batch 5 — Rename/migration smoke regression and final review

- Verify the renamed ASI loads on Steam 2.0.4.
- Verify new INI defaults, policy persistence and hotkey configuration are read.
- Verify the new log is created and records runtime identity.
- Verify old files are covered by migration guidance and are not loaded together.
- Verify no camera behavior changed relative to the already validated build.
- Compare actual changes with this plan and inspect Git status/diff, including
  staged and untracked paths.

## Validation

- Inventory has a recorded classification for every old-name reference in the
  current production/documentation/build/package scope.
- Production source/build output uses only the new current technical identity.
- Historical references remain factual and are not silently rewritten.
- New ASI loads and new INI/log paths work on the tested Steam 2.0.4 setup.
- New archive contains only the approved renamed production files.
- New archive extraction and per-file hashes are verified.
- Existing gameplay, cinematic and dialogue behavior is not intentionally
  changed by the rename.
- `git diff --check` passes for edited text files.

## Risks and rollback / safe-failure

- A missed filename reference could prevent configuration, logging or packaging
  from working. Mitigate with exact-name search before and after edits.
- Existing users may still have old files. Installation instructions must require
  removing old ASI/INI/log files before loading the renamed product.
- A failed rename build or smoke test must not replace the existing validated
  candidate. Keep old release artifacts untouched and stop before packaging
  promotion.
- If behavior changes beyond naming, stop and return to the pre-rename source
  state for diagnosis; do not adjust camera logic inside this task.

## Stop conditions and phase gates

- Stop if current and historical references cannot be separated reliably.
- Stop if the renamed binary does not build, load or create/read the new files.
- Stop if any camera/FOV behavior differs from the already validated contract.
- Stop before publication and before any Git commit/tag/push.
- The task is complete only after the renamed package and new hashes pass the
  bounded validation and the final Git review passes.

## Expected final Git review

- Report current production files renamed or updated.
- Report historical files intentionally untouched.
- Report generated artifacts excluded from repository scope.
- Report new ASI/archive hashes and smoke-test limits.
- Report remaining migration limitations or deferred work.
- Do not stage, commit, tag or push without separate explicit approval.
