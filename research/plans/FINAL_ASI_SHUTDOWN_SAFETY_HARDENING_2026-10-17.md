# ASI shutdown safety hardening

User-approved scope: close SL01/SL02, validate without graphics/WARP, build an
isolated Release candidate, update the shutdown report; do not install.

## Implementation order

1. Add a small owned Win32 file primitive and an unregistered synchronous spdlog
   sink. Replace the Overlay global ofstream with resident owner/lock storage.
   Replace persistent log FILE streams and transient INI FILE streams: /MT stdio
   termination enumerates both. Preserve config parser and replacement rules.
2. Move pending notification storage/counter/mutex into the existing resident
   Runtime. Keep SafetyHook's TrapManager resident after its first creation,
   under the existing initialization mutex, so no static map/VEH teardown runs.
   Pin the ASI from its existing initialization worker before publishing hooks
   or starting Overlay; reject initialization if process residency fails. This
   enforces the required lifetime after publication against later FreeLibrary.
   Leave thread-local deferred logging unchanged: killed non-exiting threads
   receive no TLS teardown; no demonstrated cross-thread destructor path exists.
3. Add deterministic file/log/lifetime checks and production-adapter notification
   coverage; retain existing detach/drain/partial/repeated shutdown tests.
4. Build serially under the existing external Job Object supervisor. Execute
   canonical non-graphics entries and adapter cases individually excluding the
   WARP case. Record omissions explicitly, verify descendants are gone, run
   diff/static checks and build/hash an isolated ordinary Release ASI.

Expected files: platform Win32 file/log headers, Runtime, Overlay composition,
config file transport, SafetyHook singleton, focused tests/runner metadata,
architecture and shutdown report. No camera algorithms, UI, toast, graphics or
DllMain changes. Preserve unrelated working-tree changes.

Acceptance: no automatic destruction of the identified resident mutable
containers; no production persistent/transient FILE streams in the established
log/config paths; actual concurrent logging and no-op/config roundtrip checks;
bounded non-graphics regression; unchanged installed ASI; truthful scope of
Windows termination and unsupported explicit unload.

No blanket Windows/CRT safety claim: other libraries and the host process remain
outside this bounded repair. No joins or new threads are introduced in DllMain.

## Implemented boundary refinements

- Disabled ImGui's unused automatic INI transport: every mod window already
  has NoSavedSettings and custom placement stays in the main INI. This removes
  another established FILE load/save path rather than changing UI layout.
- The retained SafetyHook VEH checks a cached RtlDllShutdownInProgress function
  before its mutex/map. Resolution occurs outside that mutex; missing capability
  refuses patch installation. Its focused native test holds the actual mutex.
- The allocator's mutable static weak-reference holder is resident too; its
  automatic destructor had the same interrupted-state issue as the trap map.

## Verified handoff

Implementation and offline qualification complete for the scoped SL01/SL02
paths. Focused 12/12 PASS; canonical `test.cmd -NonGraphics` 62 invocations /
208 adapter processes / four Zoom scenarios PASS. WARP deliberately omitted.
Both automatic-owner and old-FILE negative controls recorded destructor entry
without completion, with exit code 7 on this Windows; not called a deadlock.
The resident DLL exit/pin/FreeLibrary test passes. Process cleanup verified.

Ordinary Release candidate: `build-artifacts/shutdown-hardening/STALKER2CameraTweaks.asi`,
3,568,128 bytes, SHA-256
`97696DBA3C40067F4F89D8C8F62D2D93C5AD2A97707860492E723A4E2016F5D4`.
The candidate was not installed or launched automatically by the agent. The
user subsequently tested this SHA in a normal game session and confirmed an
ordinary exit. The three logs showed successful initialization and Overlay
open/close transitions without logged errors; the process was absent afterward.
The report's section 10 supersedes its earlier unresolved dispositions. Live
exit qualification is complete for that observed scenario; it does not certify
all Windows, crash, unload or graphics-driver teardown paths. No domain
regression replay was performed or required.
