param([ValidateSet('FocusedBuild', 'Focused', 'Regression', 'Release')]
    [string]$Phase = 'FocusedBuild')

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Import-Module (Join-Path $root 'tests/runner/process_tree_supervisor.psm1') -Force
$evidence = Join-Path $root 'build-artifacts/security-hardening'
[IO.Directory]::CreateDirectory($evidence) | Out-Null

function Invoke-Checked {
    param([string]$Name, [string]$Executable, [string[]]$Arguments, [int]$Timeout,
        [string[]]$ExpectedResiduals = @())
    $result = Invoke-TestProcessTree -FileName $Executable -ArgumentList $Arguments `
        -WorkingDirectory $root -TimeoutMilliseconds $Timeout -NoWindow `
        -MaximumOutputCharacters 1048576 -ExpectedResidualProcessNames $ExpectedResiduals
    $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (
        Join-Path $evidence ($Name + '.json')) -Encoding UTF8
    Write-Output $result.Output
    Write-Output ("{0}: exit={1} timeout={2} cleanup={3} output_capped={4}" -f
        $Name, $result.ExitCode, $result.TimedOut, $result.CleanupVerified,
        $result.OutputTruncated)
    if ($result.TimedOut -or $result.ExitCode -ne 0 -or !$result.CleanupVerified -or
        $result.OutputTruncated) { throw "Validation failed: $Name" }
}

if ($Phase -eq 'Focused') {
    $bin = Join-Path $root 'build-artifacts/cm/test-suite/tests'
    foreach ($name in @('worker_lifecycle_harness', 'config_persistence_harness',
        'localization_harness', 'startup_journal_harness')) {
        Invoke-Checked $name (Join-Path $bin ($name + '.exe')) @() 30000
    }
    foreach ($case in @('evidence-native-trampoline',
        'evidence-safetyhook-lifetime-faults', 'evidence-safetyhook-failed-create',
        'evidence-safetyhook-veh-admission', 'evidence-safetyhook-delayed-execute',
        'evidence-safetyhook-allocation-ownership',
        'evidence-zoom-native-continuation')) {
        foreach ($diagnostics in @('off', 'on')) {
            Invoke-Checked ($case + '-' + $diagnostics) `
                (Join-Path $bin 'runtime_native_adapter_harness.exe') `
                @($case, $diagnostics) 30000
        }
    }
} else {
    $timeout = if ($Phase -eq 'Regression') { 2400000 } else { 1200000 }
    Invoke-Checked $Phase (Join-Path $env:SystemRoot 'System32/cmd.exe') `
        @('/d', '/c', 'research\tools\security_validation.cmd', $Phase) $timeout @('vctip.exe')
}
