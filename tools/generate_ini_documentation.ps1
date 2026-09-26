$ErrorActionPreference = 'Stop'

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$catalogPath = Join-Path $projectRoot 'locales\en.json'
$outputPath = Join-Path $projectRoot 'build-artifacts\generated\canonical_ini_documentation.hpp'
$catalog = Get-Content -LiteralPath $catalogPath -Raw -Encoding UTF8 | ConvertFrom-Json
$keys = @(
    'common.examples',
    'tooltip.gameplay.enabled', 'tooltip.gameplay.disabled',
    'tooltip.gameplay.horplus', 'tooltip.gameplay.horplus_examples_context',
    'tooltip.gameplay.horplus_example_90', 'tooltip.gameplay.horplus_example_100',
    'tooltip.gameplay.horplus_example_110', 'tooltip.gameplay.aspect_recalculation',
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
    'tooltip.dialogue.zoom.native',
    'tooltip.dialogue.zoom.examples_context',
    'tooltip.dialogue.zoom.native_example_90', 'tooltip.dialogue.zoom.native_example_110',
    'tooltip.dialogue.zoom.adaptive', 'tooltip.dialogue.zoom.adaptive_example_90',
    'tooltip.dialogue.zoom.adaptive_example_110', 'tooltip.dialogue.zoom.reduced',
    'tooltip.dialogue.zoom.reduced_example_90', 'tooltip.dialogue.zoom.reduced_example_110',
    'tooltip.dialogue.zoom.disabled', 'tooltip.dialogue.zoom.disabled_example_90',
    'tooltip.dialogue.zoom.disabled_example_110',
    'tooltip.hotkeys.enabled', 'tooltip.hotkeys.disabled',
    'tooltip.hotkeys.single_key', 'tooltip.hotkeys.rebind',
    'tooltip.hotkeys.gameplay_cycle', 'tooltip.hotkeys.cinematic_aspect_cycle',
    'tooltip.hotkeys.cinematic_fov_cycle', 'tooltip.hotkeys.dialogue_cycle',
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
        "; true - $($values['tooltip.gameplay.enabled'])",
        "; false - $($values['tooltip.gameplay.disabled'])")
$comments.GameplayMode = [string[]]@(
        "; HorPlus - $($values['tooltip.gameplay.horplus'])", ';',
        "; $($values['common.examples']) - $($values['tooltip.gameplay.horplus_examples_context']):",
        "; $($values['tooltip.gameplay.horplus_example_90'])",
        "; $($values['tooltip.gameplay.horplus_example_100'])",
        "; $($values['tooltip.gameplay.horplus_example_110'])", ';',
        "; AspectRecalculation - $($values['tooltip.gameplay.aspect_recalculation'])")
$comments.CinematicAspect = [string[]]@(
        "; Auto - $($values['tooltip.cinematics.aspect.auto'])",
        "; Native - $($values['tooltip.cinematics.aspect.native'])",
        "; 16:9 - $($values['tooltip.cinematics.aspect.16_9'])",
        "; 21:9 - $($values['tooltip.cinematics.aspect.21_9'])",
        "; 32:9 - $($values['tooltip.cinematics.aspect.32_9'])",
        "; $($values['tooltip.cinematics.aspect.forced_example'])")
$comments.CinematicFov = [string[]]@(
        "; GameplayHorPlus - $($values['tooltip.cinematics.fov.gameplay'])", ';',
        "; $($values['common.examples']) - $($values['tooltip.cinematics.fov.gameplay_examples_context']):",
        "; $($values['tooltip.cinematics.fov.gameplay_example_90'])",
        "; $($values['tooltip.cinematics.fov.gameplay_example_112_6'])", ';',
        "; NativeHorPlus - $($values['tooltip.cinematics.fov.native'])", ';',
        "; $($values['common.examples']) - $($values['tooltip.cinematics.fov.native_examples_context']):",
        "; $($values['tooltip.cinematics.fov.native_example_90'])")
$comments.DialogueZoom = [string[]]@(
        "; Native - $($values['tooltip.dialogue.zoom.native'])",
        "; $($values['common.examples']) - $($values['tooltip.dialogue.zoom.examples_context']):",
        "; $($values['tooltip.dialogue.zoom.native_example_90'])",
        "; $($values['tooltip.dialogue.zoom.native_example_110'])", ';',
        "; Adaptive - $($values['tooltip.dialogue.zoom.adaptive'])",
        "; $($values['common.examples']) - $($values['tooltip.dialogue.zoom.examples_context']):",
        "; $($values['tooltip.dialogue.zoom.adaptive_example_90'])",
        "; $($values['tooltip.dialogue.zoom.adaptive_example_110'])", ';',
        "; Reduced - $($values['tooltip.dialogue.zoom.reduced'])",
        "; $($values['common.examples']) - $($values['tooltip.dialogue.zoom.examples_context']):",
        "; $($values['tooltip.dialogue.zoom.reduced_example_90'])",
        "; $($values['tooltip.dialogue.zoom.reduced_example_110'])", ';',
        "; Disabled - $($values['tooltip.dialogue.zoom.disabled'])",
        "; $($values['common.examples']) - $($values['tooltip.dialogue.zoom.examples_context']):",
        "; $($values['tooltip.dialogue.zoom.disabled_example_90'])",
        "; $($values['tooltip.dialogue.zoom.disabled_example_110'])")
$comments.HotkeysEnabled = [string[]]@(
        "; true - $($values['tooltip.hotkeys.enabled'])",
        "; false - $($values['tooltip.hotkeys.disabled'])")
$comments.HotkeyGameplayCycle = [string[]]@(
        "; $($values['tooltip.hotkeys.gameplay_cycle'])",
        "; $($values['tooltip.hotkeys.rebind'])",
        "; $($values['tooltip.hotkeys.single_key'])")
$comments.HotkeyCinematicAspectCycle = [string[]]@(
        "; $($values['tooltip.hotkeys.cinematic_aspect_cycle'])",
        "; $($values['tooltip.hotkeys.rebind'])",
        "; $($values['tooltip.hotkeys.single_key'])")
$comments.HotkeyCinematicFovCycle = [string[]]@(
        "; $($values['tooltip.hotkeys.cinematic_fov_cycle'])",
        "; $($values['tooltip.hotkeys.rebind'])",
        "; $($values['tooltip.hotkeys.single_key'])")
$comments.HotkeyDialogueCycle = [string[]]@(
        "; $($values['tooltip.hotkeys.dialogue_cycle'])",
        "; $($values['tooltip.hotkeys.rebind'])",
        "; $($values['tooltip.hotkeys.single_key'])")
$comments.OverlayToggle = [string[]]@(
        "; $($values['tooltip.overlay.always_active'])",
        "; $($values['tooltip.hotkeys.rebind'])",
        "; $($values['tooltip.hotkeys.single_key'])")

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
