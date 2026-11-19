# U2 provider-to-Dialogue writer endpoint — runtime evidence and offline repair

## Runtime record

- Source: user-supplied STALKER2CameraTweaks.log, archived beside this report.
- SHA-256: D55E48C02A2F9C49617989188EFF192D0848401CF9509F08021DF256BD29DB77.
- Image identity in runtime log: game 2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479; ASI A439FC24FFD46907A91F5056A0B9AF0BFEA8D629CAADA5F867119E24CDE74BEE.
- User reports that the previous repeated FOV motion is gone; after Cinematic→Dialogue, FOV moved gradually then jumped at the end.
- U2_CONTROL_BEGIN is absent. The optional control Dialogue was not exercised in this capture.

## Confirmed records

At 14:29:25.905, the post-exit provider Dialogue callback was admitted from the retained Gameplay baseline: aseline=90.3115, 
ative_target=70, policy=Reduced, and mutated callback result 80.2035. Immediately after, the same receiver's first Gameplay writer sample produced 118.602 from 80.2035; its baseline projection was RETAIN with the original 90.3115 / 127.119 pair.

At 14:29:25.917, the next native writer sample was 70.0357. The preceding Dialogue callback had been treated as a fresh Candidate because the previous callback had reset Dialogue state. The writer produced 108.977, a 9.625° decrease from the preceding writer output. The user independently observed the end-of-transition jump. The writer values correlate with that report; the log does not prove which exact value the rendered view consumed.

The reset reason is explicit at 14:29:25.914: the generic Gameplay context FOV-jump detector compared prior 90.3115 with the intended Dialogue-start value 80.2035 and reset Dialogue runtime state. Baseline contamination is not present in this capture.

## Offline correction

The provider activation's consumed, completed identity now authorizes a short continuation only while the Dialogue lifecycle remains Active and its candidate remains bound to the same receiver, settings epoch and cinematic generation; thread identity must also match. The exact callback result is carried as a one-shot writer-start witness. That first matching writer sample receives the existing Gameplay HorPlus transform. Subsequent native writer samples are mapped through the active Dialogue destination policy before Gameplay HorPlus, preserving the policy endpoint without using an arbitrary cooldown. Other writers keep the existing path.

Deterministic coverage models the observed 80.2035 → 70.0357 writer sequence and checks both outputs against the same Reduced+HorPlus endpoint, checks that the context detector does not retire the bound lifecycle, and checks baseline retention.

## Limits

This is offline writer-transaction evidence. Rendered-view continuity still needs one runtime confirmation on the new single Release ASI. The missing control phase in this user capture is not repeated because the previous separate control Dialogue evidence already exists; this repair is specifically about the provider handoff.


## Offline repair and artifact validation — 2026-11-14

The current source now carries the provider-started Dialogue lifecycle through the endpoint writer when the completed provider identity, bound Dialogue receiver, active lifecycle, settings epoch, cinematic generation and thread all match. It uses a one-shot exact callback-output witness for the first writer sample. The following native writer samples map through the active Dialogue policy using the observed native target, then use the existing Gameplay HorPlus correction. This prevents the generic context-jump detector from retiring this specifically bound Dialogue lifecycle. Ordinary Gameplay behavior is unchanged outside the qualified handoff.

The new regression models the observed `80.2035° → 70.0357°` writer inputs. Both outputs are within `0.03°` of the same Reduced+HorPlus endpoint; the bound Dialogue lifecycle remains active and the Gameplay baseline projection remains RETAIN. The `0.03°` allowance covers the logged `0.0357°` native endpoint offset from `70°`, propagated through the projection.

Validation on the final source:

- `camera_writer_transaction_harness`: PASS.
- `dialogue_boundary_transaction_harness`: PASS.
- `dialogue_fov_harness`: PASS.
- `runtime_native_adapter_harness`: PASS, 230 fresh processes with Diagnostics OFF/ON.
- `test.cmd -NonGraphics`: registry audit, CMake configure, harness build and exit-code contract passed. The canonical registry stopped at invocation 10/62 because the pre-existing Windows hardlink fixture returned error 5 (`occupied=1`). The focused relevant harnesses above were run directly under the project process-tree supervisor and passed.
- Production Release build: PASS on the incremental recheck, exit 0, including rebuilt Dialogue projection/runtime and ASI link. `dumpbin` confirms x64 PE32+ DLL. Process cleanup was verified; the expected `vctip.exe` helper had exited.

Candidate: `build-artifacts/u2-provider-dialogue-endpoint-fix-2026-11-14/production/STALKER2CameraTweaks.asi`, 3,911,168 bytes, SHA-256 `D206A6AF12882E8EE4C9E50E17D0CCD6211344EBA0EB878DE6983FC4343A66A5`. See the adjacent `candidate-manifest.json` and `validation-summary.json`. It is one Production ASI; Diagnostics remain controlled by the existing INI setting. It has not been installed over the game copy.

Rendered-view continuity remains unverified. The next useful runtime check is the same Cinematic 1 → Dialogue transition on this exact candidate; the separate control Dialogue can be included after natural recovery in the same session, without loading the save again. Dialogue → Cinematic 2 remains a separate unresolved visual boundary.
