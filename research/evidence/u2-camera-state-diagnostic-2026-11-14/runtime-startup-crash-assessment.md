# Runtime Startup Crash Assessment

Date: 2026-11-14

## Confirmed observations

- The game loaded the diagnostic candidate. `STALKER2CameraTweaks.log` records
  `modSha256=7654E719E4593B7D3073EDF5C962BB733961C897324AAF01E14FE7692381B4F6`,
  matching the candidate manifest.
- The log records the camera-state diagnostic worker starting, configuration
  with `Diagnostics.Enabled=true`, and cinematic aspect signature validation.
  No camera writer transaction had yet been captured in the available log.
- The two available crash dumps, at 22:41:28 and 22:41:47, have the same
  exception instruction and stack. The recorded exception is an access
  violation writing to `0x7ffbcd0c2db0` at
  `STALKER2CameraTweaks+0x1b605c`; return addresses are within the same module
  (`+0xc139b`, `+0xc1524`, `+0x10b0bc`, `+0xc06dc`, `+0xc1289`).
- PE section inspection maps the destination to ASI RVA `0x2f2db0`, inside the
  read-only `.rsrc` section (`RVA 0x2d0000`, virtual size `0xffc00`, section
  characteristics `0x40000040`). The faulting instruction is a four-byte copy
  from `RDX` to `RAX`. The disassembled caller passes a source address in the
  game image and obtains the destination from a mod global pointer. This
  confirms an invalid destination pointer on the startup path; it does not yet
  identify which C++ owner produced that pointer.
- The dump does not currently resolve private function names or source lines.
  The faulty copy and read-only destination are confirmed, but the responsible
  source function and the specific invalid state are not yet established.

## Assessment

This candidate is not suitable for another game launch in its current form.
The crash happened before the intended Gameplay/Cinematic/Dialogue capture,
so there is no U2 transition evidence from this run. The diagnostics worker
did start, but the available stack does not prove that it caused the access
violation. Do not attribute the crash to a particular new diagnostic function
until the module offsets are symbolized or mapped to the exact Release build.

## Next investigation

Map the faulting instruction and return addresses to the exact Release build's
functions, trace their startup call path and ownership/lifetime assumptions,
then add a focused regression for the established cause. Build and validate a
revised candidate before requesting any further runtime attempt. No new game
launch is requested by this assessment.
