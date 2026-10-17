param([ValidateSet('G5','F4','Fixed','Current')][string]$Candidate='F4', [switch]$BuildOnly, [switch]$RunOnly,
    [switch]$ReusePresentationObjects,
    [ValidateSet('off','on')][string]$Diagnostics='off', [string]$ConfigPath,
    [ValidateSet('normal','raise-host')][string]$Scenario='normal')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$tree=if($Candidate -eq 'G5') {'build-artifacts/g2-g3-candidate/cmake/production-tOFF-dOFF'} else {'build-artifacts/cm/production-tOFF-dOFF'}
if($Candidate -in @('F4','Fixed') -and (Test-Path (Join-Path $repo 'build-artifacts/invisible-overlay-desktop/F4/preserved-inputs'))) {
    $tree='build-artifacts/invisible-overlay-desktop/F4/preserved-inputs'
}
$output=Join-Path $repo ('build-artifacts/invisible-overlay-desktop/'+$Candidate)
[IO.Directory]::CreateDirectory($output) | Out-Null
Import-Module (Join-Path $repo 'tests/runner/process_tree_supervisor.psm1') -Force
$registry=Get-Content (Join-Path $repo 'tools/build/production_sources.json') -Raw | ConvertFrom-Json
$objects=foreach($entry in $registry.sources) {
    if($entry.role -eq 'dll_entry') {continue}
    $path=Join-Path $repo ($tree+'/CMakeFiles/STALKER2CameraTweaks.dir/'+$entry.path+'.obj')
    if($Candidate -eq 'Fixed' -and $entry.path -in @('src/overlay/presenter/renderer_runtime.cpp','src/overlay/composition/composition_runtime.cpp')) {
        $path=Join-Path $output ('production-obj/'+[IO.Path]::GetFileNameWithoutExtension($entry.path)+'.obj')
    }
    if(!(Test-Path -LiteralPath $path) -and !($Candidate -eq 'Fixed' -and $path.Contains('production-obj'))) {throw "Missing preserved object: $path"}
    '"'+$path+'"'
}
$exe=Join-Path $output 'production-desktop-host.exe'
$obj=Join-Path $output 'production-desktop-host.obj'
$compile=Join-Path $output 'compile.rsp'; $link=Join-Path $output 'link.rsp'
$utf8=[Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllLines($compile,@('/nologo','/c','/std:c++latest','/EHsc','/W4','/utf-8','/O1','/MT','/DNDEBUG',
    ('/Fo"'+$obj+'"'),('"'+(Join-Path $repo 'research/prototypes/overlay_owned_hwnd/production_desktop_host.cpp')+'"')),$utf8)
[IO.File]::WriteAllLines($link,@('/NOLOGO','/MANIFEST:NO',('/OUT:"'+$exe+'"'),('"'+$obj+'"'))+$objects+@(
    ('"'+(Join-Path $repo ($tree+'/resources/localization_resources.res'))+'"'),
    'user32.lib','gdi32.lib','ole32.lib','psapi.lib','bcrypt.lib','dxgi.lib','d3d11.lib','dcomp.lib','d3dcompiler.lib','dwmapi.lib'),$utf8)
$raw='/d /s /c ""'+(Join-Path $PSScriptRoot 'compile_overlay_attachment_fixture.cmd')+'" "'+$compile+'" "'+$link+'""'
if(!$RunOnly) {
if($Candidate -eq 'Fixed' -and !$ReusePresentationObjects) {
    $fixedObjects=Join-Path $output 'production-obj'
    [IO.Directory]::CreateDirectory($fixedObjects) | Out-Null
    $fixedResponse=Join-Path $output 'production-compile.rsp'
    [IO.File]::WriteAllLines($fixedResponse,@('/nologo','/c','/std:c++latest','/EHsc','/W4','/utf-8','/O1','/MT',
        '/DNDEBUG','/DOVERLAY_COMBINED','/DOVERLAY_PRODUCTION','/DOVERLAY_SETTINGS_FRONTEND','/DOVERLAY_STARTUP_JOURNAL',
        ('/I"'+$repo+'"'),('/I"'+(Join-Path $repo 'external/imgui')+'"'),
        ('/I"'+(Join-Path $repo 'external/imgui/backends')+'"'),('/Fo"'+$fixedObjects+'/"'),
        ('"'+(Join-Path $repo 'src/overlay/presenter/renderer_runtime.cpp')+'"'),
        ('"'+(Join-Path $repo 'src/overlay/composition/composition_runtime.cpp')+'"')),$utf8)
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vsdev=& $vswhere -latest -products '*' -version '[17.14,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'Common7/Tools/VsDevCmd.bat'
    if(!$vsdev) {throw 'MSVC x64 environment missing'}
    $fixedRaw='/d /s /c ""'+$vsdev+'" -arch=x64 -host_arch=x64 >nul && cl.exe @"'+$fixedResponse+'""'
    $fixedBuild=Invoke-TestCommandProcess -FileName $env:ComSpec -RawArguments $fixedRaw -WorkingDirectory $repo -TimeoutMilliseconds 120000 -ExpectedResidualProcessNames @('vctip.exe') -MaximumOutputCharacters 65536
    [IO.File]::WriteAllText((Join-Path $output 'production-build.log'),$fixedBuild.Output)
    if($fixedBuild.ExitCode -ne 0 -or $fixedBuild.TimedOut -or !$fixedBuild.CleanupVerified) {throw $fixedBuild.Output}
}
$built=Invoke-TestCommandProcess -FileName $env:ComSpec -RawArguments $raw -WorkingDirectory $repo -TimeoutMilliseconds 120000 -ExpectedResidualProcessNames @('vctip.exe') -MaximumOutputCharacters 65536
[IO.File]::WriteAllText((Join-Path $output 'build.log'),$built.Output)
$built | Select-Object ExitCode,TimedOut,CleanupVerified | ConvertTo-Json -Compress
if($built.ExitCode -ne 0 -or $built.TimedOut -or !$built.CleanupVerified) {throw $built.Output}
}
if($BuildOnly) {exit 0}
$runDirectory=Join-Path $output ((Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+$Diagnostics)
[IO.Directory]::CreateDirectory($runDirectory) | Out-Null
$fixtureArguments=@($Diagnostics)
if($ConfigPath) {
    Copy-Item -LiteralPath $ConfigPath -Destination (Join-Path $output 'STALKER2CameraTweaks.ini')
    $fixtureArguments+='config'
} else {$fixtureArguments+='default'}
$fixtureArguments+=$Scenario
$run=Invoke-TestProcessTree -FileName $exe -ArgumentList $fixtureArguments -WorkingDirectory $runDirectory -TimeoutMilliseconds 25000 -MaximumOutputCharacters 65536
[IO.File]::WriteAllText((Join-Path $runDirectory 'run.log'),$run.Output)
$run | Select-Object ExitCode,TimedOut,CleanupVerified,ExpectedDescendantsCleaned | ConvertTo-Json | Set-Content (Join-Path $runDirectory 'result.json')
$productionLog=Join-Path $output 'STALKER2CameraTweaksOverlay.log'
if(Test-Path -LiteralPath $productionLog) {Copy-Item -LiteralPath $productionLog -Destination $runDirectory}
Write-Output ('Evidence: '+$runDirectory)
$run.Output
$run | Select-Object ExitCode,TimedOut,CleanupVerified | ConvertTo-Json -Compress
if($run.ExitCode -ne 0 -or $run.TimedOut -or !$run.CleanupVerified) {exit 1}
exit 0
