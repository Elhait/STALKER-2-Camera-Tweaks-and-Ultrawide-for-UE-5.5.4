$ErrorActionPreference = 'Stop'

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$publishScript = Join-Path $projectRoot 'tools/build/publish_asi.ps1'
$powershell = (Get-Command powershell.exe -ErrorAction Stop).Source
$tempRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build-artifacts/tests/temp'))
[IO.Directory]::CreateDirectory($tempRoot) | Out-Null
$workRoot = [IO.Path]::GetFullPath((Join-Path $tempRoot (
    'publish-asi-contract-{0}-{1}' -f $PID, [guid]::NewGuid().ToString('N'))))
$tempPrefix = $tempRoot.TrimEnd('\') + '\'
if (-not $workRoot.StartsWith($tempPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing publish fixture outside the owned test temp directory: $workRoot"
}

function Assert([bool] $Condition, [string] $Message) {
    if (-not $Condition) { throw $Message }
}

function Get-Hash([string] $Path) {
    $sha = [Security.Cryptography.SHA256]::Create()
    $stream = [IO.File]::OpenRead($Path)
    try { return [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
    finally { $stream.Dispose(); $sha.Dispose() }
}

function Invoke-Publisher([string] $Source, [string] $Destination) {
    $stdoutPath = Join-Path $workRoot (([guid]::NewGuid().ToString('N')) + '.stdout')
    $stderrPath = Join-Path $workRoot (([guid]::NewGuid().ToString('N')) + '.stderr')
    $arguments = '-NoProfile -ExecutionPolicy Bypass -File "{0}" -Source "{1}" -Destination "{2}"' -f `
        $publishScript, $Source, $Destination
    $process = Start-Process -FilePath $powershell -ArgumentList $arguments `
        -PassThru -Wait -WindowStyle Hidden -RedirectStandardOutput $stdoutPath `
        -RedirectStandardError $stderrPath
    $output = if ([IO.File]::Exists($stdoutPath)) {
        [IO.File]::ReadAllText($stdoutPath)
    } else { '' }
    if ([IO.File]::Exists($stderrPath)) {
        $output += [IO.File]::ReadAllText($stderrPath)
    }
    return [pscustomobject]@{ ExitCode = $process.ExitCode; Output = $output }
}

try {
    [IO.Directory]::CreateDirectory($workRoot) | Out-Null
    $source = Join-Path $workRoot 'build-output.asi'
    $destinationDirectory = Join-Path $workRoot 'game-plugin'
    $destination = Join-Path $destinationDirectory 'STALKER2CameraTweaks.asi'
    $sourceBytes = [byte[]](3, 17, 31, 47, 63, 79, 95, 111, 127)
    [IO.File]::WriteAllBytes($source, $sourceBytes)
    [IO.Directory]::CreateDirectory($destinationDirectory) | Out-Null
    [IO.File]::WriteAllBytes($destination, [byte[]](9, 8, 7, 6))

    $publish = Invoke-Publisher $source $destination
    Assert ($publish.ExitCode -eq 0 -and $publish.Output -match 'Published ') `
        "Publish failed: exit=$($publish.ExitCode) output=$($publish.Output)"
    Assert ((Get-Hash $source) -ceq (Get-Hash $destination)) `
        'Published destination bytes do not match the source ASI.'
    Assert (@(Get-ChildItem -LiteralPath $destinationDirectory -Filter '*.tmp' -File).Count -eq 0) `
        'Successful publish left a staging file behind.'
    Write-Output 'publish_asi_atomic_replace_and_hash=PASS'

    $destinationWrite = [IO.File]::GetLastWriteTimeUtc($destination)
    $noop = Invoke-Publisher $source $destination
    Assert ($noop.ExitCode -eq 0 -and $noop.Output -match 'already current') `
        "Identical publish was not treated as a no-op: exit=$($noop.ExitCode) output=$($noop.Output)"
    Assert ([IO.File]::GetLastWriteTimeUtc($destination) -eq $destinationWrite) `
        'No-op publish rewrote the already-current destination.'
    Write-Output 'publish_asi_identical_input_is_noop=PASS'

    $missingSource = Join-Path $workRoot 'missing.asi'
    $beforeMissing = Get-Hash $destination
    $missing = Invoke-Publisher $missingSource $destination
    Assert ($missing.ExitCode -ne 0 -and (Get-Hash $destination) -ceq $beforeMissing) `
        'Missing source was accepted or changed the existing destination.'
    Assert (@(Get-ChildItem -LiteralPath $destinationDirectory -Filter '*.tmp' -File).Count -eq 0) `
        'Missing-source failure left a staging file behind.'
    Write-Output 'publish_asi_missing_source_preserves_destination=PASS'

    $normalizedDestination = $destination.ToUpperInvariant()
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $mutexHash = [BitConverter]::ToString(
            $sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($normalizedDestination))).Replace('-', '')
    }
    finally { $sha.Dispose() }
    $mutex = [Threading.Mutex]::new($false, "Local\STALKER2CameraTweaks.Publish.$mutexHash")
    $ownsMutex = $false
    try {
        $ownsMutex = $mutex.WaitOne(0)
        Assert $ownsMutex 'Could not acquire the destination mutex fixture.'
        $beforeLocked = Get-Hash $destination
        $locked = Invoke-Publisher $source $destination
        Assert ($locked.ExitCode -ne 0 -and $locked.Output -match 'Another build is publishing') `
            "Concurrent publish was not rejected: exit=$($locked.ExitCode) output=$($locked.Output)"
        Assert ((Get-Hash $destination) -ceq $beforeLocked) `
            'Rejected concurrent publish changed the destination.'
        Assert (@(Get-ChildItem -LiteralPath $destinationDirectory -Filter '*.tmp' -File).Count -eq 0) `
            'Rejected concurrent publish created a staging file.'
    }
    finally {
        if ($ownsMutex) { $mutex.ReleaseMutex() }
        $mutex.Dispose()
    }
    Write-Output 'publish_asi_destination_mutex_serializes_writers=PASS'
    Write-Output 'publish_asi_contract=PASS'
}
finally {
    if ([IO.Directory]::Exists($workRoot)) {
        Remove-Item -LiteralPath $workRoot -Recurse -Force
    }
}
