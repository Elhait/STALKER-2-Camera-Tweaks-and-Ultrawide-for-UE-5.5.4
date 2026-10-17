param([string]$ObjectDirectory = 'build-artifacts/cm/g1e-display-candidate/production-tOFF-dOFF',
    [ValidateSet('all','current')][string]$Variant = 'all', [switch]$InitialShow)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output = Join-Path $repo 'build-artifacts/overlay-owned-hwnd-renderer-alpha'
$tree = [IO.Path]::GetFullPath((Join-Path $repo $ObjectDirectory))
$allowed = [IO.Path]::GetFullPath((Join-Path $repo 'build-artifacts')) + [IO.Path]::DirectorySeparatorChar
if (!$tree.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) { throw 'Objects must be under build-artifacts.' }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$registry = Get-Content (Join-Path $repo 'tools/build/production_sources.json') -Raw | ConvertFrom-Json
$objects = foreach ($source in $registry.sources) {
    if ($source.role -ne 'dll_entry' -and $source.path -ne 'src/overlay/composition/composition_runtime.cpp') {
        $object = Join-Path $tree ('CMakeFiles/STALKER2CameraTweaks.dir/' + $source.path + '.obj')
        if (!(Test-Path $object)) { throw "Missing object: $object" }
        '"' + $object + '"'
    }
}
$response = Join-Path $output 'compile.rsp'
$linkResponse = Join-Path $output 'link.rsp'
$exe = Join-Path $output 'renderer-alpha.exe'
$obj = Join-Path $output 'renderer-alpha.obj'
$source = Join-Path $repo 'research/prototypes/overlay_owned_hwnd/renderer_alpha_smoke.cpp'
$encoding = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllLines($response, @('/nologo','/c','/std:c++latest','/EHsc','/W4','/utf-8','/O1','/MT',
    '/DNDEBUG','/DOVERLAY_COMBINED','/DOVERLAY_PRODUCTION','/DOVERLAY_SETTINGS_FRONTEND','/DOVERLAY_STARTUP_JOURNAL',
    ('/I"' + $repo + '"'), ('/I"' + (Join-Path $repo 'external/imgui') + '"'),
    ('/Fo"' + $obj + '"'), ('"' + $source + '"')), $encoding)
[IO.File]::WriteAllLines($linkResponse, @('/NOLOGO','/MANIFEST:NO',('/OUT:"' + $exe + '"'),('"' + $obj + '"')) +
    $objects + @(('"' + (Join-Path $tree 'resources/localization_resources.res') + '"'),
    'user32.lib','gdi32.lib','ole32.lib','psapi.lib','bcrypt.lib','dxgi.lib','d3d11.lib','dcomp.lib','d3dcompiler.lib','dwmapi.lib'), $encoding)
& (Join-Path $PSScriptRoot 'compile_overlay_attachment_fixture.cmd') $response $linkResponse 2>&1 |
    Tee-Object -FilePath (Join-Path $output 'build.log')
if ($LASTEXITCODE -ne 0) { throw "Actual Renderer fixture build failed: $LASTEXITCODE" }
Push-Location $output
try {
    $variants = if ($Variant -eq 'current') { @('current') } else { @('legacy-alpha','legacy-no-redirection','current') }
    foreach ($variant in $variants) {
        $arguments = @($variant)
        if ($InitialShow) { $arguments += 'initial-show' }
        $logName = if ($InitialShow) { $variant + '-initial-show.log' } else { $variant + '.log' }
        & $exe @arguments 2>&1 | Tee-Object -FilePath (Join-Path $output $logName)
        $variantExit = $LASTEXITCODE
        Write-Output "variant=$variant exit=$variantExit"
        if ($variant -eq 'current' -and $variantExit -ne 0) { throw "Current renderer alpha validation failed: $variantExit" }
    }
} finally { Pop-Location }
