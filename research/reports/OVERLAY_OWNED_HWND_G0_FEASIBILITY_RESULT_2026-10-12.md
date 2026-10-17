# Overlay-owned HWND — G0 feasibility result

Date: 2026-10-12. **G0: PASS within the isolated local desktop fixture.**
Renderer integration and G1–G5 remain unapproved. This is not an ASI or game test.

## Subject and execution

[Approved bounded plan](../plans/OVERLAY_OWNED_HWND_G0_FEASIBILITY_2026-10-12.md)
and [design](OVERLAY_OWNED_HWND_BOUNDED_DESIGN_REVIEW_2026-10-12.md).
Fixture: `research/prototypes/overlay_owned_hwnd/g0_feasibility.cpp`.
Runner: `research/tools/run_overlay_owned_hwnd_g0.ps1`.

One local surrogate host HWND on the main thread and one owned WS_POPUP Overlay
HWND on a different worker thread. Overlay styles were exactly NOACTIVATE,
TOOLWINDOW, LAYERED and, in passive mode, TRANSPARENT. Layered alpha was 255,
without a color key or NOREDIRECTIONBITMAP. One private hardware D3D11 device
and premultiplied B8G8R8A8 DComp surface; no production source dependencies.

Build: MSVC via VS 2022 developer environment 17.14.28; `/std:c++latest /EHsc
/W4 /WX /utf-8 /O1 /MT`, compile and link exit **0**, no warnings in build log.
Libraries: user32, gdi32, ole32, d3d11, dxgi, dcomp, dwmapi.
Windows registry reports build **26200.9457**, DisplayVersion **25H2**.
GPU/driver identity was not collected: CIM access was denied. Hardware D3D11
creation succeeded; no WARP substitution or compatibility claim is inferred.

The sandbox execution could sample rendered pixels but could not deliver cursor
input: SetCursorPos failed in the initial version; final fixture SendInput returned
0. That run ended with fixture exit **2 (unverified/error)**, not PASS or a design
failure. A separately permitted unsandboxed execution of the same final EXE
delivered native input and exited **0**. No synthetic window-message substitution
was used. The fixture checked foreground and the window under the cursor before
clicking; input was limited to its own two windows.

## Observed results

| G0 criterion | Observation | Result |
| --- | --- | --- |
| Fully transparent region | Screen pixel RGB 40,100,160 equals the surrogate host background | PASS |
| Premultiplied half-alpha | Screen pixel RGB 148,50,80 matches half-red over the host background | PASS |
| Visible opaque content | Screen pixel RGB 255,0,0; content exists rather than the entire window being invisible | PASS |
| Passive click-through | Click on the opaque marker: host down count 1, Overlay down count 0 | PASS |
| First interactive click | Remove only TRANSPARENT, apply FRAMECHANGED: host count remains 1, Overlay count becomes 1, WM_MOUSEACTIVATE count 1 | PASS |
| No activation/focus theft | Foreground remains surrogate host and its GUITHREADINFO focus is the host after each click | PASS |
| Persistent target | Same target pointer throughout rendering and the passive-to-interactive style transition; one creation | PASS |
| No surrogate-game target | The sole CreateTargetForHwnd call takes OverlayHwnd; no target call on surrogate host | PASS |

The host/Overlay thread IDs in the successful run were 30384 / 27184. Thus
pass-through was exercised across threads, not approximated with HTTRANSPARENT
same-thread forwarding. WM_MOUSEACTIVATE returned MA_NOACTIVATE, not ANDEAT.
Pixels were sampled from the desktop after a fixture-only DwmFlush. The saved
screen capture was also visually inspected: host-colored, half-alpha and opaque
regions are visible, without an opaque full-window background.

## Evidence and identities

[Successful native run](../evidence/overlay-compatibility/owned-hwnd-g0-2026-10-12/run-desktop.log),
[sandbox limitation](../evidence/overlay-compatibility/owned-hwnd-g0-2026-10-12/run.log),
[build log](../evidence/overlay-compatibility/owned-hwnd-g0-2026-10-12/build.log),
[screen capture](../evidence/overlay-compatibility/owned-hwnd-g0-2026-10-12/g0-passive.bmp).
The local EXE is preserved alongside these records; global executable ignore rules
mean it is not a Git-distributed release artifact.

- Final fixture source SHA-256: `6DE8E34ACFF74DABFB7E86A8FF6B646226E2735B2A09D27A483D9F51E1E22CBD`.
- Tested EXE SHA-256: `CEBC6F099C627A15BDE1CAD79E9E7EAF2C4EEE9E10E18637DFE134EFAA7C817C`.
- Preserved C-root ASI remains `78357B990DC705DD3973339E0EA4FE733E2484A2DCB1461F7C33F859BC2B624E`.

Microsoft explicitly permits layered HWND composition targets and documents
layered/transparent mouse pass-through; the fixture supplies local behavioral
evidence for this particular combination, beyond successful API return values.
[CreateTargetForHwnd](https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-idcompositiondevice-createtargetforhwnd),
[layered windows](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features).

## Limits, review and next boundary

This verifies only the selected G0 pixel/input contract on this local desktop.
It does not test production ImGui, keyboard/raw-input routing, cleanup handoff,
resize/DPI, exclusive fullscreen, HDR, repeated show/hide lifecycle, NVIDIA HUD,
G-SYNC, FG or performance. Stable pointer identity during this short experiment
is not a stress-test of device/window recreation. DwmFlush and fixture foreground
setup are measurement conveniences, not proposed production behavior.

Bounded source review: target creation is restricted to OverlayHwnd; worker-scope
COM objects retire before DestroyWindow on that worker; input failures exit
unverified rather than becoming PASS; native clicks are checked against fixture
focus/window identity. No game discovery, injection, ASI dependency, extra renderer
owner, hook, configuration key or production change was introduced.

Recommendation: accept G0 feasibility and stop. The next separately approved
design step is the **cleanup handoff review**, particularly immediate fail-closed
input withdrawal versus owner-thread Renderer::Shutdown. Do not integrate into
Renderer or start G1–G5 on the strength of this result alone.

Production root detach, all prior target-release ASIs, release artifacts and the
pinned C-root are unchanged. Canonical tests/builds and the game were not run.
