# Microsoft Wacatac — same-engine comparison, 2026-10-17

## Result

The installed Microsoft Defender scanned all three unchanged ASIs sequentially
and reported `found no threats` for each. The engine and signature versions
remained identical before and after every scan. The Microsoft detection supplied
in the VirusTotal screenshots was not reproduced in this local environment.
This does not adjudicate either Microsoft or Cynet as a false positive.

| ASI | User-supplied VirusTotal Microsoft result | Local custom scan |
| --- | --- | --- |
| `5124A197A036BFF5C630CA0E48250C3520745D508CC7E66465071ED75BA8E654` | No Microsoft alert shown in the earlier scan | Found no threats; exit 0 |
| `CC40B287F5C72A5AD7DE154241C0630094F0D49CA7523564F637327EC23F67CB` | Trojan:Win32/Wacatac.B!ml | Found no threats; exit 0 |
| `492A5884528ED8BA7C6401D15FF178B9EF2AFA77DD1903F8A687A342D7591298` | Trojan:Win32/Wacatac.B!ml | Found no threats; exit 0 |

Normalized captured evidence, complete scanner output and hashes:
[defender-same-engine-comparison.json](../evidence/security-hardening-2026-10-17/defender-same-engine-comparison.json).

## Environment and procedure

- Platform: `4.18.26090.9`; engine: `1.1.26090.9`; signatures: `1.461.74.0`.
- Scanner: `C:/ProgramData/Microsoft/Windows Defender/Platform/4.18.26090.9-0/MpCmdRun.exe`.
- Command arguments for each exact file:
  `-Scan -ScanType 3 -File <absolute ASI path> -DisableRemediation -ReturnHR`.
- Defender reported `SxS Passive Mode`, with real-time protection false. No
  existing setting was changed. The bounded query of detection/remediation
  events 1116/1117 for the previous seven days returned no matching events;
  that event-query result is not substituted for a scan result.
- Existing `SubmitSamplesConsent=2` (NeverSend) was verified before scanning;
  existing `MAPSReporting=2` remained enabled. No sample was manually submitted.
  The experiment does not establish that no metadata/cloud lookup occurred.
- Each scan had an external 60-second process-tree timeout and a
  20,000-character output budget. All completed normally without output
  truncation; process-tree cleanup was verified. No MpCmdRun process remained.
  The shared Defender service was not stopped.
- Hashes were checked before and after scanning; every ASI remained unchanged.
  No game, debugger, graphics fixture, source modification, build or ASI
  installation was performed for this investigation.

Microsoft documents that DisableRemediation custom scans ignore file exclusions,
apply no action after detection, and expose detections in console output rather
than the event log/UI. Thus an empty event log is not the acceptance criterion;
the three completed scanner outputs are the evidence here.

## Recheck after user-reported Defender update

At the user's request, all three exact ASIs were scanned again at
2026-10-17 18:28:50–51 UTC (21:28:50–51 Europe/Kyiv), using the same bounded
DisableRemediation procedure. All three invocations returned exit 0 and
`found no threats`; hashes remained unchanged and scanner cleanup was verified.

The reported active platform (`4.18.26090.9`), engine (`1.1.26090.9`) and
signatures (`1.461.74.0`) did not change before or after these scans. The
signature-update timestamp also remained 2026-10-17 11:07:43 UTC (14:07:43
Europe/Kyiv). Therefore the user-reported update was not reflected in the
active versions exposed during this recheck. It must not be presented as a
successful test against newer signatures. Scanner cache use was not determined.
Evidence: [defender-update-recheck.json](../evidence/security-hardening-2026-10-17/defender-update-recheck.json).

## Proposed R1/R2/R3 subtraction builds

The proposed combinations were evaluated against the current source and the
user's requirement not to reintroduce unsafe behavior for an AV comparison.

| Combination | Effect of simply reverting the omitted package | Disposition |
| --- | --- | --- |
| R1 + R2, no R3 | Reverts private RW construction / RX publication and brings back permanent RWX code allocations | Do not build under the current safety constraint |
| R1 + R3, no R2 | Reverts execute-only/context/ownership admission and reintroduces broad read/write AV handling | Do not build under the current safety constraint |
| R2 + R3, no R1 | Removes checked page restoration and fail-safe recovery; current R2 phase/history depends on transaction metadata maintained by that path | Not an independent switch; a safe alternative requires a new recovery design |

Relevant current code: `external/safetyhook/safetyhook.cpp` allocator/publication
at lines 131–170, 498 and 965; VEH admission at 1522–1613; protection recovery
at 1773–1797 and transaction state updates at 1880–1916. R1 and R2 share page
identity, protection transitions, transaction phase and delayed-fault metadata.
WC01 depends on preserving those semantics. No subtraction binaries were made.

The same-engine comparison above provides a controlled observation using the
already preserved artifacts, without making weaker intermediate hook engines.

## Interpretation and next decision

Confirmed: the Microsoft result differs between the user's VirusTotal scan and
this local Defender scan. The local engine did not distinguish the pre-R1–R3
artifact from either hardened artifact as a threat. Therefore a claim that R1–R3
necessarily causes local Defender to block the ASI is not supported by this run.

Unknown: the VirusTotal engine/signature/cloud-model versions and settings,
reputation/cache inputs, and which specific feature caused its verdict.
The local test is not equivalent to VirusTotal's environment or every user's
active Defender configuration. Cynet was not scanned locally and its observed
verdict remains a separate unresolved classification.

Priority: preserve this evidence and the exact ASIs; obtain the Microsoft
engine/signature details from the affected VirusTotal analysis if available and
request sample-specific Microsoft review of the unchanged hardened ASI when
authorized. Prepare separate Cynet evidence rather than infer a shared cause.
No protection-removal build or new SHA is justified by the results obtained here.
Manual game qualification remains outstanding and can stay deferred while the
user prioritizes resolving AV distribution concerns.

## References

- [Microsoft MpCmdRun custom scan and DisableRemediation options](https://learn.microsoft.com/en-us/defender-endpoint/command-line-arguments-microsoft-defender-antivirus).
- [Microsoft SubmitSamplesConsent values](https://github.com/MicrosoftDocs/windows-powershell-docs/blob/main/docset/winserver2025-ps/Defender/Set-MpPreference.md).
- [Microsoft Defender compatibility and passive modes](https://learn.microsoft.com/en-us/defender-endpoint/microsoft-defender-antivirus-compatibility).
