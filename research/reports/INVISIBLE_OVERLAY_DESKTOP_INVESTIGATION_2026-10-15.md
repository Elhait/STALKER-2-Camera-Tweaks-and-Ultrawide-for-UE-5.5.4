# Invisible Overlay — causal desktop investigation

Date: 2026-10-15  
Status: **Desktop cause reproduced; bounded production correction built and verified in the standalone host. User reports the installed candidate shows the menu, but the startup welcome toast is only noticed on manual open; that presentation symptom remains unresolved.**

## Executive result

The production display window could remain logically visible and keep input capture active while sitting behind the host/game window. This was reproduced using the preserved G5 and F4 production component sets: raise the foreground host above the unowned, non-activating Overlay HWND without changing size, DPI, content, or focus. The toast/menu then disappeared from desktop pixels while the input bridge still consumed clicks.

The previous placement logic reused cached geometry and visibility and did not require the display HWND to remain above the host. Relative `HWND_TOP` placement did not recover visibility in the controlled restore case; placing the active display in the topmost band did. A bounded production correction now keeps the visible client-sized Overlay HWND topmost, checks actual display-over-host order before admitting input, and reconciles asynchronously after `WM_WINDOWPOSCHANGED`.

This confirms a real production presentation defect and a working correction for the reproduced desktop mechanism. It does **not** establish the exact z-order event that occurred in the user's game session, nor prove compatibility with the game's fullscreen/composition stack. No game was launched.

## Compared production revisions and provenance

| Subject | ASI SHA-256 | Component objects used |
|---|---|---|
| G5 reference | `123D792A61A550892141DBEC12BB059A606BEA6A30B0C8B300491B6CAFC7DEF2` | `build-artifacts/g2-g3-candidate/cmake/production-tOFF-dOFF` |
| F4 reference | `24A50EAF166FD52E68AD90A5712C1415E5DC6CC4BA6D4157CC4BD53605A43E86` | Immutable copy under `build-artifacts/invisible-overlay-desktop/F4/preserved-inputs` |
| Corrected current candidate | `9ECB5E637BBDAA6D5658AFB76A764592CAB167CD9E0B8264386AD8E59EA84643` | Canonical current production objects, rebuilt by `build.cmd` |

The desktop host links the complete applicable production object set and matching localization resource, omitting only `dll_entry.cpp`; DllMain starts game-specific initialization and is not a standalone Renderer entry point. The fixture supplies synthetic settings/camera observations and does not install camera hooks. It exercises the production Overlay discovery, worker, input bridge, renderer, DComp presentation and finalizer. This is a component-level native desktop test, not execution of the ASI inside S.T.A.L.K.E.R. 2.

The short-tag INI hypothesis is not established. The G5 and F4 reference components also reproduced the visibility/input mismatch under host raise, so the available evidence does not show that changing the documentation marker caused this defect.

## Causal reproduction

The fixture measures actual desktop pixels with screen DC + `BitBlt(CAPTUREBLT)` and records HWND visibility/z-order. The transparent host background and separately sampled toast/menu regions are checked independently. A deliberately hidden display is a negative control and must fail the same visibility oracle.

| Revision/scenario | Desktop observation | Input/exit | Conclusion |
|---|---|---|---|
| G5, normal | Toast and menu visible before host raise; after host raise the display remained logically visible but the menu was absent from captured pixels | Input remained captured; exit 1 | Reproduces invisible-but-interactive state |
| F4, host raise | Toast/menu pixel regions absent; display HWND visible but below host | Input remained captured; exit 1 | Same defect exists in F4 reference |
| Fixed presentation objects, explicit `HWND_TOP` restore | Accepted call did not restore display pixels or relative order | Exit 1 | Relative top placement is insufficient in this tested desktop condition |
| Fixed presentation objects, topmost policy | Toast/menu visible; host raise, input return, hidden negative control, reopen, focus and minimize/restore passed | Exit 0; cleanup verified | Confirms the topmost-band mechanism in the preserved F4 component set |
| Current production candidate, final ROI oracle, Diagnostics OFF | Toast 1,014 changed samples; toast-only control correctly rejected as menu; full menu 52,983 changed samples (41,306 in menu-body ROI); host raise remained visible; deliberate hide 0 samples; reopen, focus, minimize/restore and returned game click passed | Exit 0; timeout false; cleanup verified; finalizer COMPLETE | Confirms corrected production components and final fixture oracle |

Current run evidence is in `build-artifacts/invisible-overlay-desktop/Current/20261015-145919-742-off/`. The saved `menu.bmp` was visually inspected: the settings panel is visible over the blue host, with the host visible through transparent Overlay regions. `toast-only-not-menu.bmp` validates that the toast cannot falsely satisfy the menu-body test.

## Implemented correction

- `renderer_runtime.cpp/.hpp`: add an owner-thread check that traverses the window order above the host and verifies that the display HWND is actually above it. Input presentation readiness now requires this condition.
- `renderer_runtime.cpp`: position an active display in the topmost band while retaining host-client geometry; invalidate cached placement when actual order/topmost state is wrong. Inactive/minimized states remain hidden; the game HWND is not modified.
- `composition_runtime.cpp`: revoke input promptly on `WM_WINDOWPOSCHANGED`, then enqueue a nonblocking owner refresh. The borrowed `WINDOWPOS*` is not retained; owner code re-queries current HWND state.
- `composition_runtime.cpp`: add an owner-thread terminal reason to finalization diagnostics. This is diagnostic-only and does not add file I/O or waiting to the game WndProc.
- `docs/ARCHITECTURE.md`: document the active display topmost policy and readiness/order requirement.

An experimental foreground WinEvent observer did not solve the restore case and was removed, including its cleanup handle.

The change does not create a thread, polling loop, DComp target lifecycle, or mutable runtime state. It does change the active display's z-order band. This could affect composition with other always-on-top surfaces; the desktop fixture cannot qualify the game's presentation policy.

## Focused validation and cleanup

- `composition_presenter_state_harness` and `input_state_harness`: both executed successfully under the process-tree supervisor. These validate existing state/admission invariants, not pixel visibility.
- Canonical production ASI build: exit 0, no timeout, cleanup verified. Candidate SHA-256: `9ECB5E637BBDAA6D5658AFB76A764592CAB167CD9E0B8264386AD8E59EA84643`.
- Current standalone host: exit 0, no timeout, cleanup verified; logs show owner finalization `COMPLETE`, display destruction, and zero pending owner commands.
- No full `test.cmd`, game, ETW, PresentMon, or release packaging was run.
- Final process check found no desktop host, compiler, linker, CMake, Ninja, MSBuild, or VCTIP processes.
- During this investigation the user reported the display went black and required a forced PC restart. The interruption occurred while desktop validation was underway; there is no surviving trace identifying its cause. The final Current run was executed after restart and completed normally. Do not attribute the blackout to a particular test API or claim this run proves that incident cannot recur.
- At the initial post-build check, the installed ASI still matched the old F4 hash. The user subsequently tested the new candidate in the game; the latest read-only check shows installed ASI SHA-256 `9ECB5E637BBDAA6D5658AFB76A764592CAB167CD9E0B8264386AD8E59EA84643`, matching the candidate. Installed INI remains `72AA0EF530E454F7320D6E0B0E3CBA08807879641AF7CD0D6B8F30ED49542F28`.
- Repository-root ASI remains `B265DF72FF94A9BC05B8B9E57DA1C6B9F328530B19678D86098A4C6DDC64815F`; G5 and F4 reference ASIs retain their hashes above.
- Git index SHA remains `72FAC39B216993275F82BCB64A7032F1098FA30F8B3D09F105F97AED6404CD19`. Existing staged user work and previous F4 edits were preserved.

One earlier corrected-policy desktop run (`Fixed/20261015-144444-895-off`) exited after the display HWND was destroyed during restore; its logs did not include a terminal reason. The reason field was added afterward. Four later OFF runs using the fixed F4 component set completed successfully, and the final Current run also completed with `reason=display_close`. The unexplained earlier event was not reproduced or causally explained, so it remains a recorded limitation rather than a claimed fix.

## Follow-up live observation from the user

The user reports that the installed candidate made the Overlay visible, but the welcome toast appeared only when the menu was opened manually. The first live log set had Diagnostics OFF and did not capture host activity/z-order. A later log set from the same installed candidate has Diagnostics ON and provides more detail:

- Renderer activation and first committed surface at 15:02:22.689–15:02:22.690 local time; input binding activation at 15:02:25.689.
- Overlay log order: `OVERLAY_STARTUP_HINT_WAITING_FOR_AUTO_LOCALE`, then `OVERLAY_STARTUP_HINT_CREATED`, `OVERLAY_NOTIFICATION_LIFETIME_STARTED id=0`, and `OVERLAY_STARTUP_HINT_DRAWN_AND_COMMITTED`, followed by the first `OVERLAY_VISIBILITY=VISIBLE` entry. That visibility entry is emitted by the manual toggle handler.
- In the later run, the first display-state record is `requested=0 root_attached=1 host_active=1 host_minimized=0 display_visible=1 z_order_above_game=1`, before the startup hint is created. The main runtime log records locale synchronization at 15:07:56.773.
- After `OVERLAY_STARTUP_HINT_DRAWN_AND_COMMITTED`, the log later records `root_attached=0 display_visible=0` while `host_active=1`; only after that does the first manual `OVERLAY_VISIBILITY=VISIBLE` occur and reattach/show the menu.
- The later run explicitly had `Diagnostics.Enabled=true`.

The later run rules out the simple explanation that the game was inactive or that the display was below it at the first recorded startup display state: it reports `host_active=1`, `display_visible=1`, and `z_order_above_game=1`. It also confirms that notification creation and composition Draw/Commit happen automatically before manual menu open. However, those state logs still are not a pixel capture. The later detach/hide occurs after the toast's lifetime has started and before the manual menu toggle; the overlay log has no timestamps per line, so it cannot establish how long the toast was displayed or whether the user saw its pixels. No second startup-hint creation is logged at manual open. The standalone host shows the toast before toggling, but that does not prove the game presented the toast pixels visibly. The remaining issue is therefore narrower: actual startup-toast pixels/timing in the game remain unverified despite successful automatic creation, commit, active-host state and z-order. Do not claim the symptom fixed from lifecycle logs alone.

The user confirmed the menu is now visible in-game. This is partial live evidence for the installed candidate, not a complete qualification of fullscreen/DWM/DXGI behavior or interactions with Streamline/DLSS Frame Generation, ReShade, OptiScaler, NVIDIA HUD and G-SYNC. The welcome-toast behavior remains open.

## Remaining qualification

The standalone host establishes that the current production components render and preserve input behavior in the tested desktop conditions. The user has also confirmed the menu is visible in-game. The exact reason the live welcome toast is not noticed until manual menu open remains undetermined because the run had Diagnostics OFF and no independent pixel capture during startup. The exact original trigger and broader graphics compatibility are not established.

N03 remains `Partially mitigated`; N04–N06 remain `Open`. This investigation changes none of those evidence statuses.
