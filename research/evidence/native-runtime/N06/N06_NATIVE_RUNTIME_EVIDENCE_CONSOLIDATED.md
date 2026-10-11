# N06 — Native Runtime Evidence (Consolidated)

**Evidence date:** 2026-10-09–2026-10-10  
**Consolidated:** 2026-10-11  
**Status:** Parent N06 remains **Partially verified / Open**. N06-M01 is **Complete — Verified within offline scope**.  
**Scope:** Native hook provenance, offline adapter/native replay evidence, build identity and remaining live-runtime limits. This report replaces the five separate N06 reports and their bulky supporting capture directory. The test tools and fixture sources remain available; regenerated command logs/binaries are directed to ignored `build-artifacts/`.

## 1. Decision summary

The current identified game image’s seven production camera hook sites were checked against the current resolvers, signatures, exact displaced instructions and continuation structure. No signature or instruction mismatch was found. The N06-M01 Zoom IN/OUT fixtures now replay the actual game-derived load/compare/branch windows through the real bundled SafetyHook mid-hooks and production callbacks, covering both branch outcomes.

That is strong offline provenance and callback evidence, but it is not an end-to-end live proof. The fixture’s code beyond each branch destination is a bounded marker, not the engine continuation; multiple other native consumers and object-lifetime relationships are not executed; and the previous captured installed-game logs identify an ASI different from the retained E349 artifact. Accordingly, the parent gate remains open. This consolidation does not change any N01–N06 or B1–B7 disposition.

| Gate / finding | Current disposition | What the evidence means |
|---|---|---|
| N01 — SafetyHook patch state / executable lifetime | **Fixed**, within offline fault-injection and documented retention bounds | Local vendor failure paths were exercised; this does not prove concurrent game callback quiescence. |
| N02 — Zoom exception containment | **Fixed**, within actual-adapter and local native-hook bounds | Both Zoom callbacks use the safe entry; M01 adds current-image instruction replay. Live game behavior remains distinct. |
| N03 — startup exception containment | **Partially mitigated** | The real thread-entry boundary and controlled cleanup cases were tested; full game-specific initializer/installer failure was not run. |
| N04 / N05 | **Open** | Preserve the outstanding gates as recorded in the remediation/evidence reports; this consolidation closes neither. |
| N06-M01 — game-derived Zoom replay fixtures | **Complete — Verified within offline scope** | Exact current-image Zoom windows, both branch outcomes, production callback and local SafetyHook trampoline were tested. |
| N06 — native replay provenance / identity | **Partially verified / Open** | Static site provenance and offline replay improved; final artifact/live consumer/source-identity links remain incomplete. |
| Final remediation ASI E349…DC81 live validation | **Pending in the retained audit evidence** | The old captured game logs identify A712…C7771, not E349…DC81. Later user-performed gameplay testing is not reclassified here without a captured exact ASI hash/log identity. |

## 2. Identity and reproducibility facts

### Game image

| Fact | Recorded value |
|---|---|
| Image | `Stalker2-Win64-Shipping.exe` |
| SHA-256 | `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B` |
| Length | 174,798,384 bytes |
| Steam App ID / build ID | `1643320` / `25382007` |
| PE | AMD64 PE32+, preferred base `0x140000000`, `SizeOfImage=0xB963000` |
| `.text` | RVA `0x1000`, raw offset `0x400`, raw size `0x7CCC200` |
| Engine literal | UTF-16 `Unreal Engine 5.5.4` at file offset `0x9976648` |

The probe mapped sections locally to evaluate existing resolver predicates and disabled-hook construction. It did not invoke a game DLL loader, entry point, TLS callback or game function.

### Source/build identity

The captured repository was dirty; HEAD alone is not a complete source identity. The recorded HEAD was `db67e4dfc56cfe06394c38413f5bb460f797d210` on `main`. A protected-file manifest covered 368 files and had SHA-256 `1C96A70D7533E03195E4E7F5053820CAC8567FF42EE4515C6A24B37EE53F9FE1`; before/after manifests matched. The original working-tree diff capture failed, so the staged patch was not a complete source diff. Do not describe this as a clean or bit-reproducible source baseline.

Recorded relevant input hashes:

| Input | SHA-256 |
|---|---|
| `src/plugin/runtime.cpp` | `BA575BFDCCFA995428364AD2957B840E1B67264C4334552FAE8E7A26E12F2612` |
| `src/hooks/signatures/signature_definitions.hpp` | `AAFB4689239988CD3E9F8D8422E8576FEC13103E60FDA16398E0BE1F1C0DF255` |
| `external/safetyhook/safetyhook.cpp` | `71C7F431163E4AA6AE70A2F1AC706E5FF3F98C2A1215A91F7F00D78AA42E889D` |
| `external/safetyhook/safetyhook.hpp` | `690923285915B2663CBAD225E288B41543F102875CCE4C02253127EF6749DE35` |
| `external/safetyhook/Zydis.c` | `AD1D0ECB3935432D1F42504955D5B8740C0E7E091A3FB52BC751ADDC0D39E36C` |
| `external/safetyhook/Zydis.h` | `ED35A8138132254E7877C86D3533E86860E53CDD955189C433FF1BE80B7FD534` |
| `build.cmd` | `DBA8D761E78BA5FD3D5D5CD22E33593D788D080B324CBBDC691C023C471CA843` |
| `test.cmd` | `B96C86FA2D853FA0BB5C38C6D7946C832C5C59D9621DFB64E2671589F78C87FB` |

Toolchain: Visual Studio 17.14.28; MSVC `19.44.35224.0`; linker `14.44.35224.0`; bundled Zydis reports `4.0.0`. SafetyHook vendor identity is recorded by file hashes above; no upstream release/commit is inferred.

### ASI and test artifact identities

| Artifact | SHA-256 | Interpretation |
|---|---|---|
| Captured installed `STALKER2CameraTweaks.asi` | `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771` | Matches the captured game log; not the retained E349 artifact. |
| Retained final remediation production ASI | `E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81` | Exact bytes were retained; no immutable full source-input manifest from its original build. |
| Retained final diagnostic ASI | `CD73C138AB35618182F90F8E2F7F56B51602D42773041A99CD23E0434A9CA75B` | Offline artifact identity only. |
| Ordinary actual-adapter harness | `B82F89AE2DB7F6BD7DA945E5CAB073827B842423C1E71E73E2BC0E700921AD32` | 101 scenarios × Diagnostic OFF/ON = 202 child processes PASS. |
| Diagnostic actual-adapter harness | `F62B167F41BF40DBAC2DC19B732D099EB04082E01E5CC133E010C82822987B50` | 101 scenarios × Diagnostic OFF/ON = 202 child processes PASS. |
| N06 image/resolver probe | `D08596349B029E96DCB41968BE511D3F6F8720F4CC158BFC8A1B63FF328CD8F1` | Offline probe; no hooks enabled and no game code executed. |
| N06-M01 game-derived Zoom fixture executable | `208DAF30CBD543E7C4ACDE559EA043159047E0E4D7245021ACF4654DA8B7FCB3` | Four new fixture processes PASS; binary can be regenerated by the retained runner. |

The E349 and A712 files have the same recorded length (3,465,728 bytes); they are not the same binary. The user later reported building and running the latest code in-game, but the available evidence package did not capture that run’s ASI SHA/log identity. This report therefore preserves the existing exact-artifact gate status rather than inferring identity.

## 3. Active production hook traceability

All seven active production camera sites are mid-hooks owned through the single runtime `HookSet`. Ordinary and supported diagnostic builds use the same seven-site inventory. Legacy Cinematic EXIT is an alternate resolver topology, not an eighth current hook.

| Hook | Site RVA / file offset | Displaced bytes / native operation | Callback and continuation contract | Evidence boundary |
|---|---|---|---|---|
| Gameplay writer | `0xA9F8CB` / `0xA9ECCB` (match starts `0xA9F8B2`) | `F3 0F 11 43 30` — `MOVSS [RBX+0x30],XMM0` | `ReplayManualTransition`; RSI source, RBX destination, low XMM0. Publication precedes displaced store; trampoline stores then continues at `0xA9F8D0`. | Actual callback cases and exact resolver/store validation; callback→native store→next engine consumer not executed together. |
| Cinematic aspect store | `0x6B75865` / `0x6B74C65` | `C7 80 54 02 00 00 39 8E E3 3F` — 10-byte literal store | `ApplyCinematicAspectStore`; callback advances RIP by 10 and attempts the bounded store itself, skipping the original literal store; continuation `0x6B7586F`. | Controlled store and RIP delta plus structural continuation validation; native skip/consumer path not executed. |
| Cinematic ENTER | `0x366DAFF` / `0x366CEFF` | `E8 C4 8C 4F 03` — CALL shared consumer `0x6B667C8`; hook is after RIP-relative FOV load. | `TraceCinematicEnter`; may replace low XMM0; relocated CALL executes, fallthrough `0x366DB04`. | Actual adapter lifecycle cases and exact resolver/CALL target; original load and consumer not executed. |
| Cinematic indexed EXIT | `0x366DA9D` / `0x366CE9D` | `E8 26 8D 4F 03` — CALL same consumer `0x6B667C8`; preceding MOVZX/indexed MOVSS load occurs before callback. | `TraceCinematicExit`; observes loaded low XMM0; relocated CALL then fallthrough `0x366DAA2`. | Actual adapter EXIT/recovery cases and structural CALL relocation; indexed native load/consumer not executed. |
| Dialogue boundary | `0xD09629` / `0xD08A29` | `FF 90 08 06 00 00` — `CALL [RAX+0x608]`; preceding loads include `MOVAPS XMM1,XMM6`. | `TraceDialogueBoundary`; reads low XMM6, may change low XMM1; relocated indirect CALL, fallthrough `0xD0962F`. | Actual controlled-context cases and byte/continuation validation; real vtable consumer/object lifetime not executed. |
| Zoom IN | `0x602A79` / `0x601E79`; match `0x602A6C` | `0F 2E C8 0F 87 C7 01 00 00` — UCOMISS + near JA to `0x602C49`; preceding loads `[RAX+0x4C]`, `[RSI+0x138]`. | `TraceZoomIn` through `SafeMidHookEntry`; both target `0x602C49` and fallthrough `0x602A82` exercised by M01. | Exact 22-byte game window, both branch outcomes, actual callback and real local SafetyHook replay; downstream engine code at destinations is synthetic. |
| Zoom OUT | `0x602C1F` / `0x60201F`; match `0x602C12` | `0F 2E C8 77 15` — UCOMISS + short JA to `0x602C39`; preceding loads `[RAX+0x50]`, `[RSI+0x13C]`. | `TraceZoomOut` through `SafeMidHookEntry`; both target `0x602C39` and fallthrough `0x602C24` exercised by M01. | Exact 18-byte game window, both branch outcomes, actual callback and real local SafetyHook replay; downstream engine code at destinations is synthetic. |

Resolver facts: Gameplay writer’s unique match and MOVSS destination were validated; Cinematic aspect unique match and operand/length were validated; ENTER and EXIT share the same decoded consumer; current indexed EXIT matches once while legacy EXIT matches zero; Dialogue unique match/literal predicate was independently checked; Zoom installation is gated by the exact game hash and unique match. No current-image legacy EXIT test exists; it is not applicable, not a PASS.

Captured A712 logs record the seven same RVAs, partially corroborating installation for that artifact. They do not include installed patch/stub/trampoline bytes and cannot establish E349 live execution.

## 4. What the tests actually establish

### N06 main validation

| Check | Result recorded | Limit |
|---|---|---|
| Offline probe compile, MSVC 19.44.35224.0 | Exit 0 | This is not an ASI build. |
| Actual resolver and structural relocation probe | Exit 0; seven sites accepted; indexed EXIT selected, legacy zero | Hooks remained disabled; no game instruction executed. |
| Ordinary actual-adapter matrix | 101 scenarios × Diagnostic OFF/ON = 202 child processes PASS | Controlled memory/viewport/Overlay/log leaves; does not execute every displaced native instruction. |
| Diagnostic actual-adapter matrix | 101 scenarios × Diagnostic OFF/ON = 202 child processes PASS | Same boundary under diagnostic compilation. |
| Four supplementary harnesses | Exit 0 | Historical replay inputs have UNKNOWN identity; other harnesses are synthetic/helper evidence. |
| Protected-file preservation | 368 files unchanged | The staged diff was preserved, but the initial full working-tree diff capture failed. |

The 404 adapter processes are two compiled profiles × 101 scenarios × two runtime Diagnostic modes, not 404 unique scenarios. Full `test.cmd`, production/diagnostic builds and locale audit PASS results are from the separately documented N01–N03 remediation run, not newly executed as part of N06.

Two initial parent-harness attempts stopped at `settings-api` with “Production settings persistence handler failed on valid file.” The first recorder also lost an exit code and had a concurrent journal append failure. No PASS is attributed to those attempts. Sequential runs with separate workspace-owned TEMP/TMP directories passed. The exact initial OS operation was not measured, so the difference is not assigned to a confirmed mod defect.

### N06-M01 Zoom fixtures

Game-derived windows, checked before replay:

| Fixture | Match RVA / callback hook | File offset | Exact window | Branch target / fallthrough |
|---|---|---:|---|---|
| IN | `0x602A6C` / `0x602A79` | `0x601E6C` | `F3 0F 10 40 4C F3 0F 10 8E 38 01 00 00 0F 2E C8 0F 87 C7 01 00 00` | `0x602C49` / `0x602A82` |
| OUT | `0x602C12` / `0x602C1F` | `0x602012` | `F3 0F 10 40 50 F3 0F 10 8E 3C 01 00 00 0F 2E C8 77 15` | `0x602C39` / `0x602C24` |

The runner hashes the game image and compares all instruction bytes before compiling. Each fixture executes the original loads/compare/branch through actual production `SafeMidHookEntry` callbacks and the bundled SafetyHook mid-hook. Both branch outcomes run. Register/XMM preservation, flags, stack depth, canaries and restoration of original bytes are checked. The engine continuation after each destination is replaced by a bounded synthetic marker at the exact destination offset.

| M01 test set | Result |
|---|---|
| IN/OUT × Diagnostic OFF/ON | 4 isolated processes PASS; each exercises both native branch outcomes. |
| Existing Zoom cases: `zoom-in-throw`, `zoom-out-throw`, prior synthetic `evidence-zoom-native-continuation`, `zoom`; ordinary/diagnostic binaries × runtime Diagnostic OFF/ON | 16 isolated processes PASS. |
| Preservation of production/vendor/release and prior fixture | PASS; prior `runtime_native_evidence_cases.hpp` hash `2711038F8EB65294EB4EEC5D67551D85470F3CC8D7CD5FF9FE7448C4FDE7D4C2`. |

The M01 sources retained outside this report are `tests/plugin/n06_m01_zoom_game_derived_harness.cpp` and `tests/plugin/n06_m01_zoom_game_derived_fixtures.hpp`. Their recorded hashes are respectively `7468D01B6AD05EA67A5B21928BCD1155C4A5440DA0DFB2EACB72C2090CB43E3D` and `0AC96DA0FE9ADF0679A7EE9B8DDBCC81B92A918D60D2C945183F65A8C303314F`.

## 5. Remaining evidence gaps

| ID | Open requirement | Evidence still needed / boundary |
|---|---|---|
| N06-G01 | Historical replay inputs have UNKNOWN game/mod/build fields; their original per-event corpus is absent. | Do not relabel old fixtures. New evidence needs its own captured exact identities. |
| N06-G02 | The captured installed/logged ASI is A712…C7771; exact live execution of E349…DC81 is not present in this archive. | Record exact canonical `STALKER2CameraTweaks.asi` SHA at test time plus matching game/log identity and exercise it in-game. |
| N06-G03 | No complete immutable source-input manifest exists for the original E349 build. | Future builds should preserve source/toolchain/input manifests; do not claim E349 is reproducible from HEAD alone. |
| N06-G04 | Live object lifetime, source association across all camera churn and actual engine consumers are not fully established offline. | Matched live traces or a specifically designed safe native test for each consumer boundary. |
| N06-T01 Gameplay | Callback→native `[RBX+0x30]` store→next engine consumer was not executed as one chain. | Current evidence validates callback and exact store structure separately. |
| N06-T02 Cinematics | Actual game RIP/indexed loads, shared consumer and whole native lifecycle are not executed. | Resolver/bytes and adapter cases are separate evidence. Legacy EXIT is absent in this image. |
| N06-T03 Cinematic aspect | Custom store/RIP behavior is controlled and structurally checked, not executed on a live camera object. | Live native store/consumer confirmation. |
| N06-T04 Dialogue | XMM6→low-XMM1 adapter behavior is covered; `[RAX+0x608]` vtable consumer and live source lifetime are not. | Live/native consumer evidence. |
| N06-T05 / N01 | SafetyHook local fault/lifetime handling is tested; arbitrary concurrent game callback patch/removal and native object quiescence are not. | Runtime validation; dynamic unload is unsupported. |
| N03 | Full game-specific Initialize/install/reset failure path is not run. | Keep **Partially mitigated** until a safe, actual-runtime failure validation exists. |
| N04/N05 | Existing separate verification gates remain unresolved. | Keep **Open**; this report makes no new claims for them. |

## 6. Reproduction tools and retained source

Retained, rerunnable tool sources:

- `research/evidence/native-runtime/N06/tools/n06_validate.ps1` — capture/build/probe/adapter/supplementary/final workflow.
- `research/evidence/native-runtime/N06/tools/n06_m01_validate.ps1` — verifies game hash and exact Zoom bytes, builds and runs M01 fixtures.
- `research/evidence/native-runtime/N06/tools/n06_game_image_probe.cpp` — offline mapped-image resolver/relocation probe.
- Fixture and actual-adapter sources remain under `tests/plugin/` and the registered test/build paths.

Run from the repository root. These commands regenerate detailed outputs under ignored `build-artifacts/native-runtime/`, not under this report directory:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File research/evidence/native-runtime/N06/tools/n06_validate.ps1 -Action Capture
powershell.exe -NoProfile -ExecutionPolicy Bypass -File research/evidence/native-runtime/N06/tools/n06_validate.ps1 -Action BuildProbe
powershell.exe -NoProfile -ExecutionPolicy Bypass -File research/evidence/native-runtime/N06/tools/n06_validate.ps1 -Action Probe
powershell.exe -NoProfile -ExecutionPolicy Bypass -File research/evidence/native-runtime/N06/tools/n06_validate.ps1 -Action Adapters -Profile ordinary
powershell.exe -NoProfile -ExecutionPolicy Bypass -File research/evidence/native-runtime/N06/tools/n06_validate.ps1 -Action Adapters -Profile diagnostic
powershell.exe -NoProfile -ExecutionPolicy Bypass -File research/evidence/native-runtime/N06/tools/n06_validate.ps1 -Action Supplementary
powershell.exe -NoProfile -ExecutionPolicy Bypass -File research/evidence/native-runtime/N06/tools/n06_validate.ps1 -Action Final
powershell.exe -NoProfile -ExecutionPolicy Bypass -File research/evidence/native-runtime/N06/tools/n06_m01_validate.ps1
```

`-ReuseProbeDependencies` / `-ReuseDependencies` may be used only with dependency objects from the corresponding same-run build. When validating a different revision or game image, use a new ignored output folder and capture new identities. The tools require the identified game executable to be available; a changed hash fails closed.

## 7. Consolidation and retention decision

This report now carries forward the findings, relevant identities, hook-site mapping, validation counts, limitations and reproduction commands from the former separate build-identity, hook-traceability, M01, provenance and validation reports. The former `supporting/` tree contained 115 generated captures (including 3 executables, raw stdout/stderr logs, command records, duplicate hash/protection manifests and captured installed logs). Those are intentionally not retained in Git after this consolidation; they can be regenerated by the tools, subject to current game/build inputs. This reduces the archive by about 7.3 MB while preserving the reviewable conclusions and reusable test sources.

Consequently, raw logs, exact per-run timestamps, full 368-row preservation CSVs, absolute artifact path/timestamp inventory, staged patch capture, and executable bytes are no longer available from this report directory. The evidence conclusions above are the retained record, not a claim that the deleted raw captures remain independently inspectable. For a future verification, rerun the tools and inspect outputs under ignored `build-artifacts/` before deciding whether selected evidence needs archival.

No production/vendor/runtime code, camera semantics, runtime owner, configuration, N03–N06 gate, B1–B7 decision or release artifact was changed by this consolidation. No build, test, game or ASI was run/installed for it.
