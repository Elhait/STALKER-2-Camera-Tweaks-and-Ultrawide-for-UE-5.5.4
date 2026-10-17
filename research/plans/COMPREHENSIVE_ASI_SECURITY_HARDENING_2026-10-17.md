# Comprehensive ASI security audit and hardening

User-authorized scope: inspect all production translation units, reachable
project/vendor headers, resource inputs, build and packaging paths; fix
source-proven security/safety problems; run bounded CPU-only / NonGraphics
regression and an isolated Release build. Do not install, upload samples,
run the game, graphics/WARP fixtures or injection experiments.

## Evidence baseline

- HEAD at start: `2644b0e06ed71f39b5f03e93dc98a81692da270b`; clean working tree.
- Published-subject ZIP supplied through VirusTotal:
  `7085EE28F93056E4C748904256A7C53D45CE5CE00D86617B162ABF1D47928EF1`.
- Contained ASI and existing standard Release output:
  `97696DBA3C40067F4F89D8C8F62D2D93C5AD2A97707860492E723A4E2016F5D4`.
- VirusTotal UI observed 8/68 for ZIP, 9/71 for contained ASI. No rescan or
  sample submission requested. Detection mechanism is not yet established.
- Local `v3.2.0` tag incorrectly resolves to the v3.1.0 commit. Do not mutate
  tags or remote release metadata as an incidental hardening operation.

## Ordered work

1. Establish a complete production/reachable-dependency inventory and threat
   model. Record per-file review scope and exclusions. Enumerate sensitive
   operations and trace callers, arguments, failure behavior and target scope.
2. Review hooks/memory, DLL/thread/exception lifetime, Overlay/input, native
   file/config/log paths, dependency provenance and build/package behavior.
   Separate confirmed defects from conditional risks and ordinary mod behavior.
3. For each concrete repair, record the causal evidence first, apply the
   smallest bounded change and add focused regression coverage. Preserve
   camera algorithms and established Overlay behavior. Do not change bytes
   merely to change AV scores or hide mechanisms.
4. Run focused checks and one serial NonGraphics canonical regression with
   externally bounded process-tree supervision. Build an isolated ordinary
   production Release ASI. Verify cleanup and artifact hashes; inspect final PE.
5. Review the cumulative diff and produce
   `research/reports/COMPREHENSIVE_ASI_SECURITY_HARDENING_2026-10-17.md`,
   including exact artifact/AV evidence, per-file coverage, fixes, validation,
   unresolved risks and a review/submission evidence packet. Stop before
   installation, publication, tag changes or AV submissions.

## Validation and containment

Read-only scans/PE parsing have a 60-second external bound and small outputs;
no target DLL is loaded. Compiler/test subprocesses use the repository's Job
Object supervisor, serial build jobs, finite per-step and total limits and
cleanup verification. Reuse established harnesses where they can prove the
changed contract; new tests must detect the original failure. The final report
must state incomplete coverage or unmet gates rather than promote them to PASS.

## Status

### Bounded follow-up: localization before architectural simplification

The user's latest direction is to investigate practical alternatives rather
than wait for an AV submission. First prepare byte-exact section evidence from
the v3.1.0 archive, published v3.2.0 archive and current ASI. Keep all three
inputs unchanged. The preparation tool lives in research/tools; generated
samples and manifest live only below build-artifacts. Parse and hash, never
load a DLL. Use one supervised CPU-only process, 60-second timeout, 1 MiB output
budget and verify cleanup. No upload, antivirus exclusions, automatic scans,
functional component removal, production rebuild or graphics work.

Acceptance: nine raw samples match their input file ranges, imports/resources
are attributed to exact hashes, and the report distinguishes raw-byte evidence
from an AV verdict or subsystem root cause. Negative raw-section scans cannot
exonerate a component. This is a first localization experiment, not a claim that
section splitting replaces component-level source isolation. A functional
simplification requires a concrete replacement contract and separate validation.

Hardening/qualification batch implemented and validated. Master report and
per-file review-depth ledger are prepared for review. The unrestricted full
independent implementation-audit claim remains unestablished: structural-only
and vendor-use review are explicitly distinguished from boundary review. No
user closure, release approval, tag rewrite, installation or AV submission.

Concrete repair evidence before edits:

- SH01: `NativeFile::Open(CREATE_ALWAYS)` follows final-component links and
  truncates before validating the target. Both config writers reuse `.tmp`,
  and template synchronization deletes that pathname even when opening fails.
  A pre-existing linked staging file can therefore damage an unrelated file.
  Repair: validate disk/write targets before truncation; exclusive staging
  creation; never remove a staging pathname not successfully created by us.
- SH02: `NativeFile::ReadAll` reads without a size budget; main/Overlay logs
  have no session-size limit. Bound configuration reads and log output, with
  explicit truncation/suppression semantics and focused tests.
- SH03: ImGui registers an unused shell-launch callback and unconditionally
  searches for XInput DLLs, although production enables neither links nor
  gamepad navigation. Remove those capabilities through upstream switches.
  Also compile out unused ImGui disk-file helpers: fonts are embedded and
  configuration/log transport already has explicit owners and safety checks.
- SH04: executable trampoline/stub/patch writes have no instruction-cache
  flush, contrary to the Windows generated-code contract. Add bounded flushes
  and refuse success on failure; test real execution and injected failures.
- SH05: `WorkerLifecycle::Start` suspends each new worker solely to publish its
  handle, then calls TerminateThread if resume fails. Its serialized control
  contract permits ordinary CreateThread publication without that mechanism;
  workers only read the pre-initialized stop event. Remove suspend/terminate
  and test immediately completing workers plus existing lifecycle cases.

These are source-backed hardening findings, not proven causes of AV labels.
The v3.2.0 tag mismatch remains a release-provenance finding; tags are unchanged.

- SH06: SafetyHook's trap manager did not check AddVectoredExceptionHandler's
  result before removing execute permission during patching. A registration
  failure can expose threads to an access violation without the required
  handler. Require successful registration before any protection transition;
  verify refusal and later recovery through the existing fault harness.
- SH07: the hotkey worker polls configured keys even when hotkeys are disabled
  or another application is foreground. Limit sampling/dispatch to enabled
  hotkeys in the foreground game process; consume already-held keys on return.
  Test the actual sampling helper with an instrumented reader (zero calls when
  disabled/background, only the four configured action keys when admitted).

## Final batch evidence

- Final `test.cmd -NonGraphics`: 62/62 invocations; 208 adapter children, four
  Zoom invocations; exit 0, no timeout/output cap, process-tree cleanup verified.
- Review found a defect in the initial log cap: partial WriteFile progress on
  failure was not counted. Stop further writes after either record/marker
  failure; real-prefix fault tests and the final cumulative run pass.
- Isolated ordinary production Release: exit 0, cleanup verified; no root ASI
  publication. Size 3,551,744; SHA-256
  `86C234BBB04B3597EA77F7F16413A99DC7B025CAD960AE17D44F1C23104248DD`.
- Final PE: 29/29 source resources match; ShellExecuteW, LoadLibraryA,
  TerminateThread and ResumeThread absent; FlushInstructionCache present.
- Original root/release-assets ASI and v3.2.0 ZIP are unchanged. Nexus download
  is unavailable due to quarantine; the user explicitly declined further
  retrieval work. Analyze the local ZIP matching the supplied VT hash.
- SH08 local tag mismatch and SH09 exact vendor provenance/deeper independent
  coverage remain documented, not silently marked fixed. AV cause unknown;
  no new artifact scan or live game qualification.
