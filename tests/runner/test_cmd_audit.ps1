param(
    [Parameter(Mandatory = $true)]
    [string] $RunnerPath
)

$lines = Get-Content -LiteralPath $RunnerPath
$projectRoot = Split-Path -Parent (Resolve-Path -LiteralPath $RunnerPath).Path
$sourceNames = @($lines | Select-String -Pattern 'tests\\[^ ]+_harness\.cpp' -AllMatches |
    ForEach-Object { $_.Matches | ForEach-Object { $_.Value } } | Sort-Object -Unique)
$compileNames = @($lines | Select-String -Pattern 'cl .*?/OUT:build-artifacts\\tests\\([^ ]+_harness)\.exe' -AllMatches |
    ForEach-Object { $_.Matches | ForEach-Object {
        [System.IO.Path]::GetFileName($_.Groups[1].Value)
    } } | Sort-Object -Unique)
$runNames = @($lines | Select-String -Pattern '^build-artifacts\\tests\\([^ ]+_harness)\.exe$' -AllMatches |
    ForEach-Object { $_.Matches | ForEach-Object { $_.Groups[1].Value } } | Sort-Object -Unique)

$compileMissingChecks = @()
$runMissingChecks = @()
for ($index = 0; $index -lt $lines.Count; ++$index) {
    $line = $lines[$index].Trim()
    if ($line -match '^cl .*tests\\[^ ]+_harness\.cpp ') {
        if ($index + 1 -ge $lines.Count -or $lines[$index + 1].Trim() -ne 'if errorlevel 1 goto :fail') {
            $compileMissingChecks += $line
        }
    }
    if ($line -match '^build-artifacts\\tests\\[^ ]+_harness\.exe$') {
        if ($index + 1 -ge $lines.Count -or $lines[$index + 1].Trim() -ne 'if errorlevel 1 goto :fail') {
            $runMissingChecks += $line
        }
    }
}

$sourceBasenames = @($sourceNames | ForEach-Object {
    [System.IO.Path]::GetFileNameWithoutExtension($_)
})
$sourceBasenames = @($sourceBasenames | Sort-Object -Unique)
$repositoryHarnesses = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'tests') -Recurse -File -Filter '*_harness.cpp' |
    ForEach-Object { $_.BaseName } | Sort-Object -Unique)
$unregisteredHarnesses = @($repositoryHarnesses | Where-Object { $_ -notin $sourceBasenames })
$unknownRunnerSources = @($sourceBasenames | Where-Object { $_ -notin $repositoryHarnesses })
$sameSet = (@($sourceBasenames | Where-Object { $_ -notin $compileNames }).Count -eq 0) -and
    (@($compileNames | Where-Object { $_ -notin $sourceBasenames }).Count -eq 0) -and
    (@($compileNames | Where-Object { $_ -notin $runNames }).Count -eq 0) -and
    (@($runNames | Where-Object { $_ -notin $compileNames }).Count -eq 0)

$compileChecksPass = $compileMissingChecks.Count -eq 0
$runChecksPass = $runMissingChecks.Count -eq 0
Write-Output (("runner_sources={0} runner_compiled={1} runner_executed={2} " +
    "compile_checks={3} run_checks={4} sets_equal={5} repository_inventory={6}") -f
    $sourceBasenames.Count, $compileNames.Count, $runNames.Count,
    $compileChecksPass, $runChecksPass, $sameSet,
    ($unregisteredHarnesses.Count -eq 0 -and $unknownRunnerSources.Count -eq 0))

if (-not $sameSet -or $compileMissingChecks.Count -ne 0 -or $runMissingChecks.Count -ne 0 -or
    $unregisteredHarnesses.Count -ne 0 -or $unknownRunnerSources.Count -ne 0) {
    if ($compileMissingChecks.Count -ne 0) { Write-Output 'missing_compile_checks:'; $compileMissingChecks }
    if ($runMissingChecks.Count -ne 0) { Write-Output 'missing_run_checks:'; $runMissingChecks }
    if ($unregisteredHarnesses.Count -ne 0) { Write-Output 'unregistered_repository_harnesses:'; $unregisteredHarnesses }
    if ($unknownRunnerSources.Count -ne 0) { Write-Output 'runner_sources_without_repository_harness:'; $unknownRunnerSources }
    exit 1
}
exit 0
