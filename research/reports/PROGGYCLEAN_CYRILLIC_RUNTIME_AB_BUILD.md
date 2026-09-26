# ProggyClean Cyrillic Companion — Runtime A/B Build

Date: 2026-09-24

## Outcome

Built two isolated experimental overlay ASIs for user-run visual comparison.
The only build-time difference between them is the embedded Cyrillic TTF
resource. Production font selection and the root production ASI were not
changed or replaced.

| Variant | ASI | Size | SHA-256 |
|---|---|---:|---|
| ProggyClean + GohuFont Unicode 14 TTF conversion | `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayIntegrationGohu.asi` | 1,880,576 bytes | `5F0AB539C6CB2B117A7F3BEF311C0DD0000D31760FCF07E27C58DF86B3770133` |
| ProggyClean + Terminus TTF Windows 4.49.1 | `build-artifacts/experimental-font-ab/STALKER2CameraTweaksOverlayIntegrationTerminus.asi` | 2,224,640 bytes | `C71A6CD1E8ACF2796795DB0BE2092E3F042B6DF5E3551658140F8E4ED7C79B8F` |

Both artifacts contain embedded English catalog resource 101 (10,325 bytes),
Ukrainian catalog resource 102 (16,637 bytes), and Cyrillic font resource 103.
The embedded font resource sizes are 120,776 bytes (Gohu) and 464,868 bytes
(Terminus), matching their input files.

## Controlled differences and invariants

- Same source revision, ImGui build, overlay/runtime/localization sources,
  catalog resources, compiler/linker flags, and output structure.
- Both use unmodified `AddFontDefault()` for ProggyClean Latin, keeping the
  same 13 px built-in base and raster settings.
- Both candidate TTFs are loaded through ImGui's same `AddFontFromMemoryTTF`
  path at 14 px, `MergeMode=true`, `PixelSnapH=true`, oversampling H/V=1, and
  y-offset 1 px. The candidate range is only U+0400–U+052F; it does not replace
  ASCII/Latin. Existing Segoe UI fallback remains limited to the same general
  punctuation/arrow ranges in both variants.
- The existing persisted font-size setting/scaling behavior is unchanged; no
  `FontGlobalScale` tuning was performed. At the default setting, global scale
  remains 1.0.
- PE resource inspection was performed with the images mapped as data only; no
  ASI code was executed.

The TTF cmap audit found all 74 required code points in both files: `А-Я`,
`а-я`, `Ё/ё`, and the Ukrainian-specific `І/і`, `Ї/ї`, `Є/є`, `Ґ/ґ`.

## Provenance and license

- Gohu input: `build-artifacts/experimental-font-ab/inputs/gohufont-uni-14.ttf`,
  SHA-256 `E24C3974EC8C4D3697DAFCC5AF510A43D2825A7C7251EC0464A7B98F538BE0D4`.
  [Upstream conversion repository](https://github.com/koemaeda/gohufont-ttf),
  [GohuFont official page](https://font.gohu.org/). The contributor repository
  identifies the TTF as automatically traced and WTFPL-licensed; GohuFont's
  official page marks contributed TTF conversions unsupported. This test
  therefore evaluates this exact conversion through ImGui's outline rasterizer,
  not the native Gohu BDF bitmap appearance.
- Terminus input:
  `build-artifacts/experimental-font-ab/inputs/TerminusTTFWindows-4.49.1.ttf`,
  SHA-256 `E422E88D30B12D3968871534C9BD85F266683F0318EB241A3F1E00D3D02322B3`.
  [Official Terminus TTF documentation/downloads](https://files.ax86.net/terminus-ttf/).
  The bundled `COPYING` file identifies SIL OFL 1.1. Upstream notes outline
  rendering may differ from original bitmap strikes and can look smeared under
  antialiasing; the experiment intentionally does not add candidate-specific
  tuning.

## Validation and limits

- `test.cmd`: PASS; 40/40 runner sources compiled and executed, all deterministic
  harnesses passed. Existing Zydis/ImGui vendor warnings only.
- Localization catalog audit in the isolated build script: PASS, 166/166 keys,
  placeholders and embedded-catalog contract passed.
- Both experimental ASI builds: PASS. Resource inspection confirms both locale
  catalogs and the correct candidate font are embedded in each artifact.
- No game launch or injection. The actual on-screen visual comparison is not
  agent-validated; the user should run each ASI separately and compare the same
  overlay state/settings.
- Git was not inspected or changed, per the user's explicit instruction.

## Artifacts

The two ASIs, isolated build script/sources/resource scripts, font inputs, and
license notices are under `build-artifacts/experimental-font-ab/`. The input
fonts are test-only, are not installed, and are not intended as mod sidecars.
