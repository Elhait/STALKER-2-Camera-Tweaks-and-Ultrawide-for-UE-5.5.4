$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$packageScript = Join-Path $projectRoot 'tools/build/package_release.ps1'
$powershell = (Get-Command powershell.exe -ErrorAction Stop).Source
$tempRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build-artifacts/tests/temp'))
[IO.Directory]::CreateDirectory($tempRoot) | Out-Null
$workRoot = [IO.Path]::GetFullPath((Join-Path $tempRoot (
    'release-package-contract-{0}-{1}' -f $PID, [guid]::NewGuid().ToString('N'))))
$tempPrefix = $tempRoot.TrimEnd('\') + '\'
if (-not $workRoot.StartsWith($tempPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing release-package fixture outside the owned test temp directory: $workRoot"
}

function Assert([bool] $Condition, [string] $Message) {
    if (-not $Condition) { throw $Message }
}

function Invoke-Packager([string] $ReleaseDirectory, [string] $OutputZip) {
    $stdoutPath = Join-Path $workRoot (([guid]::NewGuid().ToString('N')) + '.stdout')
    $stderrPath = Join-Path $workRoot (([guid]::NewGuid().ToString('N')) + '.stderr')
    $arguments = '-NoProfile -ExecutionPolicy Bypass -File "{0}" -ReleaseDirectory "{1}" -OutputZip "{2}"' -f `
        $packageScript, $ReleaseDirectory, $OutputZip
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

function Get-Sha256([System.IO.Stream] $Stream) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try { return [Convert]::ToBase64String($sha.ComputeHash($Stream)) }
    finally { $sha.Dispose() }
}

function Test-BytesEqual([byte[]] $Left, [byte[]] $Right) {
    if ($Left.Length -ne $Right.Length) { return $false }
    for ($index = 0; $index -lt $Left.Length; ++$index) {
        if ($Left[$index] -ne $Right[$index]) { return $false }
    }
    return $true
}

try {
    [IO.Directory]::CreateDirectory($workRoot) | Out-Null
    $releaseRoot = Join-Path $workRoot 'release-input'
    [IO.Directory]::CreateDirectory($releaseRoot) | Out-Null
    $requiredFiles = @('STALKER2CameraTweaks.asi', 'README.txt',
        'LICENSE.md', 'THIRD_PARTY_NOTICES.md')
    $payloads = @{
        'STALKER2CameraTweaks.asi' = [byte[]](0, 17, 34, 51, 68, 85, 102, 119)
        'README.txt' = [Text.Encoding]::UTF8.GetBytes("fixture README`n")
        'LICENSE.md' = [Text.Encoding]::UTF8.GetBytes("fixture license`n")
        'THIRD_PARTY_NOTICES.md' = [Text.Encoding]::UTF8.GetBytes("fixture notices`n")
        'STALKER2CameraTweaks.ini' = [Text.Encoding]::UTF8.GetBytes("user data must stay out of the archive`n")
    }
    foreach ($name in $payloads.Keys) {
        [IO.File]::WriteAllBytes((Join-Path $releaseRoot $name), $payloads[$name])
    }

    $archivePath = Join-Path $workRoot 'valid-release.zip'
    $valid = Invoke-Packager $releaseRoot $archivePath
    Assert ($valid.ExitCode -eq 0) "Valid package failed: exit=$($valid.ExitCode) output=$($valid.Output)"
    Assert ([IO.File]::Exists($archivePath)) 'Valid package did not create the requested archive.'
    Assert ($valid.Output -match 'INI excluded' -and $valid.Output -match 'ASI SHA-256=') `
        'Successful package did not report the verified inventory and ASI hash.'

    $archiveStream = [IO.File]::OpenRead($archivePath)
    try {
        $archive = [IO.Compression.ZipArchive]::new($archiveStream,
            [IO.Compression.ZipArchiveMode]::Read, $false)
        try {
            $actualNames = @($archive.Entries | ForEach-Object FullName | Sort-Object)
            $expectedNames = @($requiredFiles | Sort-Object)
            Assert (($actualNames -join "`n") -ceq ($expectedNames -join "`n")) `
                "Archive allowlist mismatch: $($actualNames -join ', ')"
            Assert ($null -eq $archive.GetEntry('STALKER2CameraTweaks.ini')) `
                'User INI must not be included in the release archive.'
            $asiStream = $archive.GetEntry('STALKER2CameraTweaks.asi').Open()
            try { $packedHash = Get-Sha256 $asiStream }
            finally { $asiStream.Dispose() }
            $sourceStream = [IO.File]::OpenRead((Join-Path $releaseRoot 'STALKER2CameraTweaks.asi'))
            try { $sourceHash = Get-Sha256 $sourceStream }
            finally { $sourceStream.Dispose() }
            Assert ($packedHash -ceq $sourceHash) 'Packed ASI bytes do not match the release input.'
        }
        finally { $archive.Dispose() }
    }
    finally { $archiveStream.Dispose() }
    Write-Output 'release_package_valid_allowlist_ini_exclusion_and_asi_hash=PASS'

    $sentinelPath = Join-Path $workRoot 'already-existing.zip'
    $sentinel = [Text.Encoding]::UTF8.GetBytes('do-not-replace-existing-archive')
    [IO.File]::WriteAllBytes($sentinelPath, $sentinel)
    $collision = Invoke-Packager $releaseRoot $sentinelPath
    Assert ($collision.ExitCode -ne 0) 'Packager unexpectedly replaced an existing archive.'
    Assert (Test-BytesEqual -Left ([IO.File]::ReadAllBytes($sentinelPath)) -Right $sentinel) `
        'Existing archive contents changed after CreateNew rejected the package.'
    Write-Output 'release_package_existing_archive_preserved=PASS'

    $badExtension = Join-Path $workRoot 'invalid-release.bin'
    $extensionResult = Invoke-Packager $releaseRoot $badExtension
    Assert ($extensionResult.ExitCode -ne 0 -and -not [IO.File]::Exists($badExtension)) `
        'Invalid output extension was accepted or left an output artifact.'
    Write-Output 'release_package_invalid_extension_rejected=PASS'

    $missingRoot = Join-Path $workRoot 'missing-input'
    [IO.Directory]::CreateDirectory($missingRoot) | Out-Null
    foreach ($name in $requiredFiles | Where-Object { $_ -ne 'THIRD_PARTY_NOTICES.md' }) {
        [IO.File]::WriteAllBytes((Join-Path $missingRoot $name), $payloads[$name])
    }
    $missingOutput = Join-Path $workRoot 'missing-input.zip'
    $missing = Invoke-Packager $missingRoot $missingOutput
    Assert ($missing.ExitCode -ne 0 -and -not [IO.File]::Exists($missingOutput)) `
        'Missing required release input was accepted or left an output artifact.'
    Write-Output 'release_package_missing_required_input_rejected=PASS'

    $inputCollision = Invoke-Packager $releaseRoot (Join-Path $releaseRoot 'STALKER2CameraTweaks.asi')
    $sourceAfterCollision = [IO.File]::ReadAllBytes(
        (Join-Path $releaseRoot 'STALKER2CameraTweaks.asi'))
    $sourcePayload = [byte[]]$payloads['STALKER2CameraTweaks.asi']
    Assert ($inputCollision.ExitCode -ne 0 -and
        (Test-BytesEqual -Left $sourceAfterCollision -Right $sourcePayload)) `
        'Output collision with a required release input changed or accepted the source.'
    Write-Output 'release_package_input_path_rejected_without_source_damage=PASS'

    Write-Output 'release_package_contract=PASS'
}
finally {
    if ([IO.Directory]::Exists($workRoot)) {
        Remove-Item -LiteralPath $workRoot -Recurse -Force
    }
}
