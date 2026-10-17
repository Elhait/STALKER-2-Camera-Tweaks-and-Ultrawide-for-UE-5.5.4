param([string]$ExperimentDirectory = 'build-artifacts/fp', [switch]$Production, [switch]$TargetRelease, [switch]$Timing)
$ErrorActionPreference = 'Stop'
if ($Production -and $TargetRelease) { throw 'Select production or target-release snapshot, not both.' }
if ($Timing -and !$TargetRelease) { throw 'Timing fixture requires target-release snapshot.' }
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output = [IO.Path]::GetFullPath((Join-Path $repo $ExperimentDirectory))
$allowed = [IO.Path]::GetFullPath((Join-Path $repo 'build-artifacts')) + [IO.Path]::DirectorySeparatorChar
if (!$output.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture output must be under build-artifacts.' }
$source = Join-Path $output 'source'
if ($Production) { $source = $repo }
$registry = Get-Content -LiteralPath (Join-Path $source 'tools/build/production_sources.json') -Raw | ConvertFrom-Json
$encoding = [Text.UTF8Encoding]::new($false)
foreach ($variant in $(if ($Production) { @('production') } elseif ($TargetRelease) { @('E') } else { @('B','C') })) {
    $tree = Join-Path $output "compile-$variant/production-tOFF-dOFF"
    if ($Production) { $tree = Join-Path $output 'cmake/production-tOFF-dOFF' }
    $objects = foreach ($entry in $registry.sources) {
        if ($entry.role -ne 'dll_entry') {
            $obj = Join-Path $tree ("CMakeFiles/STALKER2CameraTweaks.dir/" + $entry.path + '.obj')
            if (!(Test-Path -LiteralPath $obj)) { throw "Missing validated experiment object: $obj" }
            '"' + $obj + '"'
        }
    }
    $fixture = Join-Path $output "renderer-$variant.exe"
    $response = Join-Path $output "renderer-$variant.rsp"
    $linkResponse = Join-Path $output "renderer-$variant-link.rsp"
    $detach = if ($variant -eq 'C') { '1' } else { '0' }
    $lines = @('/nologo','/c','/std:c++latest','/EHsc','/W4','/utf-8','/O1','/MT',
        "/DOVERLAY_ATTACHMENT_EXPERIMENT_DETACH=$detach",'/DOVERLAY_PRODUCTION','/DOVERLAY_SETTINGS_FRONTEND','/DOVERLAY_COMBINED',
        ('/I"' + $source + '"'), ('/I"' + (Join-Path $source 'external/imgui') + '"'),
        ('/Fo"' + (Join-Path $output "renderer-$variant.obj") + '"'),
        ('"' + (Join-Path $repo 'research/prototypes/overlay_attachment/renderer_smoke.cpp') + '"'))
    if ($Production -or $TargetRelease) { $lines += '/DOVERLAY_ATTACHMENT_PRODUCTION_SMOKE' }
    if ($TargetRelease) { $lines += '/DOVERLAY_TARGET_RELEASE_SMOKE' }
    if ($Timing) { $lines += '/DOVERLAY_TARGET_TIMING_SMOKE' }
    [IO.File]::WriteAllText($response, ($lines -join "`r`n"), $encoding)
    $linkLines = @('/NOLOGO','/MANIFEST:NO', ('/OUT:"' + $fixture + '"'), ('"' + (Join-Path $output "renderer-$variant.obj") + '"')) + $objects
    $linkLines += @(('"' + (Join-Path $tree 'resources/localization_resources.res') + '"'),
        'user32.lib','psapi.lib','bcrypt.lib','dxgi.lib','d3d11.lib','dcomp.lib','d3dcompiler.lib')
    [IO.File]::WriteAllText($linkResponse, ($linkLines -join "`r`n"), $encoding)
    & (Join-Path $PSScriptRoot 'compile_overlay_attachment_fixture.cmd') $response $linkResponse 2>&1 | Tee-Object -FilePath (Join-Path $output "fixture-build-$variant.log")
    if ($LASTEXITCODE -ne 0) { throw "Fixture $variant compiler/linker failed: $LASTEXITCODE" }
    Push-Location -LiteralPath $output
    try {
        & $fixture 2>&1 | Tee-Object -FilePath (Join-Path $output "fixture-run-$variant.log")
        if ($LASTEXITCODE -ne 0) { throw "Fixture $variant failed: $LASTEXITCODE" }
    } finally { Pop-Location }
}
