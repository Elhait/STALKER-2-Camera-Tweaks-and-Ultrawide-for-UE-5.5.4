param([int] $GameProcessId = 0, [switch] $Control)
$ErrorActionPreference = 'Stop'
if ($GameProcessId -le 0) {
    $games = @(Get-Process -Name 'Stalker2-Win64-Shipping' -ErrorAction SilentlyContinue)
    if ($games.Count -ne 1) {
        throw 'Expected one game process. Supply -GameProcessId with the current Shipping process ID.'
    }
    $GameProcessId = $games[0].Id
}
$game = Get-Process -Id $GameProcessId -ErrorAction Stop
if ($game.ProcessName -ne 'Stalker2-Win64-Shipping') { throw 'The specified PID is not the game Shipping process.' }
$kind = if ($Control) { 'U2Control' } else { 'FovCapture' }
$name = "Local\STALKER2CameraTweaks.$kind.$GameProcessId"
$signal = $null
try {
    $signal = [Threading.EventWaitHandle]::OpenExisting($name)
    if (-not $signal.Set()) { throw 'Could not signal the existing capture event.' }
    if ($Control) {
        Write-Output 'Control requested: next Dialogue callback begins the next of two observation-only control phases (20 seconds / 128 records). Confirm U2_CONTROL_BEGIN and coordinator/exclusion state.'
    } else {
        Write-Output "Capture requested for PID $GameProcessId. The next accepted Cinematic ENTER starts the one-shot 60-second window."
        Write-Output 'A repeated command cannot extend or restart a spent capture; confirm FOV_CAPTURE_CONTROL and the ARM marker in the game log.'
    }
} finally {
    if ($null -ne $signal) { $signal.Dispose() }
}
