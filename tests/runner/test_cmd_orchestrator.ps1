param(
    [string] $RegistryPath = 'tests/test_harnesses.json',
    [switch] $NonGraphics
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Set-Location -LiteralPath $projectRoot
Import-Module -Name (Join-Path $PSScriptRoot 'process_tree_supervisor.psm1') -Force

$registryFile = [IO.Path]::GetFullPath((Join-Path $projectRoot $RegistryPath))
$registry = Get-Content -LiteralPath $registryFile -Raw | ConvertFrom-Json
if ($registry.schema_version -ne 1) { throw 'Unsupported test harness registry schema.' }
$supportTarget = @($registry.support_targets | Where-Object { $_.name -eq 'test_cmd_exit_status_probe' })
if ($supportTarget.Count -ne 1) { throw 'Canonical test registry must contain one exit-status support target.' }

$cmakeDirectory = $env:CMAKE_DIR
if ([string]::IsNullOrWhiteSpace($cmakeDirectory)) { throw 'CMAKE_DIR must be provided by test.cmd.' }
$cmakeExecutable = Join-Path $cmakeDirectory 'cmake.exe'
$auditScript = Join-Path $PSScriptRoot 'test_cmd_audit.ps1'
$exitContractScript = Join-Path $PSScriptRoot 'test_cmd_exit_code_contract.ps1'
$runnerScript = Join-Path $PSScriptRoot 'test_harness_runner.ps1'
$powershellExecutable = (Get-Command powershell.exe -ErrorAction Stop).Source
$buildDirectory = $env:BUILD_DIR
if ([string]::IsNullOrWhiteSpace($buildDirectory)) { throw 'BUILD_DIR must be provided by test.cmd.' }
$buildDirectory = [IO.Path]::GetFullPath((Join-Path $projectRoot $buildDirectory))
$testOutputDirectory = Join-Path $buildDirectory 'tests'
$supportExecutable = Join-Path $testOutputDirectory ((Split-Path -Leaf $supportTarget[0].output))
$ninjaExecutable = Join-Path $env:NINJA_DIR 'ninja.exe'

function Invoke-ContainedStep {
    param(
        [Parameter(Mandatory = $true)][string] $Name,
        [Parameter(Mandatory = $true)][string] $Executable,
        [Parameter(Mandatory = $true)][string[]] $Arguments,
        [Parameter(Mandatory = $true)][int] $TimeoutMilliseconds,
        [string[]] $ExpectedResidualProcessNames = @()
    )

    Write-Output ("[{0}] starting (timeout={1}ms)" -f $Name, $TimeoutMilliseconds)
    $result = Invoke-TestProcessTree -FileName $Executable -ArgumentList $Arguments `
        -WorkingDirectory $projectRoot -TimeoutMilliseconds $TimeoutMilliseconds -StreamOutput `
        -ExpectedResidualProcessNames $ExpectedResidualProcessNames
    if ($result.OutputTruncated) { Write-Output ("[{0}] output capped at 1 MiB" -f $Name) }
    if ($result.TimedOut) {
        if ($result.DescendantsOutlivedRoot) {
            Write-Output ("[{0}] FAIL: root exited with descendants still running; cleanup verified" -f $Name)
            if (-not [string]::IsNullOrWhiteSpace($result.RemainingProcesses)) {
                Write-Output ("[{0}] DESCENDANTS {1}" -f $Name, $result.RemainingProcesses)
            }
            exit 124
        }
        Write-Output ("[{0}] FAIL: timed out after {1}ms; process-tree cleanup verified" -f $Name, $TimeoutMilliseconds)
        exit 124
    }
    if ($result.ExitCode -ne 0) {
        Write-Output ("[{0}] FAIL: exit code {1}; process-tree cleanup verified" -f $Name, $result.ExitCode)
        exit $result.ExitCode
    }
    Write-Output ("[{0}] PASS; process-tree cleanup verified" -f $Name)
    if ($result.ExpectedDescendantsCleaned) {
        Write-Output ("[{0}] expected compiler helper(s) were terminated and cleanup verified: {1}" -f $Name, $result.RemainingProcesses)
    }
}

Invoke-ContainedStep -Name 'test registry audit' -Executable $powershellExecutable `
    -Arguments @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $auditScript,
        '-RunnerPath', 'test.cmd', '-RegistryPath', $RegistryPath) -TimeoutMilliseconds 30000

Invoke-ContainedStep -Name 'CMake configure' -Executable $cmakeExecutable `
    -Arguments @('-S', '.', '-B', $buildDirectory, '-G', 'Ninja',
        ('-DCMAKE_MAKE_PROGRAM=' + $ninjaExecutable), '-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_TRY_COMPILE_CONFIGURATION=Release', '-DCAMERA_TWEAKS_PROFILE=production',
        '-DCAMERA_TWEAKS_STARTUP_TIMELINE=OFF', '-DCAMERA_TWEAKS_DIALOGUE_DISCOVERY=OFF') `
    -TimeoutMilliseconds 120000 -ExpectedResidualProcessNames @('vctip.exe')

Invoke-ContainedStep -Name 'harness build' -Executable $ninjaExecutable `
    -Arguments @('-C', $buildDirectory, 'test_harnesses', '-j', '1') `
    -TimeoutMilliseconds 1200000 -ExpectedResidualProcessNames @('vctip.exe')

Invoke-ContainedStep -Name 'runner exit-code contract' -Executable $powershellExecutable `
    -Arguments @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $exitContractScript,
        '-ProbePath', $supportExecutable, '-OutputDirectory', $testOutputDirectory) -TimeoutMilliseconds 90000

$runnerArguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $runnerScript,
    '-RegistryPath', $RegistryPath, '-OutputDirectory', $testOutputDirectory)
if ($NonGraphics) { $runnerArguments += '-NonGraphics' }
Invoke-ContainedStep -Name 'canonical harness registry' -Executable $powershellExecutable `
    -Arguments $runnerArguments -TimeoutMilliseconds 1800000

Write-Output ("test.cmd completed: registry audit, build, exit-code contract and selected canonical harnesses PASS; non_graphics={0}." -f [int][bool]$NonGraphics)
exit 0
