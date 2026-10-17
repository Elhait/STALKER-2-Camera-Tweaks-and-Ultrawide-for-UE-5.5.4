# Release 3.2.1 bounded engineering pass

## Goal

Improve release identity and make only source-proven, behavior-preserving
simplifications in Overlay binding, module/patch lifetime and INI persistence.
Produce one isolated Release ASI candidate. This does not claim to fix an AV
classification.

## Evidence and change points

- CMake owns the production source list, embedded resources and isolated Release
  output. Add Windows VERSIONINFO there, with one explicit 3.2.1 version source.
- `composition_runtime.cpp` and `game_thread_subclass.hpp` own the one-shot
  game-thread WndProc handoff. Keep the thread-affinity and predecessor
  publication guarantees.
- `module_residency.hpp`, `runtime.cpp` and `external/safetyhook/safetyhook.cpp`
  own callback residency and executable patch synchronization. Retain only
  simplifications proven equivalent to these guarantees.
- `config_repository.cpp`, `config_template.cpp` and `native_file.hpp` own INI
  path selection, bounded reads and atomic persistence. Preserve user data and
  the process-termination safety rationale.

## Batches

1. Add standard VERSIONINFO to the normal ASI. Verify all string and numeric
   versions on the built PE.
2. Review Overlay handoff and teardown transitions for redundant states,
   duplicate ownership or Win32 calls. Edit only if a concrete redundancy and
   equivalent transition are proven.
3. Review module pinning and SafetyHook VEH/patch lifetime. Edit only if a
   simpler mechanism proves identical callback lifetime and patch safety. Remove
   only the unreachable TrapManager destructor flag/branch; preserve all active
   handler, map and patch synchronization.
4. Review INI access for path escape, unintended files, redundant operations and
   destructive failure. Fix concrete issues only.
5. Run focused checks for changed contracts; build one isolated Release ASI;
   record SHA-256 and inspect VERSIONINFO. No game, graphics fixture, full test
   suite, AV scan/upload, install or publication.

## Status at handoff

- VERSIONINFO implemented and checked on the isolated 3.2.1 PE.
- Removed only unreachable TrapManager destructor state. Focused native/SafetyHook
  tests pass with Diagnostics OFF/ON.
- Overlay handoff and INI safety reviewed; their existing complexity carries
  needed lifecycle or data-preservation guarantees, so no change was justified.
- Candidate built; targeted validation passed. Root ASI/release ZIPs untouched.
- No AV verdict or game-runtime evidence for this candidate.

## Acceptance and non-goals

- No known safety invariant is weakened to alter AV scoring.
- Existing installed ASI and release archives remain untouched.
- Any reviewed area with no source-proven defect is recorded as reviewed with
  no change; absence of an edit is not presented as a defect fix.
- The candidate is distinct from the user's root ASI and is not installed.
- No VirusTotal or Nexus compatibility claim is made for the candidate.
