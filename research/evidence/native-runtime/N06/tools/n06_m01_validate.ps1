param(
    [string]$GameExe='E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe',
    [switch]$ReuseDependencies
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../..'))
$out = Join-Path $repo 'build-artifacts/native-runtime/N06-M01-2026-10-10'
$support = Join-Path $out 'supporting'
New-Item -ItemType Directory -Force -Path $out,$support | Out-Null
Set-Location -LiteralPath $repo

$expectedGameHash = '61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B'
$gameHash = (Get-FileHash -LiteralPath $GameExe -Algorithm SHA256).Hash
if ($gameHash -ne $expectedGameHash) { throw "Game identity mismatch: $gameHash" }
$file = [IO.File]::ReadAllBytes($GameExe)
function Assert-ImageBytes([UInt64]$Rva, [byte[]]$Expected, [string]$Name) {
    $offset = [int]($Rva - 0xC00) # .text RVA 0x1000 maps to file offset 0x400
    if ($offset -lt 0 -or $offset + $Expected.Length -gt $file.Length) { throw "$Name site is outside the image" }
    for ($i=0; $i -lt $Expected.Length; $i++) {
        if ($file[$offset + $i] -ne $Expected[$i]) { throw "$Name image byte mismatch at RVA 0x$('{0:X}' -f ($Rva + $i))" }
    }
    $hexBytes = [BitConverter]::ToString($Expected).Replace('-','')
    "$Name rva=0x$('{0:X}' -f $Rva) fileOffset=0x$('{0:X}' -f $offset) bytes=$hexBytes" |
        Add-Content -LiteralPath (Join-Path $out 'game-derived-bytes.txt')
}
Remove-Item -LiteralPath (Join-Path $out 'game-derived-bytes.txt') -ErrorAction SilentlyContinue
Assert-ImageBytes 0x602A6C ([byte[]](0xF3,0x0F,0x10,0x40,0x4C,0xF3,0x0F,0x10,0x8E,0x38,0x01,0,0,0x0F,0x2E,0xC8,0x0F,0x87,0xC7,0x01,0,0)) 'ZoomIN'
Assert-ImageBytes 0x602C12 ([byte[]](0xF3,0x0F,0x10,0x40,0x50,0xF3,0x0F,0x10,0x8E,0x3C,0x01,0,0,0x0F,0x2E,0xC8,0x77,0x15)) 'ZoomOUT'

$fixtureFile = 'tests/plugin/runtime_native_evidence_cases.hpp'
$fixtureHash = (Get-FileHash -LiteralPath $fixtureFile -Algorithm SHA256).Hash
if ($fixtureHash -ne '2711038F8EB65294EB4EEC5D67551D85470F3CC8D7CD5FF9FE7448C4FDE7D4C2') { throw "Historical Zoom/native fixture changed: $fixtureHash" }
$protected = @('src','external','release-assets') | ForEach-Object {
    Get-ChildItem -LiteralPath $_ -File -Recurse
}
$protected += Get-ChildItem -LiteralPath . -File | Where-Object { $_.Extension -in '.asi','.zip' }
$before = @($protected | Sort-Object FullName -Unique | ForEach-Object {
    [pscustomobject]@{Path=$_.FullName.Substring($repo.Length+1);SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
})
$before | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $support 'protected-before.csv')

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/runner/runtime_native_adapter_inputs.ps1 *> (Join-Path $out 'source-inputs.log')
if ($LASTEXITCODE -ne 0) { throw 'Could not generate the existing actual production/vendor dependency list.' }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/generate_ini_documentation.ps1 *> (Join-Path $out 'generated-headers.log')
if ($LASTEXITCODE -ne 0) { throw 'Generated project headers are unavailable.' }
$env:INCLUDE = (Join-Path $repo 'build-artifacts/generated') + ';' + $env:INCLUDE
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$devcmd = & $vswhere -latest -products '*' -version '[17.14,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'Common7/Tools/VsDevCmd.bat'
if (-not $devcmd) { throw 'Supported MSVC toolchain not found.' }
$outRelative = 'build-artifacts\native-runtime\N06-M01-2026-10-10'
$resourceRelative = $outRelative + '\localization_resources.res'
$objectRelative = $outRelative + '\obj\'
$exe = Join-Path $out 'n06_m01_zoom_game_derived_harness.exe'
$exeRelative = $outRelative + '\n06_m01_zoom_game_derived_harness.exe'
New-Item -ItemType Directory -Force -Path (Join-Path $out 'obj') | Out-Null
$compileInputs = '@build-artifacts\tests\runtime_adapter_sources.rsp'
if ($ReuseDependencies) {
    $dependencySources = Get-Content -LiteralPath 'build-artifacts/tests/runtime_adapter_sources.rsp'
    $objectNames = @($dependencySources | ForEach-Object { [IO.Path]::GetFileNameWithoutExtension($_) + '.obj' })
    if (($objectNames | Select-Object -Unique).Count -ne $objectNames.Count) { throw 'Dependency object basenames collide; run without -ReuseDependencies.' }
    $dependencyObjects = @($objectNames | ForEach-Object { $objectRelative + $_ })
    foreach ($object in $dependencyObjects) { if (-not (Test-Path -LiteralPath (Join-Path $repo $object))) { throw "Missing same-run dependency object: $object" } }
    $dependencyResponse = $outRelative + '\dependency-objects.rsp'
    $dependencyObjects | Set-Content -Encoding ascii -LiteralPath $dependencyResponse
    $compileInputs = '@' + $dependencyResponse
}
$compile = 'call "' + $devcmd + '" -arch=x64 -host_arch=x64 && rc /nologo /fo ' + $resourceRelative + ' src\overlay\localization\localization_resources.rc && cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /DNDEBUG /DSAFETYHOOK_TEST_VIRTUAL_PROTECT /DOVERLAY_PRODUCTION /DOVERLAY_SETTINGS_FRONTEND /DOVERLAY_COMBINED /DOVERLAY_STARTUP_JOURNAL /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui /Iexternal\imgui\backends /Fo' + $objectRelative + ' tests\plugin\n06_m01_zoom_game_derived_harness.cpp tests\plugin\runtime_owner_binding_probe.cpp ' + $compileInputs + ' /link user32.lib psapi.lib bcrypt.lib dxgi.lib d3d11.lib dcomp.lib d3dcompiler.lib ' + $resourceRelative + ' /OUT:' + $exeRelative
$compile | Set-Content -LiteralPath (Join-Path $support 'compile-command.txt')
$compileLog = Join-Path $support 'compile.log'
$oldPreference = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
& cmd.exe /d /c $compile *> $compileLog
$compileExit = $LASTEXITCODE; $ErrorActionPreference = $oldPreference
[pscustomobject]@{Command=$compile;ExitCode=$compileExit} | ConvertTo-Json -Compress |
    Set-Content -LiteralPath (Join-Path $support 'compile-result.json')
if ($compileExit -ne 0) { Get-Content -LiteralPath $compileLog -Tail 50; throw "Fixture compile failed: $compileExit" }

$runs = @()
foreach ($name in 'zoom-in','zoom-out') {
    foreach ($mode in 'off','on') {
        $stdout = Join-Path $support "$name-diagnostic-$mode.stdout.log"
        $stderr = Join-Path $support "$name-diagnostic-$mode.stderr.log"
        $oldPreference = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
        & $exe $name $mode 1> $stdout 2> $stderr
        $exit = $LASTEXITCODE; $ErrorActionPreference = $oldPreference
        $runs += [pscustomobject]@{Fixture=$name;DiagnosticMode=$mode;ExitCode=$exit;Stdout=(Split-Path $stdout -Leaf);Stderr=(Split-Path $stderr -Leaf)}
        if ($exit -ne 0) { Get-Content -LiteralPath $stdout; Get-Content -LiteralPath $stderr; throw "$name Diagnostic Mode $mode failed: exit $exit" }
    }
}
$existingRuns = @()
$existingHarnesses = @(
    [pscustomobject]@{Profile='ordinary';Path='build-artifacts/native-runtime-evidence/2026-10-09/ordinary/runtime_native_adapter_harness.exe';Hash='B82F89AE2DB7F6BD7DA945E5CAB073827B842423C1E71E73E2BC0E700921AD32'},
    [pscustomobject]@{Profile='diagnostic';Path='build-artifacts/native-runtime-evidence/2026-10-09/diagnostic/runtime_native_adapter_harness.exe';Hash='F62B167F41BF40DBAC2DC19B732D099EB04082E01E5CC133E010C82822987B50'}
)
foreach ($harness in $existingHarnesses) {
    if ((Get-FileHash -LiteralPath $harness.Path -Algorithm SHA256).Hash -ne $harness.Hash) { throw "Existing $($harness.Profile) harness identity mismatch." }
    foreach ($scenario in 'zoom-in-throw','zoom-out-throw','evidence-zoom-native-continuation','zoom') {
        foreach ($mode in 'off','on') {
            $stdout = Join-Path $support "existing-$($harness.Profile)-$scenario-diagnostic-$mode.stdout.log"
            $stderr = Join-Path $support "existing-$($harness.Profile)-$scenario-diagnostic-$mode.stderr.log"
            $oldPreference = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
            & $harness.Path $scenario $mode 1> $stdout 2> $stderr
            $exit = $LASTEXITCODE; $ErrorActionPreference = $oldPreference
            $existingRuns += [pscustomobject]@{Profile=$harness.Profile;Scenario=$scenario;DiagnosticMode=$mode;ExitCode=$exit;Stdout=(Split-Path $stdout -Leaf);Stderr=(Split-Path $stderr -Leaf)}
            if ($exit -ne 0) { Get-Content -LiteralPath $stdout; Get-Content -LiteralPath $stderr; throw "Existing $($harness.Profile) $scenario diagnostic=$mode failed: exit $exit" }
        }
    }
}
@{NewFixtures=$runs;ExistingZoomRegressions=$existingRuns} | ConvertTo-Json -Depth 6 |
    Set-Content -LiteralPath (Join-Path $support 'fixture-results.json')
$exeHash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
$sourceHashes = @('tests/plugin/n06_m01_zoom_game_derived_harness.cpp','tests/plugin/n06_m01_zoom_game_derived_fixtures.hpp',
    'tests/plugin/runtime_native_adapter_harness.cpp','tests/plugin/runtime_native_evidence_cases.hpp') | ForEach-Object {
    [pscustomobject]@{Path=$_;SHA256=(Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash}
}
$sourceHashes | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $support 'test-source-hashes.csv')
[pscustomobject]@{GameExe=$GameExe;GameSHA256=$gameHash;FixtureExe=$exe;FixtureSHA256=$exeHash;Runs=$runs} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $support 'identity-and-results.json')

$afterFiles = @($protected | Sort-Object FullName -Unique | ForEach-Object {
    [pscustomobject]@{Path=$_.FullName.Substring($repo.Length+1);SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
})
$afterFiles | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $support 'protected-after.csv')
$delta = Compare-Object $before $afterFiles -Property Path,SHA256
if ($delta) { $delta | Format-Table; throw 'Production/vendor/release files changed during N06-M01.' }
if ((Get-FileHash -LiteralPath $fixtureFile -Algorithm SHA256).Hash -ne $fixtureHash) { throw 'Prior N06 Zoom evidence fixture changed during validation.' }
'PASS: production/vendor/release files unchanged; existing N06 replay fixture hash unchanged.' |
    Set-Content -LiteralPath (Join-Path $support 'preservation-result.txt')
Get-ChildItem -LiteralPath $out -File | Where-Object { $_.Name -ne 'output-hashes.csv' } | ForEach-Object {
    [pscustomobject]@{Name=$_.Name;SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash;Length=$_.Length}
} | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $support 'output-hashes.csv')
Copy-Item -LiteralPath (Join-Path $out 'game-derived-bytes.txt') -Destination (Join-Path $support 'game-derived-bytes.txt') -Force
@('tests/plugin/n06_m01_zoom_game_derived_harness.cpp','tests/plugin/n06_m01_zoom_game_derived_fixtures.hpp',
  'tests/plugin/runtime_native_adapter_harness.cpp','tests/plugin/runtime_native_evidence_cases.hpp',
  'research/evidence/native-runtime/N06/tools/n06_m01_validate.ps1') | ForEach-Object {
    [pscustomobject]@{Path=$_;SHA256=(Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash}
} | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $support 'tool-source-hashes.csv')
Get-ChildItem -LiteralPath $support -File | Where-Object { $_.Name -ne 'supporting-hashes.csv' } | ForEach-Object {
    [pscustomobject]@{Name=$_.Name;SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash;Length=$_.Length}
} | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $support 'supporting-hashes.csv')
Write-Output "N06-M01 PASS. Game=$gameHash fixtureExe=$exeHash; newFixtures=$($runs.Count), existingZoomProcesses=$($existingRuns.Count), total=$($runs.Count + $existingRuns.Count)."
