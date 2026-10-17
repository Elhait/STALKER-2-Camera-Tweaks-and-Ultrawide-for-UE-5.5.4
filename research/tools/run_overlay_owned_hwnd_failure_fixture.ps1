param([switch]$BuildOnly)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output = Join-Path $repo 'build-artifacts/overlay-owned-hwnd-failure'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$source = Join-Path $repo 'research/prototypes/overlay_owned_hwnd/failure_handoff_fixture.cpp'
$exe = Join-Path $output 'failure-handoff-fixture.exe'
$response = Join-Path $output 'compile.rsp'
$linkResponse = Join-Path $output 'link.rsp'
$lines = @('/nologo','/c','/std:c++latest','/EHsc','/W4','/WX','/utf-8','/O1','/MT',
    ('/Fo"' + (Join-Path $output 'failure.obj') + '"'), ('"' + $source + '"'))
[IO.File]::WriteAllLines($response, $lines, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllLines($linkResponse, @('/NOLOGO', ('/OUT:"' + $exe + '"'),
    ('"' + (Join-Path $output 'failure.obj') + '"'), 'user32.lib','gdi32.lib','ole32.lib',
    'd3d11.lib','dxgi.lib','dcomp.lib'), [Text.UTF8Encoding]::new($false))
& (Join-Path $PSScriptRoot 'compile_overlay_attachment_fixture.cmd') $response $linkResponse 2>&1 |
    Tee-Object -FilePath (Join-Path $output 'build.log')
$buildCode = $LASTEXITCODE
if ($buildCode -ne 0) { throw "Failure fixture build failed: $buildCode" }
$sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
$exeHash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
@("source=$sourceHash", "exe=$exeHash", "build_exit=$buildCode") |
    Set-Content -LiteralPath (Join-Path $output 'identity.txt') -Encoding ascii
if ($BuildOnly) { exit 0 }

$modes = @('race','startup','queue-full','partial-0','partial-1','partial-2',
    'cleanup-failure','stopping','late','stale-token')
$results = [Collections.Generic.List[object]]::new()
foreach ($mode in $modes) {
    $stdout = Join-Path $output "$mode.stdout.log"
    $stderr = Join-Path $output "$mode.stderr.log"
    Remove-Item -LiteralPath $stdout,$stderr -Force -ErrorAction SilentlyContinue
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $exe
    $start.Arguments = $mode
    $start.WorkingDirectory = $output
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    [void]$process.Start()
    $outTask = $process.StandardOutput.ReadToEndAsync()
    $errTask = $process.StandardError.ReadToEndAsync()
    if (!$process.WaitForExit(15000)) {
        $process.Kill($true)
        $process.WaitForExit()
        [IO.File]::WriteAllText($stdout, $outTask.GetAwaiter().GetResult())
        [IO.File]::WriteAllText($stderr, $errTask.GetAwaiter().GetResult())
        throw "Scenario '$mode' exceeded 15 seconds; process terminated."
    }
    $outText = $outTask.GetAwaiter().GetResult()
    $errText = $errTask.GetAwaiter().GetResult()
    [IO.File]::WriteAllText($stdout, $outText)
    [IO.File]::WriteAllText($stderr, $errText)
    $pass = $process.ExitCode -eq 0 -and $outText.Contains("RESULT mode=$mode") -and
        $outText.Contains('PASS')
    $results.Add([pscustomobject]@{scenario=$mode;exit_code=$process.ExitCode;pass=$pass})
    "SCENARIO $mode exit=$($process.ExitCode) $(if($pass){'PASS'}else{'FAIL'})"
    if (!$pass) { Write-Output $outText; Write-Output $errText }
    $process.Dispose()
}
$results | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'results.json') -Encoding ascii
$failed = @($results | Where-Object { !$_.pass }).Count
"SUMMARY scenarios=$($results.Count) passed=$($results.Count - $failed) failed=$failed"
if ($failed) { exit 1 }
exit 0
