param(
    [Parameter(Mandatory = $true)]
    [string] $ProbePath
)

$ErrorActionPreference = 'Stop'
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

function Invoke-BatchFixture {
    param(
        [Parameter(Mandatory = $true)][string] $Name,
        [Parameter(Mandatory = $true)][string[]] $Lines
    )

    $batchPath = Join-Path $fixtureRoot ($Name + '.cmd')
    [System.IO.File]::WriteAllLines($batchPath, $Lines, $utf8NoBom)
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $env:ComSpec
    $startInfo.Arguments = '/d /c "' + $batchPath + '"'
    $startInfo.WorkingDirectory = $fixtureRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    if (-not $process.Start()) { throw "Could not start fixture: $Name" }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    [pscustomobject]@{
        ExitCode = $process.ExitCode
        Output = $stdoutTask.Result + $stderrTask.Result
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
        [pscustomobject]@{ Name = 'registry_runner_timeout'; Arguments = @('--sleep', '30000'); Timeout = 250; ExpectedExit = 124; ExpectedOutput = 'TIMEOUT test_cmd_exit_status_probe after 250ms' }
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
        $runnerStartInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $runnerStartInfo.FileName = (Get-Command powershell.exe -ErrorAction Stop).Source
        $runnerStartInfo.Arguments = '-NoProfile -ExecutionPolicy Bypass -File "' + $runnerScript +
            '" -RegistryPath "' + $registryRelativePath + '"'
        $runnerStartInfo.WorkingDirectory = $projectRoot
        $runnerStartInfo.UseShellExecute = $false
        $runnerStartInfo.CreateNoWindow = $true
        $runnerStartInfo.RedirectStandardOutput = $true
        $runnerStartInfo.RedirectStandardError = $true
        if ($null -ne $runnerCase.Timeout) {
            $runnerStartInfo.EnvironmentVariables['CAMERA_TWEAKS_TEST_TIMEOUT_MS'] = [string]$runnerCase.Timeout
        }
        $runnerProcess = [System.Diagnostics.Process]::new()
        $runnerProcess.StartInfo = $runnerStartInfo
        if (-not $runnerProcess.Start()) { throw "Could not start runner fixture: $($runnerCase.Name)" }
        $runnerStdout = $runnerProcess.StandardOutput.ReadToEndAsync()
        $runnerStderr = $runnerProcess.StandardError.ReadToEndAsync()
        if (-not $runnerProcess.WaitForExit(30000)) {
            $runnerProcess.Kill()
            $runnerProcess.WaitForExit(5000) | Out-Null
            throw "Runner fixture itself did not terminate: $($runnerCase.Name)"
        }
        $runnerOutput = $runnerStdout.Result + $runnerStderr.Result
        if ($runnerProcess.ExitCode -ne $runnerCase.ExpectedExit -or
            $runnerOutput -notmatch [regex]::Escape($runnerCase.ExpectedOutput)) {
            throw "$($runnerCase.Name): expected exit $($runnerCase.ExpectedExit), got $($runnerProcess.ExitCode). Output: $runnerOutput"
        }
        Write-Output ("status_fixture={0} exit={1} PASS" -f $runnerCase.Name, $runnerProcess.ExitCode)
    }
}
finally {
    Remove-Item -LiteralPath $fixtureRoot -Recurse -Force -ErrorAction SilentlyContinue
}

exit 0
