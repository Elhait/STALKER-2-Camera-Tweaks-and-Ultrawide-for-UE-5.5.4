param([string] $BuildDirectory = 'build-artifacts/cm/test-suite')

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = [IO.Path]::GetFullPath((Join-Path $root $BuildDirectory))
Import-Module (Join-Path $PSScriptRoot 'process_tree_supervisor.psm1') -Force

# Reuse the canonical Ninja profile's non-mutated objects. The Authority
# reducers and cinematic activation are inline in the transaction test TU.
$entries = ConvertFrom-Json -InputObject (Get-Content -Raw -LiteralPath (Join-Path $build 'compile_commands.json'))
$entry = $entries | Where-Object { $_.file -match 'tests[/\\]plugin[/\\]camera_writer_transaction_harness\.cpp$' } |
    Select-Object -First 1
$ninja = Get-Content -Raw -LiteralPath (Join-Path $build 'build.ninja')
$target = [regex]::Match($ninja, '(?m)^build tests[/\\]camera_writer_transaction_harness\.exe: CXX_EXECUTABLE_LINKER__[^\s]+ ([^\r\n]+)\r?\n( +[^\r\n]+\r?\n)+')
if ($entry.file -isnot [string] -or !$target.Success) { throw 'Configure and build the canonical Ninja camera_writer_transaction_harness first.' }
$vsRoot = [regex]::Match($entry.command, '^"(.+?)\\VC\\Tools\\').Groups[1].Value
if (!$vsRoot) { throw 'Could not resolve the canonical MSVC installation.' }
$vsDevCmd = Join-Path $vsRoot 'Common7/Tools/VsDevCmd.bat'
$objects = @($target.Groups[1].Value.Trim() -split '\s+' | Where-Object { $_ -like '*.obj' })
$libs = [regex]::Match($target.Value, '(?m)^\s*LINK_LIBRARIES = ([^\r\n]+)').Groups[1].Value
$flags = [regex]::Match($target.Value, '(?m)^\s*LINK_FLAGS = ([^\r\n]+)').Groups[1].Value
if (!$objects.Count -or !$libs -or $objects[0] -notmatch 'camera_writer_transaction_harness\.cpp\.obj$') {
    throw 'Canonical link inputs did not match the intended target.'
}
$sandbox = Join-Path $root ('build-artifacts/callback-mutations-' + [Guid]::NewGuid().ToString('N'))
$shadow = Join-Path $sandbox 'shadow'
New-Item -Path (Join-Path $shadow 'tests') -ItemType Directory | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'src') -Destination (Join-Path $shadow 'src') -Recurse
Copy-Item -LiteralPath (Join-Path $root 'tests/plugin') -Destination (Join-Path $shadow 'tests/plugin') -Recurse
$shadowSource = Join-Path $shadow 'tests/plugin/camera_writer_transaction_harness.cpp'
$core = 'src/plugin/lifecycle_authority_core.hpp'
$mutations = @(
    @{ name = 'repeat-dialogue'; path = $core
       pattern = 'native\.dialogueCorrectionEnabled = mapProviderContinuation &&\s*tracker\.BlocksGameplayBaselineAdmission\([^;]+\);'
       substitute = 'native.dialogueCorrectionEnabled = mapProviderContinuation;'
       expected = 'event/departing-writer' },
    @{ name = 'late-recovery'; path = $core
       pattern = 'if \(event\.nativeBoundary && event\.engine && !physicalRecovery && !nativeProviderOutput &&'
       substitute = 'if (false && event.nativeBoundary && event.engine && !physicalRecovery && !nativeProviderOutput &&'
       expected = 'event/recovery-writer' },
    @{ name = 'foreign-writer'; path = $core
       pattern = '&& raw\.thread != scope->second\.thread\)\s*return receipt;'
       substitute = '&& false && raw.thread != scope->second.thread) return receipt;'
       expected = 'event/foreign-writer' },
    @{ name = 'foreign-setter'; path = $core
       pattern = 'if \(event\.nativeBoundary && scope\.providerDialogueOwned &&'
       substitute = 'if (false && event.nativeBoundary && scope.providerDialogueOwned &&'
       expected = 'unrelated-provider-setter' },
    @{ name = 'generation-zero'; path = 'src/cinematics/cinematic_enter_activation.hpp'
       pattern = 'dialogueStart_\.dialogueGeneration < generation && dialogueStart_\.writerSequence != 0 &&'
       substitute = 'dialogueStart_.dialogueGeneration != 0 && dialogueStart_.writerSequence != 0 &&'
       expected = 'event/blend-start-writer' }
)
function Invoke-Variant($name, $expected)
{
    $dir = Join-Path $sandbox $name
    New-Item -Path $dir -ItemType Directory | Out-Null
    $exe = Join-Path $dir 'probe.exe'
    $obj = Join-Path $dir 'probe.obj'
    $pdb = Join-Path $dir 'probe.pdb'
    $compile = $entry.command.Replace($entry.file, $shadowSource)
    if ($compile -eq $entry.command) { throw 'Could not substitute the staged harness source.' }
    $compile = ($compile -replace '/Fo\S+', ('/Fo"{0}"' -f $obj)) -replace ' /Fd\S+', ''
    # Link from the canonical working directory to keep object paths short.
    $link = 'link.exe /NOLOGO "{0}" {1} /OUT:"{2}" /PDB:"{3}" {4} {5}' -f (
        $obj, (($objects | Select-Object -Skip 1) -join ' '), $exe, $pdb, $flags, $libs)
    Set-Content -LiteralPath (Join-Path $dir 'compile.cmd') -Encoding ascii -Value @(
        '@echo off', ('call "{0}" -arch=x64 -host_arch=x64 >nul' -f $vsDevCmd),
        'if errorlevel 1 exit /b %errorlevel%', $compile, 'if errorlevel 1 exit /b %errorlevel%',
        ('pushd "{0}"' -f $build), 'if errorlevel 1 exit /b %errorlevel%', $link, 'exit /b %errorlevel%')
    $compiled = Invoke-TestCommandProcess -FileName $env:ComSpec -RawArguments '/d /c compile.cmd' -WorkingDirectory $dir -TimeoutMilliseconds 120000 -NoWindow -ExpectedResidualProcessNames @('vctip.exe')
    Set-Content -LiteralPath (Join-Path $dir 'compile.log') -Value $compiled.Output
    if ($compiled.ExitCode) { throw "Could not build $name. See $dir/compile.log" }

    $result = Invoke-TestProcessTree -FileName $exe -ArgumentList '--authority-generated-events' -WorkingDirectory $dir -TimeoutMilliseconds 60000 -NoWindow
    Set-Content -LiteralPath (Join-Path $dir 'result.log') -Value $result.Output
    if (!$expected) {
        if ($result.ExitCode -ne 0 -or $result.Output -notmatch 'M3 native callbacks:') {
            throw "Unmutated control failed. See $dir/result.log"
        }
        Write-Host "PASS $name"
        return
    }
    if ($result.ExitCode -ne 1 -or $result.Output -notmatch 'M3 counterexample generated' -or
        $result.Output -notmatch [regex]::Escape($expected)) {
        throw "Mutation $name did not produce the expected generated counterexample. See $dir/result.log"
    }
    Write-Host "KILLED $name"
    Write-Host $result.Output.Trim()
}
Write-Host "Callback mutation artifacts: $sandbox"
Invoke-Variant 'control' ''
foreach ($mutation in $mutations) {
    $path = Join-Path $shadow $mutation.path
    $source = [IO.File]::ReadAllText($path)
    $pattern = [regex]$mutation.pattern
    if ($pattern.Matches($source).Count -ne 1) { throw "Mutation anchor did not resolve uniquely: $($mutation.name)" }
    try {
        [IO.File]::WriteAllText($path, $pattern.Replace($source, $mutation.substitute, 1))
        Invoke-Variant $mutation.name $mutation.expected
    } finally {
        [IO.File]::WriteAllText($path, $source)
    }
}
Write-Host "Mutation score: $($mutations.Count)/$($mutations.Count). Process cleanup verified."
