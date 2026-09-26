# Catalog font fallback

The production overlay embeds the small catalog-derived subset at
`../CatalogSubsets/NotoSansMono-Catalog.ttf`; it no longer loads this full
font file at runtime. The full Noto Sans Mono v2.014 source binary is retained
here as the reproducible subsetting source reference. Its OFL 1.1 license is
in `OFL.txt`.

The production common subset covers the catalog-required Latin Extended /
Turkish characters and three punctuation/symbol codepoints not supplied by
the overlay's existing Proggy fonts. Its exact Unicode contract, size, hashes,
and regeneration process are recorded in `../CatalogSubsets/`.
