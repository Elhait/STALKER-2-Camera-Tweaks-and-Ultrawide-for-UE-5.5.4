$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
Import-Module (Join-Path $root 'tests/runner/process_tree_supervisor.psm1') -Force
foreach ($profile in @('production','diagnostic')) {
    $result = Invoke-TestProcessTree -FileName $env:ComSpec -ArgumentList @('/d','/c',(Join-Path $PSScriptRoot 'build-release.cmd'),$profile) -WorkingDirectory $root -TimeoutMilliseconds 300000 -NoWindow -ExpectedResidualProcessNames @('vctip.exe')
    $result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot ($profile+'-release.json')) -Encoding UTF8
    Write-Output ("{0}: exit={1} cleanup={2} timeout={3}" -f $profile,$result.ExitCode,$result.CleanupVerified,$result.TimedOut)
    if ($result.ExitCode -ne 0 -or !$result.CleanupVerified -or $result.TimedOut) {
        Write-Output $result.Output
        throw "$profile failed"
    }
}
