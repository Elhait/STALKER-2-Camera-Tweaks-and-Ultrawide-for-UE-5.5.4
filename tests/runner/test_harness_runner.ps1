param(
    [Parameter(Mandatory = $true)]
    [string] $RegistryPath = 'tests/test_harnesses.json',
    [string] $OutputDirectory,
    [switch] $NonGraphics
)

$ErrorActionPreference = 'Stop'
$supervisorModule = Join-Path $PSScriptRoot 'process_tree_supervisor.psm1'
Import-Module -Name $supervisorModule -Force
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Set-Location -LiteralPath $projectRoot
$registryFile = [IO.Path]::GetFullPath((Join-Path $projectRoot $RegistryPath))
$registry = Get-Content -LiteralPath $registryFile -Raw | ConvertFrom-Json
if ($registry.schema_version -ne 1) { throw 'Unsupported test harness registry schema.' }

$outputDirectory = if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    Join-Path $projectRoot $registry.compiler_contract.output_directory
} else {
    if ([IO.Path]::IsPathRooted($OutputDirectory)) {
        [IO.Path]::GetFullPath($OutputDirectory)
    } else {
        [IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDirectory))
    }
}
$entries = @($registry.runner)
$timeoutMilliseconds = 600000
if (-not [string]::IsNullOrWhiteSpace($env:CAMERA_TWEAKS_TEST_TIMEOUT_MS)) {
    $parsedTimeout = 0
    if (-not [int]::TryParse($env:CAMERA_TWEAKS_TEST_TIMEOUT_MS, [ref] $parsedTimeout) -or $parsedTimeout -le 0) {
        throw 'CAMERA_TWEAKS_TEST_TIMEOUT_MS must be a positive integer.'
    }
    $timeoutMilliseconds = $parsedTimeout
}
$suiteTimeoutMilliseconds = 1800000
if (-not [string]::IsNullOrWhiteSpace($env:CAMERA_TWEAKS_TEST_SUITE_TIMEOUT_MS)) {
    $parsedSuiteTimeout = 0
    if (-not [int]::TryParse($env:CAMERA_TWEAKS_TEST_SUITE_TIMEOUT_MS, [ref] $parsedSuiteTimeout) -or
        $parsedSuiteTimeout -le 0 -or $parsedSuiteTimeout -gt 3600000) {
        throw 'CAMERA_TWEAKS_TEST_SUITE_TIMEOUT_MS must be between 1 and 3600000.'
    }
    $suiteTimeoutMilliseconds = $parsedSuiteTimeout
}
$suiteClock = [Diagnostics.Stopwatch]::StartNew()

for ($index = 0; $index -lt $entries.Count; ++$index) {
    $remainingSuiteMilliseconds = $suiteTimeoutMilliseconds - [int]$suiteClock.ElapsedMilliseconds
    if ($remainingSuiteMilliseconds -le 0) {
        Write-Output ("SUITE_TIMEOUT after {0}ms" -f $suiteTimeoutMilliseconds)
        exit 124
    }
    $entry = $entries[$index]
    $executable = Join-Path $outputDirectory ($entry.harness + '.exe')
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        Write-Error "Harness executable is missing: $executable"
        exit 2
    }

    $arguments = @($entry.arguments)
    if ($NonGraphics -and $entry.harness -eq 'runtime_native_adapter_harness') {
        $arguments = @('--non-graphics')
    }
    Write-Output ("[{0}/{1}] {2}{3}" -f ($index + 1), $entries.Count, $entry.harness,
        $(if ($arguments.Count -eq 0) { '' } else { ' ' + ($arguments -join ' ') }))

    $entryTimeoutMilliseconds = [Math]::Min($timeoutMilliseconds, $remainingSuiteMilliseconds)
    $result = Invoke-TestProcessTree -FileName $executable -ArgumentList $arguments `
        -WorkingDirectory $projectRoot -TimeoutMilliseconds $entryTimeoutMilliseconds -StreamOutput
    if ($result.OutputTruncated) { Write-Output "OUTPUT_TRUNCATED $($entry.harness) limit=1048576 characters" }
    if ($result.TimedOut) {
        if ($result.DescendantsOutlivedRoot) {
            Write-Output ("TIMEOUT {0}: root exited while descendants remained; cleanup verified" -f $entry.harness)
            if (-not [string]::IsNullOrWhiteSpace($result.RemainingProcesses)) {
                Write-Output ("DESCENDANTS {0}" -f $result.RemainingProcesses)
            }
        } elseif ($entryTimeoutMilliseconds -lt $timeoutMilliseconds) {
            Write-Output ("SUITE_TIMEOUT after {0}ms at {1}; process tree cleanup verified" -f $suiteTimeoutMilliseconds, $entry.harness)
        } else {
            Write-Output ("TIMEOUT {0} after {1}ms; process tree cleanup verified" -f $entry.harness, $timeoutMilliseconds)
        }
        exit 124
    }
    $exitCode = $result.ExitCode
    if ($exitCode -ne 0) {
        Write-Output ("FAIL {0} exit={1}" -f $entry.harness, $exitCode)
        exit $exitCode
    }
}

Write-Output ("runner_invocations={0} exit=0 non_graphics={1} WARP_case_omitted={1}" -f $entries.Count,[int][bool]$NonGraphics)
exit 0
