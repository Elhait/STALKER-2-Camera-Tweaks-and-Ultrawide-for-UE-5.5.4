# Cynet “Malicious (score: 100)” — targeted investigation

Date: 2026-10-17  
Subject: 3.2.1 ASI, SHA-256 `A4D3E60231BF151E4A929312E9000F9C1F220DDE972F9DD99CF6A8DD93E1A24E`

## Conclusion

The exact reason Cynet assigned this file `Malicious (score: 100)` is **not established**. The public VirusTotal/Cynet material located describes Cynet as using multiple detection layers, including AI-based static analysis and runtime/behavioral capabilities, but does not define this result’s numeric score or identify which layer produced this particular file verdict. Therefore, “100” must not be represented as a documented 100% probability or confidence value.

The screenshot supplied by the user establishes the current VirusTotal observation: **1/71**, with Cynet as the sole detection. It does not include a Cynet explanation, threat-family name, behavior report, or classifier feature attribution. The VirusTotal file page could not be independently read through the available browser interface; no additional details are inferred from it.

Targeted source and PE checks found expected ASI mechanisms, but no concrete malicious behavior or specific defect that explains Cynet’s classification. No AV evasion or binary masking change is recommended. This is a bounded technical result, not a declaration that the file is safe or that the alert is a false positive.

## Findings

| Finding | Evidence | Real security impact | Fix required? | Link to Cynet result |
|---|---|---|---|---|
| Score semantics | The user-provided VT screenshot says `Malicious (score: 100)`. Public Cynet/VT material describes multiple detection layers but does not define this numeric field for the observed verdict. | Unknown; the number alone is not interpretable as a probability. | No code change. Ask Cynet/vendor for verdict rationale or submit a false-positive review if the user chooses. | **Not established.** |
| Runtime code generation and patching | `external/safetyhook/safetyhook.cpp:229,241,1307,1341,1605-1649` allocates executable trampoline memory, changes page protection while patching the current process, and flushes instruction caches. | Powerful behavior, but expected for this in-process camera hook. Source targets the current process; this is not remote-process injection. | No defect found in this bounded review. Keep fail-closed protection/flush handling. | **Not established.** It is a plausible classifier feature, not evidence of causation. |
| VEH and module residency | `external/safetyhook/safetyhook.cpp:1283,1443-1455,1609-1649` installs a vectored exception handler for thread-safe patch routing. `src/plugin/runtime.cpp:6438` pins the module through `src/platform/win32/module_residency.hpp:9-15` so resident callbacks cannot outlive code. | Changes execution and module lifetime within the game process; avoids callback use-after-unload. | No simplification is equivalent without replacing the lifetime/patch guarantees. No defect established. | **Not established.** |
| Overlay/window hooks | `src/overlay/composition/composition_runtime.cpp:348,1126` creates the owned overlay window and installs a one-shot, thread-specific `WH_GETMESSAGE` handoff for WndProc binding. | Expected window management/input integration for the overlay. No remote thread or process launch is involved. | No concrete defect found in this scope. | **Not established.** |
| Configuration/log file access | `src/platform/win32/native_file.hpp:38`, `src/config/config_repository.cpp:42`, and `src/config/config_template.cpp:1271` show owned Win32 file access and atomic replacement paths. Existing security audit traced these to the module’s configuration/logging needs and bounded migration paths. | Writes are confined to the mod’s configuration/log locations under the reviewed path construction; no arbitrary script/executable loading was found. | No new issue found. | **Not established.** |
| PE composition | Static inspection of A4: 3,552,768 bytes; seven conventional sections; no statically W+X section, exports, delay imports, or appended overlay; same imported DLL names as F68; 30 resources including VERSIONINFO; two TLS callbacks consistent with MSVC startup. F68: 3,551,744 bytes, same section layout/import names, 29 resources and no VERSIONINFO. | No packer or embedded executable payload evidence observed. Runtime-generated executable memory still exists by design and must not be confused with a static W+X section. | No PE defect identified. | **Not established.** |
| Unexpected capabilities | The current imports and production source review found no network client, child-process launcher, persistence mechanism, or remote-process memory write path. | No such behavior was found in the reviewed production source/import surface. This does not prove arbitrary absence beyond that surface. | None identified. | No evidence connects these capabilities to the verdict. |

## What changed between F68 and A4

The previous binary inspection recorded F68 (`f68f40…9bac2a`) at 3,551,744 bytes and A4 (`a4d3e6…e1a24e`) at 3,552,768 bytes. Both have the same section names/permissions and imported DLL names; section entropy is effectively unchanged. A4 adds a VERSIONINFO resource. The source diff for the candidate also contains the SafetyHook cleanup of an unreachable destructor-state flag/branch; the instruction-cache and failure-state hardening was already present in F68.

These are small, attributable changes, but they do not isolate what changed VirusTotal’s other vendor verdicts. Cynet’s unchanged label across F68 and A4 only shows that its displayed classification remained the same for these two submitted subjects; it does not prove an independent or correlated detection mechanism.

## External evidence and limits

- Cynet’s VirusTotal announcement describes Cynet360 as multi-layered, mentioning AI-based static analysis as well as behavior/memory monitoring and sandboxing. It does **not** disclose the feature attribution for this file: [VirusTotal: Cynet joins VirusTotal](https://blog.virustotal.com/2020/06/virustotal-cynet.html).
- VirusTotal states that vendor engines supply their own verdicts and directs false-positive disputes to the relevant engine vendor: [VirusTotal false-positive guidance](https://docs.virustotal.com/docs/false-positive).
- VirusTotal cautions that its engine results/configuration are not a substitute for comparative AV testing or necessarily identical to consumer-product behavior: [VirusTotal antivirus statistics](https://docs.virustotal.com/docs/antivirus-stats).
- The exact VT page was not independently accessible through the available browser tool. No JSON/API verdict details, Cynet sandbox output, or vendor-side explanation were available.
- No ASI was executed or uploaded by this investigation, and no new game session was launched. The user-provided live-game PML was analyzed offline. No graphics fixture, WARP, full suite, or build was run.

## User-provided Process Monitor trace

The user later supplied `Instruments/ProcessMonitor/Logfile.PML` (37,698,943 bytes, SHA-256 `AC56F645736E9D78EABD66B6AF4B7D4D362FAC482D0FFA7E77D41401624D66B0`). It was exported locally to CSV with the signed Procmon 4.12 executable; export exited successfully in under five seconds. No new capture was started. The exported CSV was filtered in-memory to PID 16540 for analysis and the broad CSV export was removed afterward; the original PML was not modified.

Observed PID 16540 is `Stalker2-Win64-Shipping.exe`, started at 16:17:21. The saved trace covers 16:22:57.2995199 through 16:26:20.5455933 (about 3m23s), so it begins roughly 5m36s after game process startup and cannot establish what happened during ASI loading or initial plugin startup.

The PML contains 105,597 rows, of which 56,972 are for PID 16540 and 48,625 are for other PIDs. Thus the saved trace is not restricted to the game PID; analysis below was restricted to PID 16540. Its observed event types are predominantly `QueryOpen` (31,779), `ReadFile` (18,024), registry opens/queries, `CreateFile` (1,032), and `WriteFile` (529). No path or detail field in the target-PID rows contains `STALKER2CameraTweaks`. The target-PID writes in this interval go to GameAnalytics SQLite files, SloppyCam state, `ReShade.log`, and `ue4ss/UE4SS.log`; they are not attributable to this mod from this trace.

No process creation, image-load, network, or `RegSetValue` event appears in the exported target-PID rows. Because those event categories are absent from the captured operation set, their absence is **not proof** those actions did not occur outside the capture’s scope. Procmon also does not directly expose this mod’s `VirtualAlloc`/`VirtualProtect`, VEH registration, or internal hook installation. The trace therefore found no unexpected mod file/registry/process activity during its observed interval, but it does not explain Cynet’s verdict and does not exercise the decisive startup interval.

- The Procmon export process completed and exited normally; no live capture was active after the export.
- The trace itself contains other processes’ events. It remains at the user-provided path and should be treated as potentially sensitive system activity.

## Next discriminating step

This trace does not supply a new technical lead. A vendor-side explanation for the exact SHA-256 (classification source/rule or sandbox/static-analysis rationale) remains the only evidence likely to identify Cynet’s specific cause. If Cynet provides none, the root cause remains opaque from the public VT result. Further local source or section inspection cannot reliably infer a proprietary classifier’s feature attribution, and changing code solely to perturb the verdict would be unjustified.
