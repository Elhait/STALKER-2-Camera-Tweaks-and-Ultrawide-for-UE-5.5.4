# Localization Catalog Contract

`en.json` is the canonical English catalog. The active set contains 18 catalogs: `en`, `uk`, `ar`, `cs`, `fr`, `de`, `it`, `ja`, `ko`, `pl`, `pt-BR`, `ru`, `sr-Cyrl`, `zh-Hans`, `es-419`, `es-ES`, `zh-Hant` and `tr`. Keep English key paths and placeholder names stable; run the catalog audit and deterministic tests after catalog changes.

Locale metadata is owned by `src/localization/locale_registry.hpp`; it provides each native display name, stable locale code, catalog filename, embedded resource identity, font profile and initial overlay font size. Selecting a locale manually, or synchronizing Auto to a different game locale, applies that registry-defined initial size; the user's slider adjustment is saved as the current `Overlay.FontSize`. Selecting Auto requests immediate synchronization through the game window procedure; Auto also synchronizes when the overlay changes from closed to open. A failed or unknown game-language read falls back to English without changing the configured Auto mode. Persist manual selections using the canonical code from the descriptor. Locale-code matching is case-insensitive, while persistence/UI readback use registry spelling. Regional/script variants remain separate identities: `pt-BR`, `sr-Cyrl`, `zh-Hans`, `es-419`, `es-ES` and `zh-Hant`. Serbian is specifically Cyrillic (`sr-Cyrl`). The JSON catalogs are build-time inputs embedded in the production overlay ASI; runtime does not load them from disk.

## Localized display text vs. canonical identifiers

Catalog entries provide localized user-facing prose, settings labels and status text. Canonical config values and selectable mode identifiers are not catalog display labels: keep names such as `HorPlus`, `AspectRecalculation`, `Auto`, `Native`, `GameplayHorPlus`, `NativeHorPlus`, `Adaptive`, `Reduced`, `Disabled`, and numeric aspect ratios unchanged in selectors, option tooltips and active-mode status. Descriptions, examples and explanatory context remain localizable.

Do not translate canonical mode/config identifiers or technical values. Keep the identifiers listed above unchanged wherever they are shown as identifiers. Their descriptions may be localized. Do not infer localizability from an English word alone: follow the value's semantic role, not whether an old catalog key once contained a translation.

## Translation guidance

- Translate the meaning of each complete key in its UI context; do not translate fragments independently when a key contains a sentence or placeholder template.
- Preserve every `{placeholder}` exactly, including spelling and case.
- English capitalizes `Gameplay` when referring to the named camera domain. Other locales should use natural grammar and capitalization for their language rather than copy English capitalization mechanically.
- Camera State is an advanced inspector. Translate its field labels, but preserve the technical meaning and distinctions of fields such as provenance, epoch, evidence validity, recovery exclusion, source, and event sequence.
- Preserve status certainty: do not turn “unavailable”, “cannot assess”, “may persist”, or “known paths are compatible” into stronger claims.
- Ukrainian terminology: translate the Gameplay domain naturally as “ігровий процес”, Cinematics as “катсцени”, and Dialogue as “діалоги”. Keep canonical mode identifiers (`HorPlus`, `AspectRecalculation`, `GameplayHorPlus`, `NativeHorPlus`), `FOV`, `ADS`, key names, numeric ratios, and config tokens unchanged.
- Keep Ukrainian phrasing concise enough for the fixed two-column overlay. Prefer natural wording over English sentence structure; the user reviews a complete catalog before renderer integration.
- For all locales, keep canonical identifiers, key names, placeholders and literal aspect ratios intact where they appear as technical values. Serbian text and glyph coverage use Cyrillic script.
- Catalog availability is separate from rendering correctness. UTF-8 and key/placeholder validation do not establish font coverage, CJK presentation, or Arabic shaping/bidi/RTL correctness; those require their own font/runtime evidence.
