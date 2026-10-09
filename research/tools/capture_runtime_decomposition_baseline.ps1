param([string]$OutputDirectory = 'build-artifacts/runtime-decomposition/baseline')
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Push-Location $repo
try {
    if (Test-Path -LiteralPath $OutputDirectory) { throw 'Baseline directory already exists; do not overwrite evidence.' }
    $output = New-Item -ItemType Directory -Path $OutputDirectory
    $paths = @(git ls-files --cached --others --exclude-standard -- src tests docs locales assets tools external research/README.md backlog/README.md build.cmd test.cmd TESTING_AND_RESEARCH.md README.md) | Sort-Object -Unique
    if ($LASTEXITCODE -ne 0) { throw 'Git source inventory failed.' }
    $manifest = foreach ($path in $paths) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { continue }
        $target = Join-Path $output.FullName ('files/' + $path)
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Copy-Item -LiteralPath $path -Destination $target
        [pscustomobject]@{ Path=$path; SHA256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
    }
    $manifest | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $output.FullName 'source-hashes.csv')
    git rev-parse HEAD | Out-File -Encoding utf8 (Join-Path $output.FullName 'head.txt')
    git status --porcelain=v1 | Out-File -Encoding utf8 (Join-Path $output.FullName 'status.txt')
    git diff --binary --output="$($output.FullName)/unstaged.patch"
    if ($LASTEXITCODE -ne 0) {
        'Unstaged Git patch unavailable; actual preserved file copies and hashes are the worktree baseline.' |
            Out-File -Encoding utf8 (Join-Path $output.FullName 'capture-limitations.txt')
    }
    git diff --cached --binary --output="$($output.FullName)/staged.patch"
    if ($LASTEXITCODE -ne 0) { throw 'Staged patch capture failed.' }
    foreach ($artifact in @('STALKER2CameraTweaks.asi','STALKER2CameraTweaksDiagnostic.asi')) {
        if (Test-Path -LiteralPath $artifact) {
            Copy-Item -LiteralPath $artifact -Destination $output.FullName
            Get-FileHash -LiteralPath $artifact -Algorithm SHA256 | Format-List | Out-File -Append -Encoding utf8 (Join-Path $output.FullName 'existing-artifacts.txt')
        }
    }
    Write-Output "Preserved $($manifest.Count) source files; baseline=$($output.FullName)"
} finally { Pop-Location }
