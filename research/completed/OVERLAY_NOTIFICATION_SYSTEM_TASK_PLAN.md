# Overlay Notification System — Task Plan

## Objective

Add a generic ImGui/D3D12 toast notification subsystem positioned at the top center of the viewport and connect the existing F9–F12 runtime hotkey changes to it.

## Established evidence and current state

- The overlay renderer and input routing are runtime-validated.
- Runtime settings mutations already have a shared API and semantic projection.
- Existing F9–F12 hotkeys change production settings through established handlers.

## Approved scope

- Add a bounded notification queue with title, change/value text, semantic status, lifetime, and generation/id.
- Render notifications at the top center, stacked downward, with fade-in/fade-out.
- Limit visible notifications to three; expire older notifications automatically.
- Notifications do not capture input, open the main overlay, or depend on overlay visibility.
- Emit notifications for existing F9–F12 changes using actual semantic status where available.

## Explicit non-goals

- No hotkey configuration/rebinding UI.
- No toast for direct ImGui control changes in this batch.
- No Windows notifications, persistence, new camera logic, or camera hook changes.

## Expected files or areas

- overlay notification state/rendering files;
- existing runtime hotkey/mutation boundary;
- deterministic notification and hotkey integration tests;
- build/test task log.

## Batches and validation

1. Define notification data/state and deterministic queue behavior.
2. Render the top-center stack without input capture.
3. Connect F9–F12 at the existing mutation boundary.
4. Run deterministic tests, `test.cmd`, production build, overlay build, `git diff --check`, and read-only Git review.

## Risks and safe failure

- Risk: notifications accidentally become a second settings owner. Mitigation: emit only after the existing mutation path accepts a change.
- Risk: toast rendering changes overlay input behavior. Mitigation: draw without hit testing or focus changes.
- Safe failure: queue overflow drops the oldest notification; settings behavior remains unchanged.

## Stop conditions and phase gates

- Stop if F9–F12 do not share a stable mutation boundary without changing camera behavior.
- Stop if notification rendering requires input or lifecycle changes.
- Do not launch the game automatically; runtime validation is a separate step after static validation.

## Final review

- Confirm no hotkey settings UI or camera behavior changed.
- Confirm notifications are top-centered, non-interactive, bounded, and hidden only by expiry.
- Archive this plan under `research/completed/` after validation passes.
