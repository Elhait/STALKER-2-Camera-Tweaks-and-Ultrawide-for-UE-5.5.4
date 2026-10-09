$ErrorActionPreference = 'Stop'

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$catalogPath = Join-Path $projectRoot 'locales\en.json'
$outputPath = Join-Path $projectRoot 'build-artifacts\generated\canonical_ini_documentation.hpp'
$catalog = Get-Content -LiteralPath $catalogPath -Raw -Encoding UTF8 | ConvertFrom-Json
$keys = @(
    'common.examples',
    'tooltip.cinematics.enabled',
    'tooltip.gameplay.enabled', 'tooltip.gameplay.disabled',
    'tooltip.gameplay.horplus', 'tooltip.gameplay.horplus_examples_context',
    'tooltip.gameplay.horplus_example_90', 'tooltip.gameplay.horplus_example_100',
    'tooltip.gameplay.horplus_example_110', 'tooltip.gameplay.aspect_recalculation',
    'tooltip.gameplay.runtime_combine',
    'tooltip.cinematics.aspect.auto', 'tooltip.cinematics.aspect.native',
    'tooltip.cinematics.aspect.16_9', 'tooltip.cinematics.aspect.21_9',
    'tooltip.cinematics.aspect.32_9', 'tooltip.cinematics.aspect.forced_example',
    'tooltip.cinematics.fov.gameplay',
    'tooltip.cinematics.fov.gameplay_examples_context',
    'tooltip.cinematics.fov.gameplay_example_90',
    'tooltip.cinematics.fov.gameplay_example_112_6',
    'tooltip.cinematics.fov.native',
    'tooltip.cinematics.fov.native_examples_context',
    'tooltip.cinematics.fov.native_example_90',
    'tooltip.dialogue.zoom.examples_context',
    'tooltip.dialogue.zoom.adaptive', 'tooltip.dialogue.zoom.adaptive_example_90',
    'tooltip.dialogue.zoom.adaptive_example_110', 'tooltip.dialogue.zoom.reduced',
    'tooltip.dialogue.zoom.reduced_example_90', 'tooltip.dialogue.zoom.reduced_example_110',
    'tooltip.dialogue.zoom.disabled', 'tooltip.dialogue.zoom.disabled_example_90',
    'tooltip.dialogue.zoom.disabled_example_110',
    'tooltip.hotkeys.enabled', 'tooltip.hotkeys.disabled',
    'tooltip.hotkeys.single_key', 'tooltip.hotkeys.rebind',
    'tooltip.hotkeys.gameplay_cycle', 'tooltip.hotkeys.cinematic_aspect_cycle',
    'tooltip.hotkeys.cinematic_fov_cycle',
    'tooltip.overlay.always_active'
)

function Get-Value($root, [string] $path) {
    $node = $root
    foreach ($part in $path.Split('.')) { $node = $node.$part }
    if ($node -isnot [string] -or [string]::IsNullOrWhiteSpace($node)) {
        throw "Canonical English documentation key is missing or empty: $path"
    }
    return $node
}

function ConvertTo-CppString([string] $value) {
    return '"' + $value.Replace('\', '\\').Replace('"', '\"') + '"'
}

$values = @{}
foreach ($key in $keys) { $values[$key] = Get-Value $catalog $key }

$comments = [ordered]@{}
$comments.GameplayEnabled = [string[]]@(
        "# ------------------------------------------------------------------------------",
        "# Controls camera correction during normal gameplay.",
        "# ------------------------------------------------------------------------------",
        "# Enable Gameplay camera correction.",
        "# Default: true",
        "# true - $($values['tooltip.gameplay.enabled'])",
        "# false - $($values['tooltip.gameplay.disabled'])")
$comments.GameplayMode = [string[]]@(
        "# Available modes:",
        "#   AspectRecalculation - $($values['tooltip.gameplay.aspect_recalculation'])",
        "#   HorPlus - $($values['tooltip.gameplay.horplus'])",
        "# $($values['common.examples']) - $($values['tooltip.gameplay.horplus_examples_context']):",
        "# $($values['tooltip.gameplay.horplus_example_90'])",
        "# $($values['tooltip.gameplay.horplus_example_100'])",
        "# $($values['tooltip.gameplay.horplus_example_110'])",
        "#   RuntimeCombine (Recommended) - $($values['tooltip.gameplay.runtime_combine'])",
        "# Default: RuntimeCombine")
$comments.CinematicAspect = [string[]]@(
        "# Available modes:",
        "#   Auto - $($values['tooltip.cinematics.aspect.auto'])",
        "#   16:9 - $($values['tooltip.cinematics.aspect.16_9'])",
        "#   21:9 - $($values['tooltip.cinematics.aspect.21_9'])",
        "#   32:9 - $($values['tooltip.cinematics.aspect.32_9'])",
        "# Example: $($values['tooltip.cinematics.aspect.forced_example'])",
        "# Default: Auto")
$comments.CinematicEnabled = [string[]]@(
        "# ------------------------------------------------------------------------------",
        "# Controls cinematic framing and FOV correction.",
        "# ------------------------------------------------------------------------------",
        "# Master switch for cinematic camera corrections.",
        "# Default: true",
        "# true - $($values['tooltip.cinematics.enabled'])",
        "# false - preserve native cinematic camera output.")
$comments.CinematicFov = [string[]]@(
        "# FOV correction mode:",
        "#   GameplayHorPlus - $($values['tooltip.cinematics.fov.gameplay'])",
        "# $($values['common.examples']) - $($values['tooltip.cinematics.fov.gameplay_examples_context']):",
        "# $($values['tooltip.cinematics.fov.gameplay_example_90'])",
        "# $($values['tooltip.cinematics.fov.gameplay_example_112_6'])",
        "#   NativeHorPlus - $($values['tooltip.cinematics.fov.native'])",
        "# $($values['common.examples']) - $($values['tooltip.cinematics.fov.native_examples_context']):",
        "# $($values['tooltip.cinematics.fov.native_example_90'])",
        "# Default: GameplayHorPlus")
$comments.DialogueEnabled = [string[]]@(
        "# ------------------------------------------------------------------------------",
        "# Controls Dialogue camera zoom correction.",
        "# ------------------------------------------------------------------------------",
        "# Master switch for Dialogue correction.",
        "# Default: true",
        "# true - apply the selected mode after an accepted Dialogue start.",
        "# false - preserve native camera output while observing Dialogue lifecycle.")
$comments.DialogueZoom = [string[]]@(
        "# Available modes:",
        "#   Adaptive - $($values['tooltip.dialogue.zoom.adaptive'])",
        "# $($values['common.examples']) - $($values['tooltip.dialogue.zoom.examples_context']):",
        "# $($values['tooltip.dialogue.zoom.adaptive_example_90'])",
        "# $($values['tooltip.dialogue.zoom.adaptive_example_110'])",
        "#   Reduced - $($values['tooltip.dialogue.zoom.reduced'])",
        "# $($values['tooltip.dialogue.zoom.reduced_example_90'])",
        "# $($values['tooltip.dialogue.zoom.reduced_example_110'])",
        "#   Disabled - $($values['tooltip.dialogue.zoom.disabled'])",
        "# $($values['tooltip.dialogue.zoom.disabled_example_90'])",
        "# $($values['tooltip.dialogue.zoom.disabled_example_110'])",
        "# Default: Adaptive")
$comments.HotkeysEnabled = [string[]]@(
        "# ------------------------------------------------------------------------------",
        "# Optional keyboard shortcuts for changing settings while playing.",
        "# ------------------------------------------------------------------------------",
        "# Enable or disable Gameplay, Cinematic and Dialogue hotkey actions.",
        "# Default: false",
        "# true - $($values['tooltip.hotkeys.enabled'])",
        "# false - $($values['tooltip.hotkeys.disabled'])")
$comments.HotkeyGameplayCycle = [string[]]@(
        "# $($values['tooltip.hotkeys.gameplay_cycle'])",
        "# $($values['tooltip.hotkeys.rebind'])",
        "# $($values['tooltip.hotkeys.single_key'])",
        "# Default: F9")
$comments.HotkeyCinematicAspectCycle = [string[]]@(
        "# $($values['tooltip.hotkeys.cinematic_aspect_cycle'])",
        "# $($values['tooltip.hotkeys.rebind'])",
        "# $($values['tooltip.hotkeys.single_key'])",
        "# Default: F10")
$comments.HotkeyCinematicFovCycle = [string[]]@(
        "# $($values['tooltip.hotkeys.cinematic_fov_cycle'])",
        "# $($values['tooltip.hotkeys.rebind'])",
        "# $($values['tooltip.hotkeys.single_key'])",
        "# Default: F11")
$comments.HotkeyDialogueCycle = [string[]]@(
        "# Cycles selectable Dialogue modes: Adaptive -> Reduced -> Disabled -> Adaptive.",
        "# $($values['tooltip.hotkeys.rebind'])",
        "# $($values['tooltip.hotkeys.single_key'])",
        "# Default: F12")
$comments.OverlayToggle = [string[]]@(
        "# Use this key to open or close the overlay. Independent of Hotkeys.Enabled.",
        "# $($values['tooltip.hotkeys.rebind'])",
        "# $($values['tooltip.hotkeys.single_key'])",
        "# Default: Delete")

$entryLines = foreach ($key in $keys) {
    "        Entry{$(ConvertTo-CppString $key), $(ConvertTo-CppString $values[$key])}"
}
$arrayLines = foreach ($name in $comments.Keys) {
    $items = @()
    foreach ($item in $comments[$name]) {
        $items += "            $(ConvertTo-CppString ([string]$item))"
    }
    "    inline constexpr std::string_view ${name}Comments[]{`n$($items -join ",`n")`n    };"
}
$source = @"
#pragma once

#include <string_view>

namespace config::canonical_ini_documentation
{
    struct Entry { std::string_view key; std::string_view value; };
    inline constexpr Entry Entries[]{
$($entryLines -join ",`n")
    };
$($arrayLines -join "`n")

    inline std::string_view Find(std::string_view key) noexcept
    {
        for (const auto& entry : Entries)
            if (entry.key == key) return entry.value;
        return {};
    }
}
"@

$directory = Split-Path -Parent $outputPath
[System.IO.Directory]::CreateDirectory($directory) | Out-Null
[System.IO.File]::WriteAllText($outputPath, $source, [System.Text.UTF8Encoding]::new($false))
