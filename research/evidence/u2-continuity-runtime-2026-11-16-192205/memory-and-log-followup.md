# Memory and PID log follow-ups

User reports roughly 2 GB more RAM/pagefile usage at game startup after agent
work. Exact counter (game private memory, whole-system commit, or pagefile
allocation/usage) is awaiting clarification. No heavy tools/tests were launched
for this check, and no processes or tracing sessions were stopped.

Clarification resolved: MSI Afterburner / RTSS. Read-only inspection of
`C:/Program Files (x86)/MSI Afterburner/Profiles/MSIAfterburner.cfg` shows
`+RAM usage,-RAM usage \\ process,+Commit charge` in Sources. Both Source RAM
usage and Source Commit charge have ShowInOSD=1. Thus the selected counters are
system RAM and system commit, not game-only RAM or physical pagefile usage.
The configuration was not changed. Additional memory in Codex, VS Code and
editor indexers contributes to these global readings even when shown over the
game; the observed 2 GB delta still lacks before/after process measurements.

Host Windows inspection, with read-only access outside isolated execution:

- No Stalker2, Ninja, CL, vctip, harness, CMake, PresentMon, WPR or WinDbg
  processes were found. This does not establish the absence of historical leaks
  or graphics-driver retained resources.
- The default isolated process query saw only its own pwsh/conhost; it is not
  sufficient evidence about host cleanup. Host checks used the actual Windows
  process list and CIM readback.
- Physical RAM: total 32,675,940 KiB; free 19,165,064 KiB at snapshot.
- Pagefile E: allocated 70,000 MB; actual current use 139 MB, peak 139 MB.
  Allocated size must not be described as current pagefile consumption.
- Group private bytes rounded to MiB: Codex/ChatGPT 2352, VS Code 2032,
  cpptools family 773, SCSKiller 1420. Parent chains show these as user app /
  editor extension processes, not surviving launched test children. Their
  before-work sizes are unavailable; no causal attribution from this snapshot.
- Active ETW sessions were enumerated; no named project recording was found.
  Existing Windows/Steam/vendor sessions were left untouched.

Next discriminating measurement: identify the user's observed counter and
capture the actual game's private bytes plus system commit/pagefile usage at
the corresponding startup/load point. Compare the same scenario before/after
work; do not infer a 2 GB mod leak from a difference in free RAM alone. Current
game is closed, so its live memory cannot be measured retrospectively.

PID logs are intentional fallback for overlapping processes. Latest launch:
28448 starts 19:22:01.107 and exits 19:22:06.026; playable PID 25756 starts
19:22:05.062 and exits 19:25:18.674. File suffixes distinguish these owners.
The prior .5552 files belong to the preceding session. Total current camera
log disk size is 3,178,178 bytes, not gigabytes. No automatic retention policy
exists; disk accumulation is a real separate concern. Any bounded retention
repair must preserve files still open by live owners and must not weaken
exclusive file ownership or reintroduce fatal startup sharing conflicts.
No game-directory files were changed or deleted for this investigation.
