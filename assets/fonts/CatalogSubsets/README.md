# Embedded catalog font subsets

These six static TrueType subsets are the complete supplemental font stack
for the 18 checked-in localization catalogs and their registry-defined display
names. `glyph-contract.json` records printable codepoints from both catalog
values and `LocaleDescriptor::displayName`, the registry-selected profile, the
exact subset cmap, and the codepoints already supplied by the ProggyClean/
ProggyVector base fonts. Offline tests check catalog strings in each active
profile and every locale name in its profile-specific selector face.

## Runtime profiles

All active profiles share the small Noto Sans Mono subset. Existing ImGui
ProggyClean and the embedded ProggyVector Cyrillic merge remain the base. Locale
metadata selects `base`, `arabic`, `japanese`, `korean`, `chinese-simplified`, or
`chinese-traditional`; a changed profile or Text Size rebuilds the active atlas.
The language selector adds separate small label faces containing only registry
display-name glyph ranges. CJK label faces remain regional and do not merge
another locale's catalog glyph set.

| Font/profile asset | Source binary | Cmap points | Embedded subset |
| --- | ---: | ---: | ---: |
| Noto Sans Mono / common | 596,428 B | 25 | 3,452 B |
| Noto Sans Arabic / arabic | 142,140 B TTF (18,777,381 B upstream package) | 43 | 5,256 B |
| Noto Sans CJK JP / japanese | 9,590,732 B | 288 | 68,600 B |
| Noto Sans CJK KR / korean | 10,415,420 B | 236 | 36,060 B |
| Noto Sans CJK SC / chinese-simplified | 17,773,132 B | 301 | 74,488 B |
| Noto Sans CJK TC / chinese-traditional | 11,942,800 B | 296 | 86,740 B |
| **Total embedded subsets** | — | **1,189** | **274,596 B** |

The locale catalogs contain 1,061 unique printable Unicode codepoints; adding
registry display names expands the full contract to 1,067 unique codepoints.
Regional CJK subsets intentionally retain duplicate codepoints with different
locale-specific outlines. The regenerated contract and offline tests establish
glyph availability only, not visual correctness or Arabic shaping/bidi/RTL.

## Offline atlas comparison

Measured with the repository's Dear ImGui v1.91.9b atlas builder and the
production font rasterization flags for every supported integer size. The
all-loaded case places all six complete profile fonts and selector labels in one
atlas; the active case rebuilds the catalog font by locale and retains only the
small per-script display-name faces needed by the language selector. Glyph count
includes every ImFont in the atlas. RGBA texture bytes are atlas width × height
× 4, matching the D3D12 backend.

| Text size | All-loaded atlas | All-loaded glyphs | RGBA bytes | Active profile + selector labels | Glyphs | RGBA bytes |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 12 | 512×1024 | 5,336 | 2,097,152 | 512×512 | 2,050 | 1,048,576 |
| 13 | 512×2048 | 7,343 | 4,194,304 | 512×512 | 2,295 | 1,048,576 |
| 14–16 | 1024×1024 | 7,343 | 4,194,304 | 512×512 | 2,007 | 1,048,576 |
| 17–19 | 1024×1024 | 7,343 | 4,194,304 | 512×1024 | 2,295 | 2,097,152 |
| 20–24 | 1024×2048 | 7,343 | 8,388,608 | 512×1024 | 2,007 | 2,097,152 |

The all-loaded atlas retains 2–8 MiB versus 1–2 MiB for one active profile plus
selector labels (2–4× less texture memory). This supports keeping the current
active-profile rebuild architecture. It also avoids regional glyph collisions:
each JP/KR/SC/TC selector label uses its matching face, while only the selected
locale's full script subset is merged into the active ImFont.

## Sources and license

- Noto Sans Mono v2.014, OFL 1.1. Source archive:
  <https://github.com/notofonts/latin-greek-cyrillic/releases/download/NotoSansMono-v2.014/NotoSansMono-v2.014.zip>.
  The source TTF is also retained at `../NotoSansMono/NotoSansMono-Regular.ttf`.
- Noto Sans Arabic v2.013 Regular, OFL 1.1. Upstream project and license:
  <https://github.com/notofonts/arabic/releases/download/NotoSansArabic-v2.013/NotoSansArabic-v2.013.zip>.
  The selected source is the unhinted
  Regular TTF (`NotoSansArabic/unhinted/ttf/NotoSansArabic-Regular.ttf`) from
  the v2.013 upstream package; the complete license is retained and embedded
  as `OFL-Arabic.txt`.
- Noto Sans CJK v2.004 JP/KR/SC/TC, SIL OFL 1.1. Sources are the four regional
  variable TTF subsets in the `Sans2.004` release at
  <https://github.com/notofonts/noto-cjk/tree/Sans2.004/Sans/Variable/TTF/Subset>.
  Each source is instantiated at `wght=400` before glyph subsetting. The
  applicable attribution is retained in `CJK-Copyright.txt`; the complete
  OFL 1.1 text is embedded in the ASI.
- Source and generated binary SHA-256 values are recorded in
  `glyph-contract.json` and pinned by `tools/fonts/rebuild_catalog_subsets.py`.

## Reproduction

1. Install `fonttools==4.66.0` in a Python environment (including its Brotli
   extra for the Arabic package).
2. Download the exact source versions above into one directory. Use the
   filenames `NotoSansMono-Regular.ttf`, `NotoSansArabic-v2.013.zip`,
   `NotoSansJP-VF.ttf`, `NotoSansKR-VF.ttf`, `NotoSansSC-VF.ttf`, and
   `NotoSansTC-VF.ttf`. Verify their SHA-256 values against the generator's
   pinned input hashes before proceeding.
3. Run `python tools/fonts/rebuild_catalog_subsets.py --source-dir <sources>
   --update-contract` from the repository root. The script derives display-name
   codepoints directly from `LocaleRegistry`, checks the locked catalog text,
   verifies source cmap coverage, instantiates each CJK source at Regular, and
   subsets the combined requirements recorded in `glyph-contract.json`. During
   routine regeneration, omit `--update-contract` to fail if registry metadata
   or catalogs have drifted from the checked-in contract.
4. Run `test.cmd` to verify the embedded subset cmap coverage and atlas build.

The production `.asi` build only reads the checked-in TTF subset assets and
license notices through `src/overlay/localization_resources.rc`. It does not
run this generator, access `%TEMP%`, read `Windows\\Fonts`, or query any host
font installation. Arabic is intentionally glyph-coverage-only; no shaping,
bidi, or RTL behavior is implemented or claimed here.
