[CmdletBinding()]
param(
    [ValidateSet('Preflight', 'Capture', 'Recover')][string]$Mode = 'Preflight',
    [ValidateRange(15, 180)][int]$Seconds = 120,
    [string]$GameDirectory = 'E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64',
    [switch]$Elevate
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$root = Join-Path $repo 'build-artifacts\overlay-idle-etw'
$principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    if (-not $Elevate) { throw 'Administrator rights required. Run with -Elevate for the Windows UAC prompt.' }
    $arguments = '-NoProfile -ExecutionPolicy Bypass -File "{0}" -Mode {1} -Seconds {2} -GameDirectory "{3}"' -f $PSCommandPath, $Mode, $Seconds, $GameDirectory
    $child = Start-Process powershell.exe -Verb RunAs -WindowStyle Hidden -ArgumentList $arguments -PassThru
    $deadline = [Diagnostics.Stopwatch]::StartNew()
    while ($deadline.Elapsed.TotalSeconds -lt 45) {
        $childFolder = Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue | Where-Object Name -Like "*-$Mode-$($child.Id)" | Select-Object -First 1
        if ($childFolder -and (Test-Path -LiteralPath (Join-Path $childFolder.FullName 'failure.txt'))) {
            $reason = Get-Content -LiteralPath (Join-Path $childFolder.FullName 'failure.txt') -TotalCount 1
            throw "$reason See $($childFolder.FullName)"
        }
        if ($Mode -eq 'Recover' -and $childFolder -and (Test-Path -LiteralPath (Join-Path $childFolder.FullName 'result.json'))) {
            Write-Output "Recovery complete. Results: $($childFolder.FullName)"
            return
        }
        if ($childFolder -and (Test-Path -LiteralPath (Join-Path $childFolder.FullName 'anchors.jsonl'))) {
            Write-Output "Recording started. Results: $($childFolder.FullName)"
            if ($Mode -eq 'Capture') { Write-Output "Start the game now. Recording stops automatically after $Seconds seconds; do not open the mod menu during the toast-to-idle reproduction." }
            return
        }
        if ($child.HasExited) { throw "Elevated process exited before recording. Inspect $root" }
        Start-Sleep -Milliseconds 250
    }
    throw "No recording-start confirmation within 45 seconds. Inspect $root before launching the game."
}

$wpr = (Get-Command wpr.exe -ErrorAction Stop).Source
$xperf = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Windows Performance Toolkit\xperf.exe'
if (-not (Test-Path -LiteralPath $xperf)) { throw 'Windows Performance Toolkit xperf.exe is required.' }
New-Item -ItemType Directory -Path $root -Force | Out-Null
$folder = Join-Path $root ((Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $Mode + '-' + $PID)
New-Item -ItemType Directory -Path $folder | Out-Null
$instance = 'STALKER2OverlayIdle-' + $PID
$started = $false
$saved = $false
$markerFailed = $false
$transcript = Join-Path $folder 'controller.log'
Start-Transcript -Path $transcript | Out-Null

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program exited with $LASTEXITCODE; arguments: $($Arguments -join ' ')" }
}
function Mark([string]$Name) {
    $anchor = [ordered]@{ name = $Name; utc = [DateTime]::UtcNow.ToString('o'); qpc = [Diagnostics.Stopwatch]::GetTimestamp(); qpcFrequency = [Diagnostics.Stopwatch]::Frequency }
    $anchor | ConvertTo-Json -Compress | Add-Content -LiteralPath (Join-Path $folder 'anchors.jsonl') -Encoding UTF8
    & $wpr -marker $Name -instancename $instance
    if ($LASTEXITCODE -ne 0) { $script:markerFailed = $true; Write-Warning 'ETW marker failed; sidecar anchor preserved.' }
}

try {
    if ($Mode -eq 'Capture') {
        $gate = Join-Path $PSScriptRoot 'overlay_idle_etw_preflight.json'
        if (-not (Test-Path -LiteralPath $gate)) { throw 'No reviewed preflight evidence. Complete and review Preflight first.' }
        $approval = Get-Content -LiteralPath $gate -Raw | ConvertFrom-Json
        if ($approval.status -ne 'PASS' -or $approval.profiles -ne 'CPU+GPU+DesktopComposition') { throw 'Preflight approval does not cover this recording profile.' }
        if ($approval.computerName -ne $env:COMPUTERNAME -or $approval.osBuild -ne [string][Environment]::OSVersion.Version.Build) { throw 'Preflight receipt is for a different machine/OS build.' }
        if (Get-Process -Name 'Stalker2-Win64-Shipping' -ErrorAction SilentlyContinue) { throw 'Start recording BEFORE starting the game.' }
    }
    # Recover only this controller's orphaned sessions; never stop foreign recordings.
    $sessions = & logman.exe query -ets 2>&1
    if ($LASTEXITCODE -ne 0) { throw 'Cannot enumerate active ETW sessions.' }
    $orphanIds = @($sessions | ForEach-Object {
        if ($_ -match 'WPR_initiated_STALKER2OverlayIdle-(\d+)_WPR (System|Event) Collector') { $Matches[1] }
    } | Select-Object -Unique)
    foreach ($controllerId in $orphanIds) {
        if (Get-Process -Id ([int]$controllerId) -ErrorAction SilentlyContinue) { throw "Another capture controller is active (PID $controllerId); no new recording started." }
        $orphanInstance = 'STALKER2OverlayIdle-' + $controllerId
        Write-Output "Saving orphaned recording: $orphanInstance"
        & $wpr -stop (Join-Path $folder ('recovered-' + $controllerId + '.etl')) 'Recovered abandoned Overlay ETW recording' -skipPdbGen -instancename $orphanInstance
        $recoveryExit = $LASTEXITCODE
        if ($recoveryExit -eq -2147024893) {
            # ERROR_PATH_NOT_FOUND after ignored recording outputs disappeared.
            # The controller is dead and its named session cannot be saved.
            Write-Warning "Missing recording path for $orphanInstance; cancelling only this orphan. Its trace cannot be recovered."
            Invoke-Checked $wpr @('-cancel', '-instancename', $orphanInstance)
            "Unrecoverable recording cancelled: $orphanInstance (stop error 0x80070003)" | Add-Content -LiteralPath (Join-Path $folder 'recovery.txt')
        } elseif ($recoveryExit -ne 0) {
            throw "Orphan recovery failed with $recoveryExit; session left untouched: $orphanInstance"
        }
    }
    if ($Mode -eq 'Recover') {
        [ordered]@{ status = 'RECOVERED'; instances = $orphanIds; utc = [DateTime]::UtcNow.ToString('o') } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $folder 'result.json')
        Write-Output 'Orphan recovery complete; no new recording started.'
        return
    }
    # A named instance isolates our commands from unrelated recordings.
    $status = & $wpr -status 2>&1
    $status | Set-Content -LiteralPath (Join-Path $folder 'existing-wpr-status.txt')
    if ($LASTEXITCODE -ne 0 -or ($status -join "`n") -notmatch 'WPR is not recording') { throw 'Existing/unknown WPR recording state; leave it untouched.' }
    foreach ($provider in 'Microsoft-Windows-DXGI', 'Microsoft-Windows-DxgKrnl', 'Microsoft-Windows-Dwm-Core', 'Microsoft-Windows-DirectComposition') {
        & logman.exe query providers $provider | Set-Content -LiteralPath (Join-Path $folder ($provider + '.txt'))
        if ($LASTEXITCODE -ne 0) { throw "Provider unavailable: $provider" }
    }
    & $wpr -profiledetails CPU+GPU+DesktopComposition -filemode | Set-Content -LiteralPath (Join-Path $folder 'profile-details.txt')
    if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect installed profiles.' }
    $disk = Get-PSDrive -Name ([IO.Path]::GetPathRoot($folder).Substring(0,1))
    $requiredSpace = if ($Mode -eq 'Capture') { 20GB } else { 10GB }
    if ($disk.Free -lt $requiredSpace) { throw "Insufficient free disk space: require $requiredSpace bytes for recording and merge." }
    if ($Mode -eq 'Capture') {
        foreach ($name in 'STALKER2CameraTweaks.asi', 'Stalker2-Win64-Shipping.exe', 'STALKER2CameraTweaks.ini') {
            $inputPath = Join-Path $GameDirectory $name
            if (-not (Test-Path -LiteralPath $inputPath)) { throw "Missing identity input: $inputPath" }
            Get-FileHash -LiteralPath $inputPath -Algorithm SHA256 | Select-Object Path, Hash | ConvertTo-Json -Compress | Add-Content -LiteralPath (Join-Path $folder 'identity.jsonl')
        }
        Copy-Item -LiteralPath (Join-Path $GameDirectory 'STALKER2CameraTweaks.ini') -Destination $folder
    }
    Invoke-Checked $wpr @('-start', 'CPU', '-start', 'GPU', '-start', 'DesktopComposition', '-filemode', '-recordtempto', $folder, '-instancename', $instance)
    $started = $true
    Mark ('STALKER2_' + $Mode + '_START')
    $clock = [Diagnostics.Stopwatch]::StartNew()
    $duration = if ($Mode -eq 'Preflight') { 8 } else { $Seconds }
    $seenGame = $false
    while ($clock.Elapsed.TotalSeconds -lt $duration) {
        if ($Mode -eq 'Preflight') {
            # Real CPU samples plus blocked/runnable transitions, without touching the game.
            $busy = [Diagnostics.Stopwatch]::StartNew()
            while ($busy.ElapsedMilliseconds -lt 50) { [Math]::Sqrt(1234567) | Out-Null }
            Start-Sleep -Milliseconds 50
        } else {
            if (-not $seenGame) {
                $game = Get-Process -Name 'Stalker2-Win64-Shipping' -ErrorAction SilentlyContinue
                if ($game) {
                    $game | Select-Object Id, StartTime, Path | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $folder 'game-process.json')
                    Mark 'STALKER2_GAME_PID_DETECTED'
                    $seenGame = $true
                }
            }
            Start-Sleep -Milliseconds 500
        }
    }
    Mark ('STALKER2_' + $Mode + '_STOP_REQUEST')
    $etl = Join-Path $folder 'capture.etl'
    Invoke-Checked $wpr @('-stop', $etl, 'Overlay toast to idle scheduling and graphics investigation', '-skipPdbGen', '-instancename', $instance)
    $started = $false
    $saved = $true
    if ($Mode -eq 'Preflight') {
        Invoke-Checked $xperf @('-i', $etl, '-o', (Join-Path $folder 'trace-stats.txt'), '-a', 'tracestats')
        # A short excerpt proves provider delivery without expanding the entire ETL to multi-GiB CSV.
        Invoke-Checked $xperf @('-i', $etl, '-o', (Join-Path $folder 'events.csv'), '-a', 'dumper', '-range', '0', '2000000')
        Invoke-Checked $xperf @('-i', $etl, '-o', (Join-Path $folder 'markers.txt'), '-a', 'marks')
        foreach ($eventName in 'CSwitch', 'ReadyThread', 'Profile') {
            Invoke-Checked $xperf @('-i', $etl, '-o', (Join-Path $folder ($eventName + '-stacks.html')), '-a', 'stack', '-butterfly', '1', '-pid', "$PID", '-event', ('.*' + $eventName + '.*'))
        }
        Write-Output 'Recorded, NOT automatically PASS. Review real stack records, graphics events, markers and event loss before approving Capture.'
    } else {
        foreach ($name in 'STALKER2CameraTweaks.log', 'STALKER2CameraTweaksOverlay.log', 'STALKER2CameraTweaksStartup.log') {
            $logPath = Join-Path $GameDirectory $name
            if (Test-Path -LiteralPath $logPath) { Copy-Item -LiteralPath $logPath -Destination $folder }
        }
        if (-not $seenGame) { throw 'ETL saved, but no game PID was observed. This is NOT a game capture.' }
    }
    [ordered]@{ status = 'RECORDED'; mode = $Mode; profiles = 'CPU+GPU+DesktopComposition'; controllerPid = $PID; markerFailure = $markerFailed; etl = $etl; utc = [DateTime]::UtcNow.ToString('o') } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $folder 'result.json')
} catch {
    $_ | Out-String | Set-Content -LiteralPath (Join-Path $folder 'failure.txt')
    Write-Error $_ -ErrorAction Continue
    exit 1
} finally {
    if ($started) {
        # Preserve evidence from OUR instance only, including partial/failed runs.
        & $wpr -stop (Join-Path $folder 'partial.etl') 'Interrupted overlay ETW recording' -skipPdbGen -instancename $instance
        if ($LASTEXITCODE -ne 0) { Write-Warning "Our session may still be active: $instance. Do not cancel unrelated sessions." }
    }
    Stop-Transcript | Out-Null
}
Write-Output "Results: $folder; saved=$saved"
