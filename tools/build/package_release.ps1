param(
    [Parameter(Mandatory = $true)][string]$ReleaseDirectory,
    [Parameter(Mandatory = $true)][string]$OutputZip
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
$releaseRoot = [System.IO.Path]::GetFullPath($ReleaseDirectory)
$archivePath = [System.IO.Path]::GetFullPath($OutputZip)
$requiredFiles = @(
    'STALKER2CameraTweaks.asi',
    'README.txt',
    'LICENSE.md',
    'THIRD_PARTY_NOTICES.md'
)

foreach ($name in $requiredFiles) {
    $sourcePath = Join-Path $releaseRoot $name
    if (-not [System.IO.File]::Exists($sourcePath)) {
        throw "Required release file is missing: $sourcePath"
    }
}
if ([System.IO.Path]::GetExtension($archivePath) -ine '.zip') {
    throw "Output must be a .zip archive: $archivePath"
}
if ($requiredFiles | Where-Object { [System.IO.Path]::GetFullPath((Join-Path $releaseRoot $_)) -ieq $archivePath }) {
    throw 'Output archive cannot replace a release input.'
}

$parent = [System.IO.Path]::GetDirectoryName($archivePath)
[System.IO.Directory]::CreateDirectory($parent) | Out-Null
$created = $false
$validated = $false
$fileStream = $null
$archive = $null
try {
    $fileStream = [System.IO.File]::Open($archivePath,
        [System.IO.FileMode]::CreateNew,
        [System.IO.FileAccess]::ReadWrite,
        [System.IO.FileShare]::None)
    $created = $true
    $archive = [System.IO.Compression.ZipArchive]::new(
        $fileStream, [System.IO.Compression.ZipArchiveMode]::Create, $true)
    foreach ($name in $requiredFiles) {
        $entry = $archive.CreateEntry($name, [System.IO.Compression.CompressionLevel]::Optimal)
        $input = [System.IO.File]::OpenRead((Join-Path $releaseRoot $name))
        $output = $entry.Open()
        try { $input.CopyTo($output) }
        finally { $output.Dispose(); $input.Dispose() }
    }
    $archive.Dispose()
    $archive = $null
    $fileStream.Dispose()
    $fileStream = $null

    $readStream = [System.IO.File]::OpenRead($archivePath)
    try {
        $check = [System.IO.Compression.ZipArchive]::new(
            $readStream, [System.IO.Compression.ZipArchiveMode]::Read, $false)
        try {
            $actual = @($check.Entries | ForEach-Object FullName | Sort-Object)
            $expected = @($requiredFiles | Sort-Object)
            if (@($actual | Where-Object { $_ -match '(?i)(^|/)STALKER2CameraTweaks\.ini$' }).Count -ne 0 -or
                ($actual -join "`n") -cne ($expected -join "`n")) {
                throw "Release archive contents failed allowlist validation: $($actual -join ', ')"
            }
            $asiEntry = $check.GetEntry('STALKER2CameraTweaks.asi')
            $asiStream = $asiEntry.Open()
            $sha256 = [System.Security.Cryptography.SHA256]::Create()
            try {
                $packedHash = [System.BitConverter]::ToString($sha256.ComputeHash($asiStream)).Replace('-', '')
                $sourceStream = [System.IO.File]::OpenRead(
                    (Join-Path $releaseRoot 'STALKER2CameraTweaks.asi'))
                try {
                    $sourceHash = [System.BitConverter]::ToString(
                        $sha256.ComputeHash($sourceStream)).Replace('-', '')
                }
                finally { $sourceStream.Dispose() }
                if ($packedHash -cne $sourceHash) {
                    throw 'Archived ASI hash does not match the release input.'
                }
            }
            finally { $sha256.Dispose(); $asiStream.Dispose() }
        }
        finally { $check.Dispose() }
    }
    finally { $readStream.Dispose() }
    $validated = $true
    Write-Output "Release archive created and verified: $archivePath"
    Write-Output "Archive inventory: ASI, README.txt, LICENSE, THIRD_PARTY_NOTICES; INI excluded; ASI SHA-256=$sourceHash."
}
finally {
    if ($archive) { $archive.Dispose() }
    if ($fileStream) { $fileStream.Dispose() }
    if ($created -and -not $validated -and [System.IO.File]::Exists($archivePath)) {
        Remove-Item -LiteralPath $archivePath -Force
    }
}
