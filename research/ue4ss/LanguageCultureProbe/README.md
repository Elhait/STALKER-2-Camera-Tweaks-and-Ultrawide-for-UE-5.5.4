# Language/culture runtime probe

This temporary UE4SS Lua mod samples Unreal's reflected
`UKismetInternationalizationLibrary` getters. It does not change game settings,
write UObject properties, query launcher state, or modify the production ASI.

Copy this folder into the game's `ue4ss/Mods` directory and add
`LanguageCultureProbe : 1` to that UE4SS installation's `Mods/mods.txt`.
Remove the copied folder and that one entry after the test.

With the game's existing language unchanged, use:

1. F2 once the initial game UI is visible.
2. Change only Interface Language, apply it, and use F3 once the UI is usable.
   Leave Voicing Language unchanged.
3. If the game requests a restart, restart and use F4 once the UI is visible.

The three records are printed to the UE4SS log/console with explicit stage
labels. Each record includes the result or a per-call failure for
`GetCurrentLanguage`, `GetCurrentLocale`, and `GetCurrentCulture`. F2/F3/F4
are manual snapshots, not polling; do not interpret a missing record as a
language value. Capture the corresponding visible UI language and whether the
game requested a restart alongside the log.

The returned UE `FString` userdata must be converted with UE4SS's
`:ToString()` method before logging; plain Lua `tostring()` prints only its
wrapper type/address.

Static basis: the installed UE4SS dump declares all three functions on
`UKismetInternationalizationLibrary`; the generated UE4SS SDK exposes matching
static wrappers; installed Lua probes use `StaticFindObject`, `RegisterKeyBind`,
and `ExecuteInGameThread`. The first live run must still confirm that Lua
reflection dispatches these getters successfully in this game build.
