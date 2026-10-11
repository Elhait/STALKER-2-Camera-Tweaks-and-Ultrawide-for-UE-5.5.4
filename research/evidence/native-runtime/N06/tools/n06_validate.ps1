param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('Capture','BuildProbe','Probe','Adapters','Supplementary','Final')][string]$Action,
    [ValidateSet('ordinary','diagnostic')][string]$Profile='ordinary',
    [switch]$ReuseProbeDependencies,
    [string]$GameExe='E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe'
)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../..'))
$out=Join-Path $repo 'build-artifacts/native-runtime/N06-2026-10-10'
New-Item -ItemType Directory -Force -Path $out | Out-Null
Set-Location -LiteralPath $repo

function ProtectedManifest {
    $files=@('src','external','tests','release-assets') | ForEach-Object {
        if (Test-Path -LiteralPath $_) { Get-ChildItem -LiteralPath $_ -File -Recurse }
    }
    $files+=Get-ChildItem -LiteralPath $repo -File | Where-Object { $_.Extension -in '.asi','.zip','.ini','.cmd' }
    $files | Sort-Object FullName -Unique | ForEach-Object {
        [pscustomobject]@{Path=$_.FullName.Substring($repo.Length+1);SHA256=(Get-FileHash -LiteralPath $_.FullName).Hash}
    }
}
function RunRecorded([string]$Name,[string]$Exe,[string[]]$Arguments) {
    $suffix=if(Test-Path (Join-Path $out ($Name+'.stdout.log'))){'-'+[DateTime]::UtcNow.ToString('HHmmss-fff')}else{''}
    $stdout=Join-Path $out ($Name+$suffix+'.stdout.log'); $stderr=Join-Path $out ($Name+$suffix+'.stderr.log')
    $start=[DateTime]::UtcNow
    # The existing harness bounds each owned child to 10 seconds. Direct native
    # invocation gives an explicit exit code, including under Windows PowerShell 5.
    $previous=$ErrorActionPreference; $ErrorActionPreference='Continue'
    & $Exe @Arguments 1> $stdout 2> $stderr
    $code=$LASTEXITCODE; $ErrorActionPreference=$previous
    [pscustomobject]@{Name=$Name;Exe=$Exe;Arguments=$Arguments;ExitCode=$code;UTC=$start.ToString('o');Stdout=$stdout;Stderr=$stderr} |
        ConvertTo-Json -Compress | Set-Content -LiteralPath (Join-Path $out ($Name+$suffix+'.command.json'))
    Write-Output "$Name exit=$code stdout=$stdout"
    if ($code -ne 0) { Get-Content -LiteralPath $stderr; throw "$Name failed" }
}
switch($Action) {
    'Capture' {
        if (-not (Test-Path (Join-Path $out 'protected-before.csv'))) {
            ProtectedManifest | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $out 'protected-before.csv')
        }
        & git rev-parse HEAD | Set-Content -LiteralPath (Join-Path $out 'git-head.txt')
        & git branch --show-current | Set-Content -LiteralPath (Join-Path $out 'git-branch.txt')
        & git status --short | Set-Content -LiteralPath (Join-Path $out 'git-status-before.txt')
        # Preserve staged changes separately. The full current file manifest is
        # authoritative for working bytes; a commit alone does not identify them.
        & git diff --cached HEAD -- src external tests build.cmd test.cmd | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $out 'staged-source-diff-from-HEAD.patch')
        $identities=@($GameExe,(Join-Path (Split-Path $GameExe) 'STALKER2CameraTweaks.asi'))
        $identities+=@('final-production.asi','final-diagnostic.asi','ordinary/runtime_native_adapter_harness.exe','diagnostic/runtime_native_adapter_harness.exe') |
            ForEach-Object { Join-Path $repo ('build-artifacts/native-runtime-evidence/2026-10-09/'+$_) }
        $identities+=@('runtime_evidence_replay_harness.exe','gameplay_camera_resolver_harness.exe','cinematic_aspect_harness.exe','instruction_validator_harness.exe') |
            ForEach-Object { Join-Path $repo ('build-artifacts/tests/'+$_) }
        $identities | ForEach-Object {
            if (Test-Path -LiteralPath $_) {
                $item=Get-Item -LiteralPath $_
                [pscustomobject]@{Path=$item.FullName;SHA256=(Get-FileHash -LiteralPath $_).Hash;Length=$item.Length;LastWriteUTC=$item.LastWriteTimeUtc.ToString('o');FileVersion=$item.VersionInfo.FileVersion;ProductVersion=$item.VersionInfo.ProductVersion}
            } else { [pscustomobject]@{Path=$_;SHA256='UNAVAILABLE';Length=0;LastWriteUTC='';FileVersion='';ProductVersion=''} }
        } | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $out 'artifact-identities.csv')
        Get-FileHash -LiteralPath (Join-Path $out 'protected-before.csv'),(Join-Path $out 'staged-source-diff-from-HEAD.patch') | Format-List
    }
    'BuildProbe' {
        $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        $devcmd=& $vswhere -latest -products '*' -version '[17.14,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'Common7/Tools/VsDevCmd.bat'
        if (-not $devcmd) { throw 'Required toolchain missing' }
        $productionRegistryPath=Join-Path $repo 'tools/build/production_sources.json'
        if(-not(Test-Path -LiteralPath $productionRegistryPath)){throw 'Production source registry is missing'}
        $productionRegistry=Get-Content -LiteralPath $productionRegistryPath -Raw | ConvertFrom-Json
        if($productionRegistry.schema_version -ne 1){throw 'Unsupported production source registry schema'}
        $runtimeOwners=@($productionRegistry.sources | Where-Object {$_.role -eq 'runtime_owner'})
        $dllEntries=@($productionRegistry.sources | Where-Object {$_.role -eq 'dll_entry'})
        $sources=@($productionRegistry.sources | Where-Object {$_.role -notin 'runtime_owner','dll_entry'} | ForEach-Object {$_.path})
        if($runtimeOwners.Count -ne 1 -or $runtimeOwners[0].path -ne 'src/plugin/runtime.cpp' -or
            $dllEntries.Count -ne 1 -or $dllEntries[0].path -ne 'src/plugin/dll_entry.cpp' -or
            $sources.Count -ne 59){throw 'Production source registry violates the native-adapter source contract'}
        $rsp=Join-Path $out 'probe-sources.rsp'
        $sources | Set-Content -Encoding ascii -LiteralPath $rsp
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/generate_ini_documentation.ps1
        if($LASTEXITCODE -ne 0){throw 'Generated documentation dependency failed'}
        $obj='build-artifacts\native-runtime\N06-2026-10-10\probe-obj'
        New-Item -ItemType Directory -Force -Path $obj | Out-Null
        $compileInputs='@build-artifacts\native-runtime\N06-2026-10-10\probe-sources.rsp'
        if($ReuseProbeDependencies){
            $objects=@($sources | ForEach-Object { Join-Path $obj ([IO.Path]::GetFileNameWithoutExtension($_)+'.obj') })
            foreach($object in $objects){if(-not(Test-Path -LiteralPath $object)){throw "Missing N06 dependency object: $object"}}
            $objects | Set-Content -Encoding ascii -LiteralPath (Join-Path $out 'probe-dependency-objects.rsp')
            $compileInputs='@build-artifacts\native-runtime\N06-2026-10-10\probe-dependency-objects.rsp'
        }
        $exe='build-artifacts\native-runtime\N06-2026-10-10\n06_game_image_probe.exe'
        $res='build-artifacts\native-runtime\N06-2026-10-10\probe-resources.res'
        $command='call "'+$devcmd+'" -arch=x64 -host_arch=x64 && cl /Bv && rc /nologo /fo '+$res+' src\overlay\localization\localization_resources.rc && cl /nologo /std:c++latest /O1 /MT /EHsc /W4 /utf-8 /DNDEBUG /DOVERLAY_PRODUCTION /DOVERLAY_SETTINGS_FRONTEND /DOVERLAY_COMBINED /DOVERLAY_STARTUP_JOURNAL /DSAFETYHOOK_TEST_VIRTUAL_PROTECT /Ibuild-artifacts\generated /Iexternal\safetyhook /Iexternal\spdlog\include /Iexternal\imgui /Iexternal\imgui\backends /Fo'+$obj+'\ research\evidence\native-runtime\N06\tools\n06_game_image_probe.cpp tests\plugin\runtime_owner_binding_probe.cpp @build-artifacts\native-runtime\N06-2026-10-10\probe-sources.rsp /link user32.lib psapi.lib bcrypt.lib dxgi.lib d3d11.lib dcomp.lib d3dcompiler.lib '+$res+' /OUT:'+ $exe
        # cl /Bv exits unsuccessfully without an input. Record compiler identity separately.
        $command=$command.Replace('cl /Bv && ','')
        $command=$command.Replace('@build-artifacts\native-runtime\N06-2026-10-10\probe-sources.rsp',$compileInputs).Replace('cl /nologo /std:','cl /Bv /nologo /std:')
        $logName=if(Test-Path (Join-Path $out 'probe-build.log')){'probe-build-'+[DateTime]::UtcNow.ToString('HHmmss-fff')+'.log'}else{'probe-build.log'}
        $log=Join-Path $out $logName
        $command | Set-Content -LiteralPath (Join-Path $out 'probe-build-command.txt')
        $previous=$ErrorActionPreference; $ErrorActionPreference='Continue'
        & cmd.exe /d /c $command *> $log
        $code=$LASTEXITCODE; $ErrorActionPreference=$previous
        [pscustomobject]@{Name='probe-build';Command=$command;ExitCode=$code;UTC=[DateTime]::UtcNow.ToString('o');Log=$log} | ConvertTo-Json -Compress | Add-Content -LiteralPath (Join-Path $out 'commands.jsonl')
        Write-Output "probe-build exit=$code log=$log"
        if($code -ne 0){Get-Content -LiteralPath $log -Tail 35;throw 'Probe compile failed'}
        Get-FileHash -LiteralPath $exe | Format-List
    }
    'Probe' { RunRecorded 'game-image-probe' (Join-Path $out 'n06_game_image_probe.exe') @($GameExe) }
    'Adapters' {
        $env:TEMP=Join-Path $out ('temp-'+$Profile); $env:TMP=$env:TEMP
        New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
        $exe=Join-Path $repo ('build-artifacts/native-runtime-evidence/2026-10-09/'+$Profile+'/runtime_native_adapter_harness.exe')
        RunRecorded ($Profile+'-actual-adapters') $exe @()
    }
    'Supplementary' {
        $env:TEMP=Join-Path $out 'temp-supplementary'; $env:TMP=$env:TEMP
        New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
        foreach($name in 'runtime_evidence_replay','gameplay_camera_resolver','cinematic_aspect','instruction_validator'){
            RunRecorded $name (Join-Path $repo ('build-artifacts/tests/'+$name+'_harness.exe')) @()
        }
    }
    'Final' {
        $after=@(ProtectedManifest)
        $after | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $out 'protected-after.csv')
        $diff=Compare-Object (Import-Csv (Join-Path $out 'protected-before.csv')) $after -Property Path,SHA256
        if($diff){$diff | Format-Table;throw 'Protected files changed during evidence audit'}
        'PASS: production/vendor/tests/configuration/release assets unchanged' | Set-Content -LiteralPath (Join-Path $out 'preservation-result.txt')
        & git status --short | Set-Content -LiteralPath (Join-Path $out 'git-status-after.txt')
        foreach($identity in Import-Csv (Join-Path $out 'artifact-identities.csv')){
            if($identity.SHA256 -ne 'UNAVAILABLE' -and (Get-FileHash -LiteralPath $identity.Path).Hash -ne $identity.SHA256){throw "Original artifact changed: $($identity.Path)"}
        }
        $support=Join-Path $out 'supporting'
        New-Item -ItemType Directory -Force -Path $support | Out-Null
        foreach($name in 'STALKER2CameraTweaks.log','STALKER2CameraTweaksStartup.log','STALKER2CameraTweaksOverlay.log'){
            $log=Join-Path (Split-Path $GameExe) $name
            if(Test-Path -LiteralPath $log){Copy-Item -LiteralPath $log -Destination (Join-Path $support ('installed-'+$name))}
        }
        Get-Content -LiteralPath 'E:\Steam\steamapps\appmanifest_1643320.acf' |
            Select-String '"appid"|"name"|"buildid"|"TargetBuildID"|"LastUpdated"' |
            ForEach-Object {$_.Line.Trim()} | Set-Content -LiteralPath (Join-Path $support 'steam-build-identity.txt')
        @('n06_game_image_probe.cpp','n06_validate.ps1') | ForEach-Object {Join-Path $PSScriptRoot $_} |
            ForEach-Object {[pscustomobject]@{Path=$_.Substring($repo.Length+1);SHA256=(Get-FileHash -LiteralPath $_).Hash}} |
            Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $out 'audit-tool-hashes.csv')
        Get-ChildItem -LiteralPath $out -File | Where-Object {$_.Name -ne 'output-hashes.csv'} | ForEach-Object {
            [pscustomobject]@{Name=$_.Name;SHA256=(Get-FileHash -LiteralPath $_.FullName).Hash;Length=$_.Length}
        } | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $out 'output-hashes.csv')
        Get-ChildItem -LiteralPath $out -File | Where-Object {$_.Extension -in '.csv','.log','.json','.jsonl','.txt','.patch','.rsp'} |
            ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $support}
        Get-ChildItem -LiteralPath $support -File | Where-Object {$_.Name -ne 'supporting-hashes.csv'} |
            ForEach-Object {[pscustomobject]@{Name=$_.Name;SHA256=(Get-FileHash -LiteralPath $_.FullName).Hash;Length=$_.Length}} |
            Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $support 'supporting-hashes.csv')
        Write-Output "Preservation PASS: $($after.Count) protected files unchanged."
    }
}
