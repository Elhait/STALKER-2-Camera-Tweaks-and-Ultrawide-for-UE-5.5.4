# Startup toast positive live baseline

- Run date: 2026-10-16, log files last written around 19:58 local time.
- Mod artifact SHA-256: `72F373AB015D93D1662E2D62DCAB67B4FF681C3202D7C08F343748790BFD02E5`.
- Game executable SHA-256: `61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B`.
- Diagnostics: enabled in the run configuration recorded by the main log.
- User observation: startup toast was visible before pressing Delete; it was
  visible for about 10 seconds and was already gone when the user opened the
  overlay.

## Log evidence

The overlay log records validated game Interface Language data, gate
eligibility, toast creation, draw-list submission, and TTL start at monotonic
`14907562`; expiration, display hide, and root detach occur at `14917562`,
exactly 10,000 ms later. Host-active, visible-HWND, and above-host state are
recorded during the toast interval.

The same log contains `OVERLAY_VISIBILITY=VISIBLE` and then
`OVERLAY_VISIBILITY=HIDDEN` before `queue_expire`. Those two visibility markers
have no timestamps. They show an open/close transition in the log ordering but
cannot establish precisely when the user pressed Delete or whether the toast
remained perceptible while the menu was open. The user observation and log
ordering are preserved separately rather than forcing a timing interpretation.

## Evidence files

This folder preserves unchanged copies of the three runtime logs. SHA-256:

| File | SHA-256 |
| --- | --- |
| `STALKER2CameraTweaks.log` | `D14EED40C4556B7681BF0EA51459943383F021BC18D877B0CE840A5FF519F06A` |
| `STALKER2CameraTweaksOverlay.log` | `01C2F82C4DC219BB10DEC1BB09152826CDED70E69411C00BC63D0F9A76918A3A` |
| `STALKER2CameraTweaksStartup.log` | `BC0B2D42C29DB788C6005A2D3536C809FEFBE0A6559015187D87B778E8318A41` |

## Evidence limits

This is positive live evidence that the toast was perceptible before the
manual menu action in this run. It does not establish repeatability across
launches, exact perceived duration, or continued toast visibility under the
open menu. No screenshot/video was supplied, and the logs' visibility markers
are not pixel evidence.
