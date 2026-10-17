param(
    [Parameter(Mandatory = $true)][string] $Executable,
    [string] $ArgumentsJson = '[]',
    [ValidateRange(100, 600000)][int] $TimeoutMilliseconds = 300000
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'process_tree_supervisor.psm1') -Force
$arguments = [string[]](ConvertFrom-Json -InputObject $ArgumentsJson)
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$result = Invoke-TestProcessTree -FileName $Executable -ArgumentList $arguments `
    -WorkingDirectory $root -TimeoutMilliseconds $TimeoutMilliseconds -StreamOutput
if (-not $result.CleanupVerified) { throw 'CTest harness cleanup is unverified.' }
if ($result.OutputTruncated) { Write-Output 'CTest harness output capped at 1 MiB.' }
Write-Output ("CTest containment: exit={0} timeout={1} cleanup=verified" -f `
    $result.ExitCode, $result.TimedOut)
if ($result.TimedOut) { exit 124 }
exit $result.ExitCode
