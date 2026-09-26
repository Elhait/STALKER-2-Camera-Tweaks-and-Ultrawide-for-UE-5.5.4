# Overlay Focus Flicker — Result

## Runtime evidence

- With the overlay hidden, the user observed the game window flicker while
  switching the active window.
- With the overlay visible, the same flicker was observed. Timestamped logs
  recorded focus loss/regain and successful swapchain resize/resource recovery.
- The user repeated the active-window switch with the mod absent and observed
  the same flicker.

## Conclusion

For the tested game, display mode, and system configuration, the flicker does
not require `STALKER2CameraTweaks` or overlay visibility. The evidence supports
classifying it as behavior of the tested game/presentation environment, not a
mod regression. No overlay-side workaround is warranted by this result.

This is a scoped runtime observation, not a claim about every system or display
configuration. Overlay swapchain resize/rebuild remains required and was
observed to recover successfully in the instrumented run.

## Disposition

- Focus/Alt+Tab diagnostic telemetry: removed after the A/B and no-mod control.
- Functional window procedure, input/cursor handling, and resize/resource
  lifecycle: unchanged.
- Further investigation or runtime testing: not planned unless the behavior
  changes or new contradictory evidence appears.
