# Final v2 Architecture Closure Audit

Date: 2026-09-26  
Scope: read-only review of the final repository state after Batches 0–7. No
production source was changed and the game was not launched. Baselines:
[`FINAL_V2_ARCHITECTURE_REPAIR_PLAN.md`](../active/FINAL_V2_ARCHITECTURE_REPAIR_PLAN.md)
and
[`FINAL_V2_REPOSITORY_ARCHITECTURE_AUDIT_2026-09-25.md`](FINAL_V2_REPOSITORY_ARCHITECTURE_AUDIT_2026-09-25.md).

## 1. Final finding matrix

| Finding | Final status | Current authority / repair | Evidence and guardrails | Remaining limitation |
| --- | --- | --- | --- | --- |
| F-01 — RuntimeSettingsApi publication | **CLOSED WITH DOCUMENTED LIMITATION** | `RuntimeSettingsApi` publishes one immutable callback/user-data bundle once; an acquire/release readiness state gates all readers. Process-resident `RuntimeState` owns callback context. | `runtime_settings_api_harness` covers unpublished/partial/full states, competing publishers and concurrent readers across Apply/snapshot/semantic-snapshot/persist; startup publication is installed once in `runtime.cpp`. | The test is a deterministic API concurrency test, not an injected-game early-Present/input startup test. That runtime pressure case remains. |
| F-02 — Forced21x9 authority and projection | **CLOSED WITH DOCUMENTED LIMITATION** | `cinematics::Forced21x9Aspect` (3440/1440) is the shared semantic value consumed by production application and overlay assessment; the semantic snapshot remains observational. | `camera_integration_harness` covers a matching 3440×1440 case, a distinct 32:9 mismatch, Auto/Native and unavailable evidence; cinematic aspect and semantic snapshot harnesses pass. | Offline policy agreement does not prove the in-game status at those resolutions; validate both viewport cases in runtime. |
| F-03 — Main INI multi-writer race | **CLOSED WITH DOCUMENTED LIMITATION** | `config_repository` owns one serialized update authority used by settings writes, first-run creation, managed-template synchronization, placement/migration and the atomic X/Y pair. Staged replacement preserves the prior file on failure. | Config persistence and placement harnesses cover parse/write, template idempotence, migration, failure preservation and synchronized setting+placement writers. Fresh run: 24/24 pairs, zero lost setting/X/Y values; Batch 3 also recorded 10 repeated runs (240 pairs). | Same-process overlap is covered; simultaneous movement + settings save followed by actual game restart and legacy-file migration remain runtime checks. |
| F-04 — Binding/setting metadata spread | **CLOSED WITH DOCUMENTED LIMITATION** | `HotkeyBindingRegistry` owns the five stable binding identities, INI section/key and defaults. Runtime maps typed mutation kinds to those identities; the overlay presentation table supplies view-specific label/group/field metadata. Per-feature effects remain explicit in Runtime. | Config harness checks completeness, uniqueness, defaults, parser/persistence and conflict behavior. Input harness checks presentation coverage, field/default parity and rebinding identity. | Consolidation is deliberately bounded to hotkey binding metadata. Other setting/mode effect and transition switches remain explicit; no generic settings framework or exhaustive metadata conversion was attempted. |
| F-05 — Test audit brittle/incomplete | **CLOSED WITH DOCUMENTED LIMITATION** | `test_cmd_audit.ps1` compares declared source, compile and run sets with the repository harness inventory. `localization_catalog_audit.ps1` preserves behavioral catalog/resource/glyph checks and labels intentional source policies. | At the Batch 7 closure snapshot the inventory was 43/43/43. The latest 2026-09-26 audit-only refresh reports 45/45/45 registered source/compile/run entries with equal sets and complete repository inventory. Batch 4 deliberately verified detection of an unregistered harness and a changed embedded resource ID; behavior checks remain in registered harnesses. | Some renderer/resource wiring checks remain source-level by design because headless tests are not a reliable ImGui visual oracle. Runtime visuals and Arabic shaping/bidi are not established by glyph coverage. |
| F-06 — Conflicting support wording | **CLOSED WITH DOCUMENTED LIMITATION** | `docs/SUPPORTED_BUILD_MANIFEST.md` is the canonical per-build evidence matrix; `TESTING_AND_RESEARCH.md` links to it. Runtime, historical static resolver, narrow language-reader and current local artifact scopes are separate. | Reviewed matching-image v0.4.0 resolver task, 2.0.5 runtime record, 2.0.6 combined runtime record, the documented release-candidate hash and current binary hashes. Relative evidence links resolve. | The 2.0.6 runtime log's loaded-mod hash (`19F2…`) differs from release-candidate hash (`D04A…`) and the current locally built ASI (`1AFF…`). Equivalence is **UNKNOWN**; no runtime claim is transferred. Static 2.0.2–2.0.4 evidence is only for historical v0.4.0 resolver contracts, not the full current resolver set. |
| F-07 — Production overlay named as POC | **CLOSED WITH DOCUMENTED LIMITATION** | `build.cmd` defines `OVERLAY_PRODUCTION`; `discovery_runtime.cpp` accepts that shipped integration gate while retaining the legacy `OVERLAY_RENDERING_POC` gate for the retired standalone experiment. Build/profile meaning is documented. | Production, supported diagnostic and `build-overlay-settings.cmd` compatibility alias build successfully. Production and diagnostic profiles retain matching production source/resource inputs; diagnostic definitions/output remain distinct. | Retired `build-overlay-poc.cmd` and `build-overlay-discovery.cmd` are not supported ASI profiles and currently fail as standalone builds (POC omits current localization/frontend dependencies; discovery omits `diagnostic_runtime.cpp`). They were not repaired. Exact-artifact production overlay startup is not runtime-tested. |

## 2. Cross-batch integrity review

- **F-01 × F-04:** Hotkey identity consolidation still routes mutations through typed `RuntimeSettingsApi`; it does not bypass the immutable publication boundary or own runtime state. The callback bundle is immutable after the single publication, and callback context remains process-resident.
- **F-02 × semantic snapshot/UI:** Cinematic policy remains authoritative in `src/cinematics`; the overlay derives a read-only assessment from the shared forced-aspect constant and snapshot facts. No second `21/9` production value or camera write was introduced.
- **F-03 × F-04:** Registry-driven hotkey persistence ends at the existing config repository API. Settings, placement X/Y, migration marker, initial file creation and template updates all serialize at that config-owned boundary; the former plugin-owned persistence mutex is absent.
- **F-05 × F-04/F-07:** The runner inventories every repository harness and compares source/compile/run sets. The localization audit's remaining source policies have stated structural purpose; the build-gate rename did not remove test coverage or make a policy depend on the old POC spelling.
- **F-06 × F-07:** The manifest keeps runtime log hashes, current locally built outputs and release candidate/asset identities distinct. The production-semantic flag changes source selection only; it does not authorize carrying old runtime evidence to a new ASI hash.
- **F-07 profile separation:** `build.cmd` and `build-diagnostic.cmd` compile the same production feature/overlay/resource source set, with diagnostic instrumentation definitions and a distinct output for the latter. `build-overlay-settings.cmd` remains a production alias. Retired research scripts are clearly outside the supported release matrix.

The inspected authorities remain singular for the repaired concerns: Runtime owns runtime settings/effects and process lifetime; the cinematic domain owns aspect semantics; config repository owns main-INI updates; the hotkey descriptor registry owns stable binding metadata; the supported-build manifest owns the evidence summary. No cross-batch duplicate authority or concrete regression was found.

The guarded resolver/fail-closed contracts, configured/desired/active/observed distinctions, gameplay/cinematic/dialogue lifecycle and restoration paths, separate discovery/renderer lifecycles, INI migration compatibility, and localization resource/profile contracts remain represented in current source, project contracts and corresponding harnesses. This is a source/test closure assessment, not a new native-runtime proof.

## 3. Final automated validation

Historical full `test.cmd` run recorded at the Batch 7 closure snapshot on 2026-09-26:

- Harness inventory at that run: **43 source / 43 compiled / 43 executed**, sets equal and repository inventory complete.
- Harness result: **PASS**. No architecture-repair XFAIL was emitted (**0 XFAIL**). The concurrent placement exercise passed 24/24 pairs with zero lost setting, X or Y updates.
- Localization/resource audit at that run: **PASS** — 18 catalogs, 162 keys, locale parity, placeholders, UTF-8, embedded resource IDs/files, glyph/profile coverage and selector fonts.
- Scope note: audit output explicitly says Arabic shaping/bidi/RTL and runtime visual rendering are not established by offline glyph coverage.

Build evidence from the completed Batch 7 validation (production source has not changed since; subsequent edits were documentation-only):

- `build.cmd`: **PASS**, production ASI emitted with `OVERLAY_PRODUCTION`.
- `build-diagnostic.cmd`: **PASS** with default diagnostic definitions and again with all optional feature diagnostic definitions enabled together.
- `build-overlay-settings.cmd`: **PASS**, compatibility production build alias.
- Project-owned production warnings: **none observed**. Vendor warnings remain Zydis `C4201` and ImGui DX12 `C4189`. The all-options diagnostic compile also reports `C4456` shadow warnings in diagnostic-only runtime blocks.
- Retired standalone research scripts were probed and failed as documented in F-07; they are not counted as supported build profiles and were not repaired.

## 4. Consolidated final runtime regression checklist

No game was launched for this audit. The following scenarios remain to establish the final artifact's integrated behavior. Earlier runtime records belong to other mod hashes and cannot be reused as proof for the current binary.

1. **F-01 startup/readiness:** On the exact production ASI, exercise normal startup and early Present/input pressure while RuntimeSettingsApi is being published; confirm complete settings access after publication, fail-closed behavior before publication, and that failed/unsupported overlay discovery stays isolated from camera features.
2. **F-02 projection:** With `Forced21x9`, verify status is aligned at 3440×1440; switch to a clearly different 32:9 viewport and verify a real mismatch. Include Auto/Native and unavailable viewport paths if observed in the same run.
3. **F-03 persistence:** Move the overlay and save a setting/mode as close together as practical, restart, and verify both placement coordinates and the setting. On a disposable config, verify one-time import from the legacy overlay INI, marker persistence, idempotent second start and preservation of the old file.
4. **F-04 binding/settings:** Rebind one representative key and verify displayed binding, persistence, dispatch and notification agree. Change one representative setting/mode and verify its expected immediate-versus-next-lifecycle effect and saved/read-back value.
5. **F-07 overlay/Auto localization:** Start the exact production artifact, verify renderer/resource readiness and overlay toggle/startup hint; in Auto, exercise closed→open reads across a controlled `uk → en → uk` change and confirm catalog/font/recommended-size synchronization. This exact artifact has no runtime evidence.
6. **Core release regression on the same artifact:** Check gameplay startup/enable-disable restoration and camera recreation after save/load/death; ADS/binocular specificity; cinematic Auto and forced 16:9 / 3440×1440 / 32:9 framing, GameplayHorPlus and NativeHorPlus FOV selection, and F10/F11 applying at the next lifecycle; dialogue Native/Adaptive/Reduced/Disabled at a representative high-FOV baseline, sequential dialogs, cinematic isolation and recovery. Include the established gameplay/cinematic/dialogue coexistence scenario and confirm native post-cinematic recovery remains the game's transition (not a mod rewrite).

This is a consolidated evidence checklist, not permission to claim unsupported executable versions or to treat glyph audits as runtime visual proof.

## 5. Binary identity and evidence status

| Evidence subject | SHA-256 | Status |
| --- | --- | --- |
| Current local production output `STALKER2CameraTweaks.asi` | `1AFF9A14D5A55B9E08FE1D3E3418ABDC6CA5C39B046B5DD787620850CC7A843D` | Built with the final Batch 7 production gate; **not runtime-tested**. |
| Current local all-options diagnostic output `STALKER2CameraTweaksDiagnostic.asi` | `0E9E6CE57733B987AC8DACC0FF0F8CA5FC1E1BBD34A7B4F47955B8DBEC0EBA11` | Diagnostic build only; not production and not runtime-tested here. |
| `release-assets/STALKER2CameraTweaks.asi` | `3C6F95F75D36B04780740D82783D586E821C1B8AFFABCE4463F12CDEBFCDE4E8` | Existing release-assets file; no reviewed runtime log identifies it. |
| Documented v1.0.0 release-candidate ASI | `D04A43E28DB5DFFD10D88B6F30BEF8FEC2A560E1CD9949FEA31FAF485DA0E7BC` | Candidate identity in documentation; not equated to the recorded combined runtime binary. |
| Steam 2.0.6 combined runtime log | Game `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`; mod `19F2F31C20BB5D47CD12D2D3D773774985A5D771E6F7F8730A6363983161DA72` | Existing scenario evidence only for the logged game/mod pair. |
| Steam 2.0.5 runtime record | Game `E7B481A97C02D80581FAB0BECE940214A88EBE30211088A00129845A039F9293`; mod `69021D8758069F7EFE098B3C562A41E326A6DB0BF4BA88B789FB38854217DFB2` | Earlier tested scenarios; not evidence for current local output. |

The 2.0.6 Auto language-reader observation is narrower than a complete game compatibility claim. Static resolver evidence is historical v0.4.0 contract coverage on Steam 2.0.2–2.0.4 only. Unknown identities remain unknown; hashes are evidence identifiers, not a general allowlist.

### Subsequent owner runtime confirmation and audit refresh — 2026-09-26

After this closure snapshot, the project owner confirmed that the current
overlay/mod works in-game and that nearly all behavior has been checked. The
exact hash and complete scenario matrix for that later review were not supplied
in this report, so this confirmation is recorded as owner-reported runtime
validation and is not retroactively assigned to the older hashes in the table
above.

The current audit scripts were also rerun in audit-only mode: runner
registration/compile/run-set audit **45/45/45**, sets equal and repository
inventory complete; localization/resource/glyph audit **PASS**, 18 catalogs,
182 keys. These are refreshed inventory/catalog checks, not a fresh execution
of every harness or a substitute for runtime evidence. The earlier 43/43 and
162-key values above describe the Batch 7 run and remain historical results.

## 6. New architecture findings

**None found** in the bounded F-01–F-07 closure review. The retired POC/discovery scripts' failures are known, documented and outside the supported build profile; they are not newly promoted production defects. No speculative refactoring or performance issue is reported.

## 7. Architecture closure verdict

# ARCHITECTURE CLOSED WITH DOCUMENTED LIMITATIONS

F-01–F-07's accepted source-level defects are repaired and the final offline harness, localization audit and supported build-profile evidence are coherent. Cross-batch review found no replacement race, duplicate owner or regression in the repaired contracts. Closure is limited by the explicitly UNKNOWN binary identity between the 2.0.6 runtime log and release-candidate/current artifacts, and by the consolidated final in-game regression checklist above. This verdict does not certify the current local ASI for release or claim runtime compatibility beyond its identified evidence.
