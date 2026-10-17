param([switch]$BuildOnly)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output = Join-Path $repo 'build-artifacts/overlay-owned-hwnd-g1b-lifecycle'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$source = Join-Path $repo 'research/prototypes/overlay_owned_hwnd/g1b_unowned_lifecycle.cpp'
$exe = Join-Path $output 'g1b-unowned-lifecycle.exe'
$response = Join-Path $output 'compile.rsp'
$linkResponse = Join-Path $output 'link.rsp'

$lines = @('/nologo','/c','/std:c++latest','/EHsc','/W4','/WX','/utf-8','/O1','/MT',
    ('/Fo"' + (Join-Path $output 'g1b.obj') + '"'), ('"' + $source + '"'))
[IO.File]::WriteAllLines($response, $lines, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllLines($linkResponse, @('/NOLOGO', ('/OUT:"' + $exe + '"'),
    ('"' + (Join-Path $output 'g1b.obj') + '"'), 'user32.lib','ole32.lib',
    'gdi32.lib','d3d11.lib','dxgi.lib','dcomp.lib','dwmapi.lib'),
    [Text.UTF8Encoding]::new($false))

& (Join-Path $PSScriptRoot 'compile_overlay_attachment_fixture.cmd') `
    $response $linkResponse 2>&1 |
    Tee-Object -FilePath (Join-Path $output 'build.log')
$buildCode = $LASTEXITCODE
if ($buildCode -ne 0) { throw "G1-B lifecycle fixture build failed: $buildCode" }
Get-FileHash -LiteralPath $source,$exe -Algorithm SHA256 | Format-List |
    Out-File (Join-Path $output 'identity.txt')
if ($BuildOnly) { exit 0 }

Push-Location -LiteralPath $output
try {
    & $exe 2>&1 | Tee-Object -FilePath (Join-Path $output 'run.log')
    $fixtureCode = $LASTEXITCODE
} finally {
    Pop-Location
}
"G1-B lifecycle build exit code: $buildCode"
"G1-B lifecycle fixture exit code: $fixtureCode"
[IO.File]::WriteAllText((Join-Path $output 'exit-code.txt'),
    "build=$buildCode`r`nfixture=$fixtureCode`r`n")
exit $fixtureCode
