# ProggyClean Cyrillic Companion — Research Pass

Date: 2026-09-24

## Result

The search did not find a verifiable ProggyClean fork with complete Ukrainian
glyph coverage. The strongest candidates for a controlled follow-up comparison
are **GohuFont Unicode 14** and **Terminus at its native 14/16 px sizes**. Neither
has yet been rendered beside ProggyClean through this project's ImGui path, so
neither is accepted as a visual match. The same-renderer specimen remains the
necessary next experiment.

No production source, configuration, font asset, or ASI was changed. No font was
installed and the game was not launched. Git was not inspected or modified.

## Existing baseline

The production font setup currently starts with ImGui's built-in default font
(ProggyClean) and merges Segoe UI for Cyrillic. The user's visual assessment is
English 10/10 and current Ukrainian 5/10; increasing size makes the mismatch
more noticeable. A previous bundled Noto Sans Mono font was also rejected
visually. These are user/runtime observations, not independent renderer
measurements.

## Candidate evidence

| Candidate | Ukrainian glyph evidence | Pixel/style evidence | License / integration notes | Assessment |
|---|---|---|---|---|
| **GohuFont Unicode 14** | Upstream Unicode BDF contains U+0406/U+0407, U+0404/U+0405, U+0454–U+0457, and U+0490/U+0491, covering `І Ї Є Ґ і ї є ґ`. | Hand-authored monospace bitmap font; official Unicode 14 BDF uses an 8-pixel cell, ascent 11 and descent 3. The official specimen looks crisp and pixel-oriented. | WTFPL v2. Canonical Unicode source is BDF; upstream describes contributed TTF conversions as unsupported. A direct ImGui TTF merge is therefore not established; conversion or bitmap-glyph atlas integration would need a separate technical decision. | **Best initial bitmap-style candidate**, contingent on a same-pipeline specimen and a safe, reproducible atlas path. |
| **Terminus** | Upstream documents broad language/codepage coverage including KOI8-U and Windows-1251. Exact eight Ukrainian code points were not verified against the selected upstream release file during this pass. | Native bitmap sizes include 8×14 and 8×16; Terminus TTF documentation recommends native integer pixel sizes and warns outline rasterization can look smeared. | OFL 1.1 for contemporary releases. The original bitmap is preferred where supported; ImGui's ordinary TTF path may not use embedded bitmap strikes as intended. | **Strong second candidate**; first verify exact glyphs and the actual ImGui raster result. |
| **Sergamon** | Official glyph grid explicitly includes `І Ї Є Ґ і ї є ґ`. | Hand-authored 8×16 grid, generated TTF, intended to be used at 16 px without antialiasing. Official live sample is chunky and visually heavier than ProggyClean. | SIL OFL 1.1. | Good coverage/control sample, but **unlikely as Cyrillic-only companion** because the style contrast is pronounced. |
| **Greybeard** | Upstream claims coverage of most Latin/Cyrillic, but exact Ukrainian-specific letters were not confirmed. | Bitmap/pixel monospaced variants around the desired sizes; preview is chunky/heavy. | MIT. | Low-priority control only; likely too heavy beside ProggyClean. |
| **512_8** | Upstream describes Latin/Greek/Cyrillic, but warns that some glyphs are approximations or indistinguishable. | Strict 8×8 grid has little vertical room for readable extended Cyrillic. | Public domain. | **Reject for this use**: the author's own caveat and limited grid make Ukrainian readability too risky. |

No credible ProggyClean Cyrillic expansion with verified Ukrainian coverage was
found in this search pass. This is a search result, not proof that none exists.

## Visual observations and limits

Official web specimens were inspected for GohuFont, Sergamon, Greybeard, and
Terminus. Sergamon and Greybeard looked notably heavier than ProggyClean. Gohu
looked more bitmap-oriented and is the most promising visual direction, but the
web previews differ in renderer, size, antialiasing, and context. They cannot
answer the central question: how the fallback Cyrillic looks merged into the
actual ProggyClean atlas in this overlay.

A controlled matrix has **not** been produced. The current workspace did not
have the previously checked Python/Pillow specimen route available, and the
official Gohu source is BDF rather than a supported upstream TTF. We should not
substitute browser previews or a different text renderer and call that an
equivalent comparison.

## Recommended next bounded experiment

Create a disposable, non-production ImGui specimen using the existing ImGui
version and the exact ProggyClean base/raster settings. Compare only Cyrillic
fallbacks, keeping sample text, pixel size, pixel snapping, oversampling,
spacing, and display scale identical. Include at least 13, 14, and 16 px and the
sample:

```text
Gameplay Mode / Режим ігрового процесу
Camera Transition / Перехід камери
Enabled / Увімкнено
Waiting / Очікування
HorPlus змінює FOV ігрового процесу
І Ї Є Ґ і ї є ґ 0123456789 : . , -
```

Prioritize Gohu Unicode 14 and Terminus 14/16. Include Sergamon only as a
control if the specimen can use its intended 16 px monochrome rendering. Do not
change production or generate a distributable ASI in this experiment.

## Primary sources

- [GohuFont official site and previews](https://font.gohu.org/)
- [GohuFont upstream repository and licensing](https://github.com/hchargois/gohufont)
- [GohuFont Unicode 14 BDF source](https://raw.githubusercontent.com/hchargois/gohufont/master/gohufont-uni-14.bdf)
- [Terminus upstream project](https://terminus-font.sourceforge.net/)
- [Terminus TTF documentation and license](https://files.ax86.net/terminus-ttf/)
- [Sergamon upstream project](https://github.com/sgmonda/sergamon)
- [Sergamon live specimen](https://sgmonda.com/sergamon/)
- [Greybeard upstream project and license](https://github.com/flowchartsman/greybeard)
- [512_8 upstream project](https://github.com/alexfru/512_8)

## Status

- Candidate and licensing survey: complete for this pass.
- Exact Ukrainian coverage: confirmed for Gohu and Sergamon; pending for the
  selected Terminus release and Greybeard.
- Same-ImGui visual comparison: not completed.
- Production integration/build/runtime validation: explicitly out of scope.
