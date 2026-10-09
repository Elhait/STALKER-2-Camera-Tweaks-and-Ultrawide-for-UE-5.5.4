$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$lines = Get-Content -LiteralPath (Join-Path $repo 'build.cmd')
$sources = @($lines | Where-Object { $_ -match '^ (src|external)\\' } | ForEach-Object { $_.Trim().TrimEnd('^').Trim() }) -join ' '
$sources = @($sources -split '\s+' | Where-Object { $_ -notin @('src\plugin\runtime.cpp','src\plugin\dll_entry.cpp') })
if ($sources.Count -lt 40 -or $sources -contains 'src\plugin\runtime.cpp') { throw 'Production source registry extraction failed.' }
$sources -join "`r`n" | Set-Content -Encoding ascii -LiteralPath (Join-Path $repo 'build-artifacts/tests/runtime_adapter_sources.rsp')
Write-Output "Runtime adapter inputs: $($sources.Count) actual production/vendor sources; runtime.cpp included once; dll_entry excluded."
