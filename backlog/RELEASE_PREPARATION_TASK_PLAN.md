# STALKER 2 Ultrawide and Camera Tweaks — Reusable Release Preparation and Publishing Playbook

This is the canonical reusable release-preparation and publication reference
for the project. It is a permanent process document, not a release-specific
task plan.

Before asking how a GitHub Release or Nexus field should be filled, check this
document and the previous release convention first. Ask only when the field or
UI is new, the convention is ambiguous, the UI changed, or the current release
intentionally differs.

Do not mark a release `READY` while a mandatory gate is incomplete.
Do not create a commit, tag or publication automatically.

A batch may be marked `PASS` only when:

1. its required outputs were produced;
2. its explicit validation was performed;
3. the validation evidence was recorded; and
4. no unresolved mandatory finding remains.

Do not begin a dependent batch while its prerequisite is `FAIL`,
`BLOCKED` or `NOT VALIDATED`.

When a later batch changes or invalidates an earlier output, reopen that batch
and every dependent batch. Validation batches are no-edit by default: they
report failures and route them back to the owning batch instead of silently
repairing them.

Dependency invalidation:

| Changed output | Reopen |
| --- | --- |
| A0 scope | all affected downstream batches |
| A1 evidence/claims | A4 onward |
| A2 source or binary | A2, A5, A6 and A7 |
| A3 provenance/attribution | A1 onward if new completed work, evidence or reference claim is discovered; A4 onward if only paths or attribution wording changed |
| A4 documentation | A5, A6 and A7 when derived content changes |
| A5 release assets | A6 and A7 |
| A6 archive | A6 verification and A7 |
| Repository-only commit scope | A8 only, unless content changes are found |

## Phase A — Release preparation

Execution order:

```text
A0 Scope lock
  → A1 Evidence / release delta / claim matrix
  → A2 Production binary gate
  → A3 Plans / provenance / attribution
  → A4 Documentation
  → A5 Release assets
  → A6 Archive construction + extraction
  → A7 Audit-only final consistency
  → A8 Git pre-commit review
  → Final Phase-A report
  → READY FOR USER COMMIT APPROVAL
```

### A0 — Release scope lock

Record before opening later batches:

- release version and previous release;
- promoted production features;
- accepted known issues and limitations;
- explicitly excluded, experimental, unresolved and deferred work;
- target production source state and approved runtime evidence.

Gate: release scope, promoted features and non-goals are unambiguous. If scope
changes, reopen A0 and all affected downstream batches.

### A1 — Evidence, release delta and claim matrix

Before editing release-facing documentation, review:

- completed task plans and research reports from the current release cycle;
- runtime-validated behavior and exact executable identity;
- static-only and cross-patch evidence;
- promoted production changes;
- deferred/rejected work and accepted limitations;
- external reference mods and required attribution.

Required output must contain the actual:

- `previous release → current release` delta;
- completed work list;
- runtime/static/deferred/rejected classification;
- evidence-to-claim matrix:

```text
feature or claim → evidence → status → allowed public wording
```

Do not mark A1 `PASS` merely because the plans were reviewed. The actual
delta and matrix must be present in the batch result, and every completed
release feature must appear in them.

Gate: no completed in-scope feature is missing and no public claim exceeds its
evidence. Until A1 is `PASS`, do not edit release documentation.

### A2 — Production binary gate

- [ ] Confirm every feature in scope completed bounded runtime/regression
  validation.
- [ ] Exclude experimental, unresolved or unvalidated functionality.
- [ ] Identify the approved production binary separately from test, debug and
  diagnostic builds.
- [ ] Do not select the newest `build-artifacts/test-asi` file merely because it
  is newest.
- [ ] Record the expected release version from the approved production source.
- [ ] Record the final production ASI SHA-256.
- [ ] Record accepted known issues and limitations.
- [ ] Confirm the selected binary was built from the final release source state.
- [ ] Confirm feasibility policies, temporary bindings, diagnostic logging and
  test-only instrumentation are absent.
- [ ] Record runtime validation and static portability separately.
- [ ] Record downstream documentation/template/assets mismatches as required
  work for A4/A5; do not require them to be synchronized yet.

Gate: binary identity, source correspondence, diagnostic exclusion and runtime
scope are `PASS`. Do not create or refresh release assets before A2 passes.

### A3 — Plans, provenance and attribution

- [ ] Review plans in the project root, `backlog` and research archives.
- [ ] `backlog/RELEASE_PREPARATION_TASK_PLAN.md` is permanent; do not archive,
  move or mark it completed for an individual release.
- [ ] Move completed plans to the appropriate completed/archive location.
- [ ] Keep active and deferred plans in `backlog`.
- [ ] Do not archive unfinished work merely because a release is being prepared.
- [ ] Update links to moved plans and reports.
- [ ] Preserve research provenance and historical negative results.
- [ ] Create future plans in the appropriate `backlog` structure by default.
- [ ] Check whether external references require attribution in `README.md`,
  `NEXUS_DESCRIPTION.md` or `THIRD_PARTY_NOTICES.md`.
- [ ] State whether each reference supplied a clue, semantic anchor, behavior
  comparison or independently reproduced implementation.
- [ ] Do not imply external code/assets were copied when they were not.
- [ ] Verify license and permissions before including external code or assets.

Validation:

- [ ] Recheck links after plan/report moves.
- [ ] Confirm every plan is classified as completed, active, deferred or
  rejected.
- [ ] Confirm attribution wording is accurate and consistent.

Gate: plans, provenance and attribution `PASS`. If A3 changes referenced
documentation, A4 and all later dependent batches must run afterward.

### A4 — Documentation batch

Review and update from the A1 evidence matrix, not from the INI alone:

- [ ] `README.md`
- [ ] `RELEASE_NOTES.md`
- [ ] `TESTING_AND_RESEARCH.md`
- [ ] `GITHUB_RELEASE_BODY.md`
- [ ] `NEXUS_DESCRIPTION.md`

The documentation must clearly distinguish runtime validation, static
cross-patch validation, known limitations, experimental/deferred work and
actually supported game versions. `TESTING_AND_RESEARCH.md` must match the
completed plans, reports and runtime evidence of the current cycle.

`README.md` is a GitHub/repository-facing Markdown document. `NEXUS_DESCRIPTION.md`
is a publication source for the Nexus Mods Description field: preserve the
formatting conventions of the current live Nexus description, including its
heading style, emphasis, links, lists, spacing and configuration/code blocks.
Do not reformat the Nexus source merely to make it resemble GitHub Markdown.
The `.md` filename does not require GitHub Markdown when the target publication
platform uses Nexus-compatible BBCode such as `[b]`, `[list]`, `[code]` and
`[url]`. Update the existing Nexus presentation incrementally for each release;
keep detailed reverse-engineering evidence in `TESTING_AND_RESEARCH.md` and
research reports, not in the user-facing Nexus description.

Validation:

- [ ] Every completed release feature is represented.
- [ ] Runtime, static-only, deferred and rejected claims are separated.
- [ ] Known limitations, supported versions and attribution are correct.
- [ ] Version, defaults, hotkeys, restart behavior and compatibility agree.
- [ ] README and Nexus text are semantically cross-checked.
- [ ] No stale contradiction remains between public documents.

Gate: documentation `PASS`. A documentation failure blocks A5–A8.

### A5 — Release assets batch

- [ ] Update the compact user-facing `release-assets/README.txt`.
- [ ] Confirm the user-owned runtime INI is not included in release assets.
- [ ] Review asset names, installation instructions and compatibility.
- [ ] Keep test ASIs, tracers, logs, research, plans and debug artifacts out.
- [ ] Confirm no feasibility-only policy, diagnostic option or temporary binding
  is documented or implied by the distributed assets.
- [ ] Confirm the release-assets README is derived from the A4 documentation.

Required result: a clean release-assets set matching the approved binary,
production INI and documentation. Do not create the ZIP in A5.

Gate: assets `PASS`. Any affected asset or documentation change reopens A5, A6
and A7.

### A6 — Archive construction and extraction

Create the archive in:

```text
E:\Work\Slaker2 mods\01-Projects\STALKER-2-Ultrawide-Fix-for-UE-5.5.4\release-assets
```

Normally include only:

- final production `STALKER2CameraTweaks.asi`;
- `README.txt`;
- `LICENSE.md`;
- `THIRD_PARTY_NOTICES.md`.

Do not include old binaries, source, plans, research, logs, tracers, debug
artifacts or build scripts without a documented reason. Do not delete previous
archives without separate approval.

Validation:

- [ ] Create the archive only from the A5 release-assets set.
- [ ] Extract into a clean temporary directory.
- [ ] Verify extracted structure and required files.
- [ ] Confirm extracted ASI is byte-identical to the approved binary.
- [ ] Record ASI and archive SHA-256 values.

Gate: archive `PASS`. Do not silently edit archive contents after verification.

### A7 — Audit-only final consistency

Compare only, without repairing:

```text
A1 evidence/claim matrix
  ↕
source/config
  ↕
README / Testing / Release Notes / GitHub / Nexus
  ↕
release-assets
  ↕
extracted ZIP
  ↕
versions, defaults, attribution and hashes
```

- [ ] Confirm all release-facing claims are supported by A1 evidence.
- [ ] Confirm documentation, INI, assets and extracted ZIP agree.
- [ ] Confirm version and approved ASI SHA-256 agree everywhere relevant.
- [ ] Confirm known limitations and runtime/static boundaries are consistent.
- [ ] Run `git diff --check` only as a formatting check; repository scope
  classification belongs to A8.

A7 is an explicit no-edit batch. If it finds a required change:

1. mark A7 `FAIL`;
2. identify the owning earlier batch;
3. make the correction there;
4. rerun every invalidated dependent batch;
5. return to A7.

Gate: A7 `PASS` only when the complete release content is consistent.

### A8 — Git pre-commit review

Only after A0–A7 pass:

- [ ] Inspect `git status`.
- [ ] Inspect the full relevant tracked diff.
- [ ] Inspect staged and unstaged changes separately.
- [ ] Inspect all untracked files; do not assume `git diff` includes them.
- [ ] Compare changed, added, deleted and moved paths with the approved scope.
- [ ] Classify release, completed research, active/deferred research, generated
  artifacts and unrelated/ambiguous changes.
- [ ] Review staged path moves and detect stale index entries.
- [ ] Run `git diff --check`.
- [ ] Propose the exact commit scope.
- [ ] Propose one concise commit title derived from the actual final diff.
- [ ] Do not create a commit automatically.

Repository-only observations may remain within A8. If the review finds a
content change that affects A2–A7, reopen the affected batch instead.

### Final Phase-A report

The report is written or finalized only after A8:

```text
A0 Scope lock: PASS/FAIL
A1 Evidence inventory: PASS/FAIL
A2 Production binary: PASS/FAIL
A3 Plans/provenance/attribution: PASS/FAIL
A4 Documentation: PASS/FAIL
A5 Release assets: PASS/FAIL
A6 Archive: PASS/FAIL
A7 Final consistency: PASS/FAIL
A8 Git pre-commit review: PASS/FAIL

Release version: <version>
Production ASI SHA-256: <sha256>
Release archive: <archive-path>
Archive SHA-256: <archive-sha256>
Known limitations: <summary>
Preparation status: READY / BLOCKED
Proposed commit title: <title>
```

If a mandatory gate fails, report `BLOCKED` with the exact reason. Only when
A0–A8 are `PASS` may Phase A be marked:

```text
Phase A: READY FOR USER COMMIT APPROVAL
```

## Phase B — Publishing

Execution order:

```text
B0 Commit after explicit approval
  → B1 Tag and push
  → B2 GitHub Release
  → B3 Nexus Release
  → B4 Post-publish verification
```

### B0 — Commit

Begin only after Phase A is `READY` and the user explicitly approves the
exact commit scope.

- Create only the approved commit.
- Record commit id and working-tree result.
- Stop if the actual diff differs from the approved scope.

### B1 — Tag and push

- Use the established tag convention; do not invent a new one.
- Create the approved tag.
- Push the approved commit and tag.
- Record the resulting identifiers.

### B2 — GitHub Release

- Create or update the GitHub Release using `GITHUB_RELEASE_BODY.md`.
- Upload the final verified archive.
- Verify version, title, target branch, prerelease/draft state and asset.
- Recorded execution for v0.5.0: tag `v0.5.0`, target `main`, title
  `v0.5.0 — Dynamic Ultrawide Gameplay, Cinematics, Dialogue FOV & Custom
  Framing`, Latest release selected, and archive
  `STALKER2CameraTweaks-UE5.5.4-v0.5.0.zip` uploaded.
- If the UI changed, adapt the playbook when mapping is unambiguous; ask only
  for a genuinely new or ambiguous decision.

### B3 — Nexus Release

- Create or update the Nexus release using `NEXUS_DESCRIPTION.md`.
- Upload the same verified archive.
- Verify file display name, category, compatibility selections, changelog and
  main-file replacement/coexistence choice.
- Recorded execution for v0.5.0: updated the existing `0.4.0` Main file,
  archived the previous file, kept the new file as primary and enabled mod
  manager downloads and the requirements popup.
- Recorded display name: `STALKER 2 Ultrawide and Camera Tweaks by Elhait`.
- Recorded file description: `Unified gameplay, cinematic and dialogue FOV
  fixes for Steam 2.0.4 / UE 5.5.4. Adds Adaptive and Reduced dialogue zoom,
  custom cinematic framing, dynamic aspect correction and guarded signature
  resolution.`
- Recorded changelog: `Added FOV-aware dialogue zoom: Native, Adaptive,
  Reduced and Disabled. Smooth EXIT recovery for dialogue lifecycle.
  Configurable F9/F10 next-lifecycle hotkeys. Renamed production identity to
  STALKER2CameraTweaks.`
- Do not infer unknown field values from assumptions.

### B4 — Post-publish verification

- [ ] Published version and tag match the prepared version.
- [ ] Published archive matches the verified archive byte-for-byte.
- [ ] GitHub description and asset are correct and publicly accessible.
- [ ] Nexus description, changelog and main file are correct, visible and
  publicly accessible.
- [ ] Installation files and defaults are correct.
- [ ] Record published identifiers and archive SHA-256.

## GitHub Release field reference

Record values actually used and reuse them unless intentionally changed:

- Tag format: `v<major>.<minor>.<patch>`
- Release title: `v<version> — Dynamic Ultrawide Gameplay, Cinematics, Dialogue FOV & Custom Framing`
- Target branch: `main`
- Latest release: `yes`
- Prerelease: `no`
- Draft before publication: `no`
- Uploaded archive: `STALKER2CameraTweaks-UE5.5.4-v0.5.0.zip`
- Release body: `GITHUB_RELEASE_BODY.md`
- Naming notes: use feature-oriented title with version first; keep the full
  product name in the release body.

## Nexus Mods field reference

Record values actually used and reuse them unless intentionally changed:

- Version format: `<major>.<minor>.<patch>`
- File display name: `STALKER 2 Ultrawide and Camera Tweaks by Elhait`
- File category: `Main`
- Short description: `Unified gameplay, cinematic and dialogue FOV fixes for Steam 2.0.4 / UE 5.5.4. Adds Adaptive and Reduced dialogue zoom, custom cinematic framing, dynamic aspect correction and guarded signature resolution.`
- Long description: `NEXUS_DESCRIPTION.md`
- Changelog: four-line v0.5.0 entry covering dialogue modes, smooth EXIT
  recovery, F9/F10 next-lifecycle hotkeys and renamed identity.
- Replace/coexist with previous main file: update existing `0.4.0` and archive
  the previous file.
- Compatibility/game-version selections: preserve the existing mod-page
  selections; runtime claim is Steam 2.0.4.
- Persistent category/options: primary file enabled, mod-manager download
  enabled, requirements popup enabled.
- Uploaded archive: `STALKER2CameraTweaks-UE5.5.4-v0.5.0.zip`

After the first actual publication, replace these placeholders with the values
used. They become canonical defaults unless intentionally changed.

## Published record

```text
Published commit: <id>
Published tag: <tag>
GitHub release: <identifier/link>
Nexus release/file: <identifier/link>
Published archive SHA-256: <sha256>
```
