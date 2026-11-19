$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
Import-Module (Join-Path $root 'tests/runner/process_tree_supervisor.psm1') -Force
function Run-Contained([string]$name, [string]$exe, [string[]]$arguments, [int]$timeout=30000) {
    $result = Invoke-TestProcessTree -FileName $exe -ArgumentList $arguments -WorkingDirectory $root -TimeoutMilliseconds $timeout -NoWindow -ExpectedResidualProcessNames @('vctip.exe')
    $result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot ($name+'.json')) -Encoding UTF8
    Write-Output ("{0}: exit={1} cleanup={2} timeout={3}" -f $name,$result.ExitCode,$result.CleanupVerified,$result.TimedOut)
    if ($result.ExitCode -ne 0 -or !$result.CleanupVerified -or $result.TimedOut) {
        Write-Output $result.Output
        throw "$name failed"
    }
}
Run-Contained 'focused-build' $env:ComSpec @('/d','/c',(Join-Path $PSScriptRoot 'build-focused.cmd')) 240000
$bin = Join-Path $root 'build-artifacts/cm/test-suite/tests'
Run-Contained 'trace-model' (Join-Path $bin 'camera_writer_identity_harness.exe') @()
foreach ($mode in @('off','on')) {
    Run-Contained ('adapter-aggregation-'+$mode) (Join-Path $bin 'runtime_native_adapter_harness.exe') @('diagnostic-aggregation',$mode)
    Run-Contained ('adapter-work-'+$mode) (Join-Path $bin 'runtime_native_adapter_harness.exe') @('diagnostic-work',$mode)
    foreach ($case in @('writer-combine','lifecycle','lifecycle-off','recovery','recovery-foreign-source','dialogue-off','dialogue-active','dialogue-disabled-in-flight')) {
        Run-Contained ($case+'-'+$mode) (Join-Path $bin 'runtime_native_adapter_harness.exe') @($case,$mode)
    }
}
