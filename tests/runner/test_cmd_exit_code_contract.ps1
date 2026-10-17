param(
    [Parameter(Mandatory = $true)]
    [string] $ProbePath,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
$supervisorModule = Join-Path $PSScriptRoot 'process_tree_supervisor.psm1'
Import-Module -Name $supervisorModule -Force
$ProbePath = [IO.Path]::GetFullPath($ProbePath)
if (-not (Test-Path -LiteralPath $ProbePath -PathType Leaf)) {
    throw "Exit-status probe executable is missing: $ProbePath"
}

$tempRoot = $env:TEMP
if ([string]::IsNullOrWhiteSpace($tempRoot)) {
    throw 'TEMP must be set by test.cmd before running the status-contract fixtures.'
}
$fixtureRoot = Join-Path $tempRoot ('test-cmd-status-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)

function Assert-ProcessMarkersStopped {
    param([Parameter(Mandatory = $true)][string[]] $MarkerPaths)
    $ownedPids = @()
    foreach ($markerPath in $MarkerPaths) {
        if (-not (Test-Path -LiteralPath $markerPath -PathType Leaf)) {
            throw "Descendant PID marker was not created: $markerPath"
        }
        $ownedPids += [uint32](Get-Content -LiteralPath $markerPath -Raw)
    }
    $remaining = @($ownedPids | Where-Object { [CameraTweaks.Testing.ProcessTreeSupervisor]::IsProcessRunning($_) })
    if ($remaining.Count -eq 0) { return }

    foreach ($processId in $remaining) {
        Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue
    }
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while ($watch.ElapsedMilliseconds -lt 5000 -and
        @($remaining | Where-Object { [CameraTweaks.Testing.ProcessTreeSupervisor]::IsProcessRunning($_) }).Count -ne 0) {
        Start-Sleep -Milliseconds 50
    }
    $stillRunning = @($remaining | Where-Object { [CameraTweaks.Testing.ProcessTreeSupervisor]::IsProcessRunning($_) })
    if ($stillRunning.Count -ne 0) {
        throw "Test-owned process cleanup remains unverified for PIDs: $($stillRunning -join ', ')."
    }
    throw "Supervisor leaked test-owned descendants; emergency cleanup verified for PIDs: $($remaining -join ', ')."
}

function Invoke-BatchFixture {
    param(
        [Parameter(Mandatory = $true)][string] $Name,
        [Parameter(Mandatory = $true)][string[]] $Lines
    )

    $batchPath = Join-Path $fixtureRoot ($Name + '.cmd')
    [System.IO.File]::WriteAllLines($batchPath, $Lines, $utf8NoBom)
    $result = Invoke-TestCommandProcess -FileName $env:ComSpec `
        -RawArguments ('/d /c "' + $batchPath + '"') -WorkingDirectory $fixtureRoot `
        -TimeoutMilliseconds 30000 -NoWindow
    if ($result.TimedOut) { throw "Batch fixture timed out: $Name; process-tree cleanup verified." }
    [pscustomobject]@{
        ExitCode = $result.ExitCode
        Output = $result.Output
    }
}

try {
    $statuses = @(
        [pscustomobject]@{ Name = 'success'; Code = 0 },
        [pscustomobject]@{ Name = 'positive'; Code = 7 },
        [pscustomobject]@{ Name = 'negative_ntstatus'; Code = -1073741819 }
    )
    foreach ($status in $statuses) {
        $lines = @(
            '@echo off',
            ('"' + $ProbePath + '" ' + $status.Code),
            'if not "%errorlevel%"=="0" goto :fail',
            'echo CONTINUED_AFTER_CHILD',
            'exit /b 0',
            ':fail',
            'set "CODE=%errorlevel%"',
            'echo CAPTURED_FAILURE=%CODE%',
            'exit /b %CODE%'
        )
        $result = Invoke-BatchFixture -Name $status.Name -Lines $lines
        if ($result.ExitCode -ne $status.Code) {
            throw "$($status.Name): expected exit $($status.Code), got $($result.ExitCode). Output: $($result.Output)"
        }
        if ($status.Code -eq 0) {
            if ($result.Output -notmatch 'CONTINUED_AFTER_CHILD') {
                throw 'Successful child did not continue through the success branch.'
            }
        } elseif ($result.Output -match 'CONTINUED_AFTER_CHILD') {
            throw "$($status.Name): failure continued to the next step."
        }
        Write-Output ("status_fixture={0} exit={1} PASS" -f $status.Name, $result.ExitCode)
    }

    $staleOutput = Join-Path $fixtureRoot 'stale-output.cmd'
    [System.IO.File]::WriteAllLines($staleOutput, @('@echo off', 'echo STALE_OUTPUT_EXECUTED'), $utf8NoBom)
    $compileFailureLines = @(
        '@echo off',
        ('"' + $ProbePath + '" 19'),
        'if not "%errorlevel%"=="0" goto :fail',
        ('call "' + $staleOutput + '"'),
        'echo CONTINUED_AFTER_FAILED_COMPILE',
        'exit /b 0',
        ':fail',
        'set "CODE=%errorlevel%"',
        'echo CAPTURED_COMPILE_FAILURE=%CODE%',
        'exit /b %CODE%'
    )
    $compileFailure = Invoke-BatchFixture -Name 'compile_failure_with_stale_output' -Lines $compileFailureLines
    if ($compileFailure.ExitCode -ne 19 -or
        $compileFailure.Output -match 'STALE_OUTPUT_EXECUTED|CONTINUED_AFTER_FAILED_COMPILE') {
        throw "Failed compile did not stop before stale output execution. Exit=$($compileFailure.ExitCode) Output: $($compileFailure.Output)"
    }
    Write-Output 'status_fixture=compile_failure_with_stale_output exit=19 stale_output_not_run PASS'

    $projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
    $runnerScript = Join-Path $PSScriptRoot 'test_harness_runner.ps1'
    $runnerCases = @(
        [pscustomobject]@{ Name = 'registry_runner_negative_ntstatus'; Arguments = @('-1073741819'); Timeout = $null; ExpectedExit = -1073741819; ExpectedOutput = 'FAIL test_cmd_exit_status_probe exit=-1073741819' },
        [pscustomobject]@{ Name = 'registry_runner_crash'; Arguments = @('--crash'); Timeout = $null; ExpectedExit = -1073741819; ExpectedOutput = 'FAIL test_cmd_exit_status_probe exit=-1073741819' },
        [pscustomobject]@{ Name = 'registry_runner_timeout'; Arguments = @('--sleep', '30000'); Timeout = 250; ExpectedExit = 124; ExpectedOutput = 'TIMEOUT test_cmd_exit_status_probe after 250ms; process tree cleanup verified' }
    )
    foreach ($runnerCase in $runnerCases) {
        $registryRelativePath = $fixtureRoot.Substring($projectRoot.Length + 1).Replace('\', '/') + '/' + $runnerCase.Name + '.json'
        $registryAbsolutePath = Join-Path $projectRoot $registryRelativePath
        $runnerRegistry = [ordered]@{
            schema_version = 1
            compiler_contract = @{ output_directory = 'build-artifacts/tests' }
            runner = @(@{ harness = 'test_cmd_exit_status_probe'; arguments = $runnerCase.Arguments })
        }
        [System.IO.File]::WriteAllText($registryAbsolutePath,
            (($runnerRegistry | ConvertTo-Json -Depth 5) + "`n"), $utf8NoBom)
        $previousTimeout = $env:CAMERA_TWEAKS_TEST_TIMEOUT_MS
        if ($null -ne $runnerCase.Timeout) {
            $env:CAMERA_TWEAKS_TEST_TIMEOUT_MS = [string]$runnerCase.Timeout
        } else {
            Remove-Item Env:CAMERA_TWEAKS_TEST_TIMEOUT_MS -ErrorAction SilentlyContinue
        }
        try {
            $runnerResult = Invoke-TestProcessTree -FileName (Get-Command powershell.exe -ErrorAction Stop).Source `
                -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $runnerScript,
                    '-RegistryPath', $registryRelativePath, '-OutputDirectory', $OutputDirectory) -WorkingDirectory $projectRoot `
                -TimeoutMilliseconds 30000 -NoWindow
        } finally {
            if ($null -eq $previousTimeout) { Remove-Item Env:CAMERA_TWEAKS_TEST_TIMEOUT_MS -ErrorAction SilentlyContinue }
            else { $env:CAMERA_TWEAKS_TEST_TIMEOUT_MS = $previousTimeout }
        }
        $runnerOutput = $runnerResult.Output
        if ($runnerResult.TimedOut -or $runnerResult.ExitCode -ne $runnerCase.ExpectedExit -or
            $runnerOutput -notmatch [regex]::Escape($runnerCase.ExpectedOutput)) {
            throw "$($runnerCase.Name): expected exit $($runnerCase.ExpectedExit), got $($runnerResult.ExitCode); timeout=$($runnerResult.TimedOut); cleanup=$($runnerResult.CleanupVerified). Output: $runnerOutput"
        }
        Write-Output ("status_fixture={0} exit={1} cleanup=verified PASS" -f $runnerCase.Name, $runnerResult.ExitCode)
    }

    $treeMarkers = 1..3 | ForEach-Object { Join-Path $fixtureRoot ("descendant-$_.pid") }
    $treeTimeout = Invoke-TestProcessTree -FileName $ProbePath `
        -ArgumentList (@('--spawn-descendants') + $treeMarkers) -WorkingDirectory $fixtureRoot `
        -TimeoutMilliseconds 1500 -CleanupTimeoutMilliseconds 5000 -NoWindow
    if (-not $treeTimeout.TimedOut -or -not $treeTimeout.CleanupVerified) {
        throw "Parent/child/grandchild timeout was not reported with verified cleanup. timeout=$($treeTimeout.TimedOut) cleanup=$($treeTimeout.CleanupVerified)"
    }
    Assert-ProcessMarkersStopped -MarkerPaths $treeMarkers
    Write-Output 'status_fixture=parent_child_grandchild_timeout cleanup=verified PASS'

    $orphanMarkers = 4..6 | ForEach-Object { Join-Path $fixtureRoot ("orphan-descendant-$_.pid") }
    $orphanResult = Invoke-TestProcessTree -FileName $ProbePath `
        -ArgumentList (@('--spawn-orphans') + $orphanMarkers) -WorkingDirectory $fixtureRoot `
        -TimeoutMilliseconds 10000 -CleanupTimeoutMilliseconds 5000 -NoWindow
    if (-not $orphanResult.TimedOut -or -not $orphanResult.DescendantsOutlivedRoot -or
        -not $orphanResult.CleanupVerified) {
        throw "Root-exit descendant containment was not reported correctly. timeout=$($orphanResult.TimedOut) orphan=$($orphanResult.DescendantsOutlivedRoot) cleanup=$($orphanResult.CleanupVerified)"
    }
    Assert-ProcessMarkersStopped -MarkerPaths $orphanMarkers
    Write-Output 'status_fixture=root_exit_with_descendants cleanup=verified PASS'

    $expectedHelperMarkers = 7..9 | ForEach-Object { Join-Path $fixtureRoot ("expected-helper-$_.pid") }
    $expectedHelperResult = Invoke-TestProcessTree -FileName $ProbePath `
        -ArgumentList (@('--spawn-orphans') + $expectedHelperMarkers) -WorkingDirectory $fixtureRoot `
        -TimeoutMilliseconds 10000 -CleanupTimeoutMilliseconds 5000 -NoWindow `
        -ExpectedResidualProcessNames @([IO.Path]::GetFileName($ProbePath), 'conhost.exe')
    if ($expectedHelperResult.TimedOut -or $expectedHelperResult.ExitCode -ne 0 -or
        -not $expectedHelperResult.ExpectedDescendantsCleaned -or
        -not $expectedHelperResult.CleanupVerified) {
        throw "Explicit expected-helper cleanup was not distinguished from a test leak. exit=$($expectedHelperResult.ExitCode) timeout=$($expectedHelperResult.TimedOut) expectedCleanup=$($expectedHelperResult.ExpectedDescendantsCleaned) cleanup=$($expectedHelperResult.CleanupVerified) processes=$($expectedHelperResult.RemainingProcesses)"
    }
    Assert-ProcessMarkersStopped -MarkerPaths $expectedHelperMarkers
    Write-Output 'status_fixture=explicit_expected_helper_cleanup cleanup=verified PASS'

    $cancelMarker = Join-Path $fixtureRoot 'owner-cancel-descendant.pid'
    $cancelResult = Invoke-TestProcessTree -FileName $ProbePath `
        -ArgumentList @('--job-close-descendant', $cancelMarker) -WorkingDirectory $fixtureRoot `
        -TimeoutMilliseconds 10000 -CleanupTimeoutMilliseconds 5000 -NoWindow
    if ($cancelResult.TimedOut -or $cancelResult.ExitCode -ne 0 -or -not $cancelResult.CleanupVerified) {
        throw "Owner cancellation did not terminate its owned descendant. exit=$($cancelResult.ExitCode) timeout=$($cancelResult.TimedOut) cleanup=$($cancelResult.CleanupVerified) output=$($cancelResult.Output)"
    }
    Assert-ProcessMarkersStopped -MarkerPaths @($cancelMarker)
    Write-Output 'status_fixture=owner_job_close_cancellation cleanup=verified PASS'

    $ctestRunner = Join-Path $PSScriptRoot 'ctest_harness_runner.ps1'
    $powershell = (Get-Process -Id $PID).Path
    foreach ($status in @(0, 37, -1073741819)) {
        $wrapped = Invoke-TestProcessTree -FileName $powershell -ArgumentList @(
            '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $ctestRunner,
            '-Executable', $ProbePath, '-ArgumentsJson', ('["' + $status + '"]')) `
            -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 15000 -NoWindow
        if ($wrapped.TimedOut -or -not $wrapped.CleanupVerified -or
            $wrapped.ExitCode -ne $status) {
            throw "CTest wrapper status propagation failed: expected=$status actual=$($wrapped.ExitCode) $($wrapped.Output)"
        }
    }
    $ctestMarkers = 10..12 | ForEach-Object { Join-Path $fixtureRoot ("ctest-$_.pid") }
    $ctestArguments = ConvertTo-Json -InputObject (@('--spawn-descendants') + $ctestMarkers) -Compress
    $wrappedTimeout = Invoke-TestProcessTree -FileName $powershell -ArgumentList @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $ctestRunner,
        '-Executable', $ProbePath, '-ArgumentsJson', $ctestArguments,
        '-TimeoutMilliseconds', '1500') -WorkingDirectory $fixtureRoot `
        -TimeoutMilliseconds 15000 -NoWindow
    if ($wrappedTimeout.TimedOut -or -not $wrappedTimeout.CleanupVerified -or
        $wrappedTimeout.ExitCode -ne 124) {
        throw "CTest wrapper timeout/cleanup failed: $($wrappedTimeout.Output)"
    }
    Assert-ProcessMarkersStopped -MarkerPaths $ctestMarkers
    Write-Output 'status_fixture=ctest_wrapper_nonzero_and_descendant_timeout cleanup=verified PASS'
}
finally {
    Remove-Item -LiteralPath $fixtureRoot -Recurse -Force -ErrorAction SilentlyContinue
}

exit 0
