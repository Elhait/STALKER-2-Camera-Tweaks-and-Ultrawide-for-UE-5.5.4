$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$registryPath = Join-Path $repo 'tools/build/production_sources.json'
$registry = Get-Content -LiteralPath $registryPath -Raw | ConvertFrom-Json
if ($registry.schema_version -ne 1) { throw 'Unsupported production source registry schema.' }
$entries = @($registry.sources)
$paths = @($entries | ForEach-Object { $_.path })
if (@($paths | Sort-Object -Unique).Count -ne $paths.Count) { throw 'Duplicate path in production source registry.' }
foreach ($entry in $entries) {
    if ($entry.role -notin @('runtime_owner', 'dll_entry', 'production', 'vendor')) {
        throw "Unknown production source role '$($entry.role)' for '$($entry.path)'."
    }
    if (-not (Test-Path -LiteralPath (Join-Path $repo $entry.path) -PathType Leaf)) {
        throw "Missing production source '$($entry.path)'."
    }
}
$runtimeOwners = @($entries | Where-Object role -eq 'runtime_owner')
$dllEntries = @($entries | Where-Object role -eq 'dll_entry')
if ($runtimeOwners.Count -ne 1 -or $runtimeOwners[0].path -ne 'src/plugin/runtime.cpp') {
    throw 'Registry must designate runtime.cpp exactly once as runtime_owner.'
}
if ($dllEntries.Count -ne 1 -or $dllEntries[0].path -ne 'src/plugin/dll_entry.cpp') {
    throw 'Registry must designate dll_entry.cpp exactly once as dll_entry.'
}
$adapterSources = @($entries | Where-Object role -notin @('runtime_owner', 'dll_entry') |
    ForEach-Object { $_.path.Replace('/', '\') })
if ($adapterSources.Count -ne 59) { throw "Expected 59 adapter dependency inputs, found $($adapterSources.Count)." }
$outputPath = Join-Path $repo 'build-artifacts/tests/runtime_adapter_sources.rsp'
$outputContent = ($adapterSources -join "`r`n") + "`r`n"
if (-not (Test-Path -LiteralPath $outputPath) -or
    [IO.File]::ReadAllText($outputPath) -cne $outputContent) {
    [IO.File]::WriteAllText($outputPath, $outputContent, [Text.Encoding]::ASCII)
}
Write-Output "Runtime adapter inputs: $($adapterSources.Count) registry sources; runtime.cpp included by harness once; dll_entry excluded."
