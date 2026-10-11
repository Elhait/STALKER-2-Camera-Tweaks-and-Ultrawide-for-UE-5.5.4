param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Capture', 'Focused', 'Full', 'Production', 'Diagnostic', 'Locale', 'Preservation')]
    [string]$Action,
    [ValidateSet('ordinary', 'diagnostic', 'both')]
    [string]$Profile = 'both',
    [switch]$ReuseObjects
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$evidenceRoot = Join-Path $repoRoot 'build-artifacts/native-runtime-evidence/2026-10-09'
$evidenceRelative = 'build-artifacts\native-runtime-evidence\2026-10-09'
New-Item -ItemType Directory -Force -Path $evidenceRoot | Out-Null
Set-Location -LiteralPath $repoRoot

function Get-PreservedManifest {
    $files = @('src', 'external', 'locales', 'docs') | ForEach-Object {
        Get-ChildItem -LiteralPath (Join-Path $repoRoot $_) -File -Recurse
    }
    $files += Get-ChildItem -LiteralPath $repoRoot -File | Where-Object {
        $_.Extension -in '.asi', '.zip', '.ini', '.md', '.cmd'
    }
    $files += Get-Item -LiteralPath @(
        'backlog/active/RUNTIME_CPP_DECOMPOSITION_IMPLEMENTATION_PLAN_2026-10-08.md',
        'research/evidence/runtime-decomposition/MOVE_LEDGER_2026-10-08.md'
    )
    $files | Sort-Object FullName -Unique | ForEach-Object {
        [pscustomobject]@{
            Path = $_.FullName.Substring($repoRoot.Length + 1).Replace('\', '/')
            SHA256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    }
}

function Invoke-RecordedCommand([string]$Name, [string]$Command) {
    $logPath = Join-Path $evidenceRoot ($Name + '-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff') + '.log')
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & cmd.exe /d /c $Command *> $logPath
    $result = $LASTEXITCODE
    $ErrorActionPreference = $previousPreference
    [pscustomobject]@{
        Name = $Name; Command = $Command; ExitCode = $result
        UTC = [DateTime]::UtcNow.ToString('o'); Log = $logPath
    } | ConvertTo-Json -Compress | Add-Content -LiteralPath (Join-Path $evidenceRoot 'commands.jsonl')
    Write-Output "$Name exit=$result log=$logPath"
    if ($result -ne 0) {
        Get-Content -LiteralPath $logPath -Tail 45
        throw "$Name failed with exit $result"
    }
}

switch ($Action) {
    'Capture' {
        $baselinePath = Join-Path $evidenceRoot 'preserved-before.csv'
        if (Test-Path -LiteralPath $baselinePath) { throw 'Preservation baseline already exists; do not overwrite it.' }
        Get-PreservedManifest | Export-Csv -LiteralPath $baselinePath -NoTypeInformation -Encoding UTF8
        & git status --short | Set-Content -LiteralPath (Join-Path $evidenceRoot 'git-status-before.txt')
        Write-Output "Captured preservation baseline: $baselinePath"
    }
    'Focused' {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        $devcmd = & $vswhere -latest -products '*' -version '[17.14,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'Common7/Tools/VsDevCmd.bat'
        if (-not $devcmd) { throw 'Supported Visual Studio toolchain unavailable.' }
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/runner/runtime_native_adapter_inputs.ps1
        if ($LASTEXITCODE -ne 0) { throw 'Adapter source input generation failed.' }
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/generate_ini_documentation.ps1
        if ($LASTEXITCODE -ne 0) { throw 'INI generated headers unavailable.' }
        $env:INCLUDE = (Join-Path $repoRoot 'build-artifacts/generated') + ';' + $env:INCLUDE
        $env:TEMP = Join-Path $evidenceRoot 'temp'
        $env:TMP = $env:TEMP
        New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
        $profiles = if ($Profile -eq 'both') { @('ordinary', 'diagnostic') } else { @($Profile) }
        foreach ($profile in $profiles) {
            $outputDir = Join-Path $evidenceRelative $profile
            New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
            $defines = '/DNDEBUG /DOVERLAY_PRODUCTION /DOVERLAY_SETTINGS_FRONTEND /DOVERLAY_COMBINED /DOVERLAY_STARTUP_JOURNAL /DSAFETYHOOK_TEST_VIRTUAL_PROTECT'
            $resource = Join-Path $outputDir 'resources.res'
            $exe = Join-Path $outputDir 'runtime_native_adapter_harness.exe'
            $compileInputs = '@build-artifacts\tests\runtime_adapter_sources.rsp'
            $linkInputs = ''
            if ($ReuseObjects) {
                $objects = Get-Content build-artifacts/tests/runtime_adapter_sources.rsp | ForEach-Object {
                    $object = Join-Path $outputDir ([IO.Path]::GetFileNameWithoutExtension($_) + '.obj')
                    if (-not (Test-Path -LiteralPath $object)) { throw "Missing retained dependency object: $object" }
                    $object
                }
                $objectRsp = Join-Path $outputDir 'dependency-objects.rsp'
                $objects | Set-Content -LiteralPath $objectRsp -Encoding ascii
                $compileInputs = '@' + $objectRsp
            }
            $compile = 'call "' + $devcmd + '" -arch=x64 -host_arch=x64 && rc /nologo /fo ' + $resource + ' src\overlay\localization\localization_resources.rc && cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 ' + $defines + ' /Fo' + $outputDir + '\ /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui /Iexternal\imgui\backends tests\plugin\runtime_native_adapter_harness.cpp tests\plugin\runtime_owner_binding_probe.cpp ' + $compileInputs + ' /link user32.lib psapi.lib bcrypt.lib dxgi.lib d3d11.lib dcomp.lib d3dcompiler.lib ' + $resource + $linkInputs + ' /OUT:' + $exe
            Invoke-RecordedCommand ($profile + '-compile') $compile
            Invoke-RecordedCommand ($profile + '-matrix') ('"' + $exe + '"')
            Get-FileHash -LiteralPath $exe -Algorithm SHA256 | Format-List
        }
    }
    'Full' { Invoke-RecordedCommand 'test-full' 'test.cmd' }
    'Locale' { Invoke-RecordedCommand 'locale-audit' 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests\runner\localization_catalog_audit.ps1' }
    'Production' {
        $env:CAMERA_TWEAKS_BUILD_PROFILE = ''
        $env:CAMERA_TWEAKS_OUTPUT_NAME = Join-Path $evidenceRelative 'production.asi'
        $env:CAMERA_TWEAKS_OBJECT_DIR = Join-Path $evidenceRelative 'production-obj'
        Invoke-RecordedCommand 'production-build' 'build.cmd'
        Get-FileHash -LiteralPath $env:CAMERA_TWEAKS_OUTPUT_NAME -Algorithm SHA256 | Format-List
    }
    'Diagnostic' {
        $env:CAMERA_TWEAKS_OUTPUT_NAME = Join-Path $evidenceRelative 'diagnostic.asi'
        $env:CAMERA_TWEAKS_OBJECT_DIR = Join-Path $evidenceRelative 'diagnostic-obj'
        Invoke-RecordedCommand 'diagnostic-build' 'tools\build\build-diagnostic.cmd'
        Get-FileHash -LiteralPath $env:CAMERA_TWEAKS_OUTPUT_NAME -Algorithm SHA256 | Format-List
    }
    'Preservation' {
        $before = Import-Csv -LiteralPath (Join-Path $evidenceRoot 'preserved-before.csv')
        $after = @(Get-PreservedManifest)
        $after | Export-Csv -LiteralPath (Join-Path $evidenceRoot 'preserved-after.csv') -NoTypeInformation -Encoding UTF8
        $difference = Compare-Object $before $after -Property Path, SHA256
        if ($difference) { $difference | Format-Table; throw 'Preserved production/release/decomposition file changed.' }
        Write-Output "Preservation PASS: $($after.Count) files unchanged."
    }
}
