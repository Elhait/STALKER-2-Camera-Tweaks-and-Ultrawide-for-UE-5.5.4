param([switch]$BuildOnly)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output = Join-Path $repo 'build-artifacts/overlay-owned-hwnd-g0'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$source = Join-Path $repo 'research/prototypes/overlay_owned_hwnd/g0_feasibility.cpp'
$exe = Join-Path $output 'g0-feasibility.exe'
$response = Join-Path $output 'compile.rsp'
$linkResponse = Join-Path $output 'link.rsp'
$lines = @('/nologo','/c','/std:c++latest','/EHsc','/W4','/WX','/utf-8','/O1','/MT',
    ('/Fo"' + (Join-Path $output 'g0.obj') + '"'), ('"' + $source + '"'))
[IO.File]::WriteAllLines($response, $lines, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllLines($linkResponse, @('/NOLOGO', ('/OUT:"' + $exe + '"'),
    ('"' + (Join-Path $output 'g0.obj') + '"'), 'user32.lib','gdi32.lib','ole32.lib',
    'd3d11.lib','dxgi.lib','dcomp.lib','dwmapi.lib'), [Text.UTF8Encoding]::new($false))
& (Join-Path $PSScriptRoot 'compile_overlay_attachment_fixture.cmd') $response $linkResponse 2>&1 |
    Tee-Object -FilePath (Join-Path $output 'build.log')
$buildCode = $LASTEXITCODE
if ($buildCode -ne 0) { throw "G0 build failed: $buildCode" }
Get-FileHash -LiteralPath $source,$exe -Algorithm SHA256 | Format-List |
    Out-File (Join-Path $output 'identity.txt')
if ($BuildOnly) { exit 0 }
Push-Location -LiteralPath $output
try {
    & $exe 2>&1 | Tee-Object -FilePath (Join-Path $output 'run.log')
    $runCode = $LASTEXITCODE
} finally { Pop-Location }
"G0 exit code: $runCode"
[IO.File]::WriteAllText((Join-Path $output 'exit-code.txt'), "build=$buildCode`r`nfixture=$runCode`r`n")
exit $runCode
