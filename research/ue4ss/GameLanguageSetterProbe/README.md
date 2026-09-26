# Game Interface Language setter probe

This diagnostic-only UE4SS Lua mod tests whether the game's reflected
`UCppMediator::SetSelectedTextLanguage` function observes a live Interface
Language change and whether the confirmed UE getter changes in the setter's
pre/post window. It reads state only; it does not set language or touch ASI
production code.

Copy this folder into the game's `ue4ss/Mods` directory and add
`GameLanguageSetterProbe : 1` to `Mods/mods.txt`. The target UFunction must
already be registered when UE4SS starts the mod; the log explicitly reports
hook registration failure if it is unavailable then.

Minimal test:

1. F2 once in the initial UI to record the current UE language.
2. Change only Interface Language once and apply it. Do not change voicing.
   If the setter is called, its entry and exit records include the enum
   argument and current UE language.
3. Once the UI shows the new language, press F3 for one settled getter sample.

No language sweep, polling, restart or launcher information is needed. If the
setter does not fire, that rejects this setter as the menu-change signal for
this test path only; it does not invalidate the already-confirmed UE language
getter or the separate Unreal culture-change delegate candidate.

Evidence basis: the installed game dump declares the reflected setter with one
`ELocalizationLanguage` parameter; installed UE4SS exposes reflected UFunction
pre/post hooks and the prior getter probe established Lua access to
`GetCurrentLanguage()` with `FString:ToString()` conversion.
