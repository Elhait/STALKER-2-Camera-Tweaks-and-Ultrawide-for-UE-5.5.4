param(
    [Parameter(Mandatory = $true)]
    [string] $RegistryPath = 'tests/test_harnesses.json'
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Set-Location -LiteralPath $projectRoot
$registryFile = [IO.Path]::GetFullPath((Join-Path $projectRoot $RegistryPath))
$registry = Get-Content -LiteralPath $registryFile -Raw | ConvertFrom-Json
if ($registry.schema_version -ne 1) { throw 'Unsupported test harness registry schema.' }

$outputDirectory = Join-Path $projectRoot $registry.compiler_contract.output_directory
$entries = @($registry.runner)
$timeoutMilliseconds = 600000
if (-not [string]::IsNullOrWhiteSpace($env:CAMERA_TWEAKS_TEST_TIMEOUT_MS)) {
    $parsedTimeout = 0
    if (-not [int]::TryParse($env:CAMERA_TWEAKS_TEST_TIMEOUT_MS, [ref] $parsedTimeout) -or $parsedTimeout -le 0) {
        throw 'CAMERA_TWEAKS_TEST_TIMEOUT_MS must be a positive integer.'
    }
    $timeoutMilliseconds = $parsedTimeout
}

for ($index = 0; $index -lt $entries.Count; ++$index) {
    $entry = $entries[$index]
    $executable = Join-Path $outputDirectory ($entry.harness + '.exe')
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        Write-Error "Harness executable is missing: $executable"
        exit 2
    }

    $arguments = @($entry.arguments)
    Write-Output ("[{0}/{1}] {2}{3}" -f ($index + 1), $entries.Count, $entry.harness,
        $(if ($arguments.Count -eq 0) { '' } else { ' ' + ($arguments -join ' ') }))

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $executable
    $startInfo.Arguments = $arguments -join ' '
    $startInfo.WorkingDirectory = $projectRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $false
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    if (-not $process.Start()) {
        Write-Output "FAIL $($entry.harness) could not start"
        exit 125
    }
    if (-not $process.WaitForExit($timeoutMilliseconds)) {
        Write-Output ("TIMEOUT {0} after {1}ms" -f $entry.harness, $timeoutMilliseconds)
        $process.Kill()
        $process.WaitForExit(5000) | Out-Null
        exit 124
    }
    $exitCode = $process.ExitCode
    if ($exitCode -ne 0) {
        Write-Output ("FAIL {0} exit={1}" -f $entry.harness, $exitCode)
        exit $exitCode
    }
}

Write-Output ("runner_invocations={0} exit=0" -f $entries.Count)
exit 0
