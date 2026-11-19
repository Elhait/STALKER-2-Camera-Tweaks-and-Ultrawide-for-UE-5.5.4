# Startup crash assessment — 2026-11-15

## Observed run

The current game log reports the ordinary Release ASI SHA-256
`D101064AE314C47A09C6927AEA0D1E6771571C9FA5233FA43413CED0F750635B` and game
image SHA-256
`2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`. Diagnostics
were enabled. The camera-writer, post-store, post-helper, Dialogue and cinematic
capabilities all resolved and initialized. The last available periodic camera
summary at 01:45:53 reported zero writer transactions; the log then stopped,
before a camera writer transaction was observed. No matching new dump was
present in the inspected Win64 or UE4SS dump folders.

The overlay startup journal reports `dxgi_factory_hooks=0 swapchain_hooks=0`.
The supplied crash stack contains `sl_interposer` and `d3d12`. The contemporaneous
`scskiller.log` says `sl.interposer.dll` was hooked at 11.2 seconds and records a
Streamline factory observation at 14.3 seconds. These records make the injected
frame-generation/render interception path a concrete lead, but do not prove it
caused the access violation. The crash stack alone also does not exclude an
interaction with this mod's startup hooks.

## Assessment

The new diagnostic serialization path only runs when writer events reach its
worker. Since no camera writer event was recorded before the log stopped, the
new one-line-per-transaction serializer is not evidenced as having executed in
the failing interval. This weakens a direct causal link to that change, but does
not rule out an install-time interaction from the optional diagnostic observer
or another startup component.

The following user-provided run used the same ordinary ASI and game image with
`[Diagnostics] Enabled=true`, initialized the same capabilities, recorded 2,600
writer transactions, and did not crash. This materially weakens a deterministic
causal link between the diagnostic-enabled build and the reported access
violation. It does not explain why the earlier run crashed; the `sl_interposer`
path remains a hypothesis only. No further diagnostics-off comparison is
currently required unless the crash recurs. No FOV policy change is justified
by this crash evidence.

A later run loaded the refreshed ordinary ASI hash
`FA236C5F2C5C3D9982B9FAC4D70B3D0B3A7037C3FF54577A67159F7F4D5F096A` with
diagnostics still enabled and reached Gameplay, Dialogue and both Cinematic
writer segments. Its 5,217 transaction summaries show no hook/readback
correlation failures or queue loss, and the supplied log contains no crash
marker. This further establishes successful execution of the current observer
through that scenario; it still does not prove the earlier access violation's
cause or final rendered behavior.
