$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$source = Join-Path $repo 'src/plugin/runtime.cpp'
$expected = 'D4E124A5A1871CE80F736BAF4AD221E2F4AB24931CA0B2DC9BD761E74F00B341'
if ((Get-FileHash $source -Algorithm SHA256).Hash -ne $expected) { throw 'Source changed: review audited inventory before generating ledger.' }
$lines = Get-Content $source
$audit = Get-Content (Join-Path $repo 'research/reports/RUNTIME_CPP_DECOMPOSITION_AUDIT_2026-10-08.md') -Raw
$functionIndex = ($audit -split '## Appendix A\. Function/template index')[1] -split '### Local data shapes' | Select-Object -First 1
$fieldIndex = ($audit -split '## Appendix B\. RuntimeState member index')[1]
$guards = @(); $stack = [System.Collections.Generic.List[string]]::new()
for ($guardLine=0; $guardLine -lt $lines.Count; ++$guardLine) {
    $line = $lines[$guardLine]
    if ($line -match '^\s*#(if|ifdef|ifndef)\s+(.*)') {
        $directive = $line.Trim(); $continuation = $guardLine
        while ($directive.EndsWith('\')) {
            ++$continuation
            $directive = $directive.TrimEnd('\').TrimEnd() + ' ' + $lines[$continuation].Trim()
        }
        $stack.Add($directive)
    }
    elseif ($line -match '^\s*#(else|elif)') { $stack[$stack.Count-1] += ' / ' + $line.Trim() }
    $guards += $(if ($stack.Count) { $stack -join '; ' } else { 'Always' })
    if ($line -match '^\s*#endif') { $stack.RemoveAt($stack.Count-1) }
}
function Destination([string]$name, [int]$line) {
    if ($name -match '^(SelectGameplayMode|RuntimeHotkeyBinding|ApplyGameplayModeSetting|ApplyRuntimeSetting|ReadRuntimeSettings|PersistHotkeyActionValue|PersistRuntimeSetting)$') { return @('runtime_settings.cpp','B2','Same owner; settings under correction gate; persistence outside gate') }
    if ($name -match '^(MarkCameraStateFreshness|MarkCameraPresentationTransition|MarkCameraStateForSetting|PublishCameraStateSnapshot|PublishCameraFovTransaction|ReadOverlaySemanticSnapshot)$') { return @('runtime_camera_publication.cpp','B3','Same owner; preserve correction gate then cameraStateMutex and wake order') }
    if ($name -match '^(ReplayManualTransition|GetCameraWriterEngineOperations|CameraWriterRecoveryStateView|GetCameraWriterCinematicStateView|GetCameraWriterGameplayStateView|ClearCinematicRecoveryAuxiliaryState|CompleteCinematicRecovery|ResetAtomicExitHandoff|ArmAtomicExitHandoff|ApplyPendingGameplayModeTransition|ApplyGameplayAspectFixAtomic)$') { return @('runtime_camera_callbacks.cpp','B6','Same owner/gate; preserve whole writer/recovery dispatch') }
    if ($name -match '^(ApplyCinematicAspectStore|TraceCinematicEnter|TraceCinematicExit|GetCinematicLifecycleStateView|ResolveAutoAspect|ReadAutoAspectForCinematicStoreLog|ResolveAutoAspectForCinematicStore|ResolveCinematicAspect)$') { return @('runtime_cinematic_callbacks.cpp','B5','Same owner/gate; native register/store/selection order') }
    if ($name -match '^(TraceDialogueBoundary|ResetDialogueRuntimeState|GetDialogueBoundaryStateView|ResetPostCinematicDialogueExclusion|ArmPostCinematicDialogueExclusion|ObservePostCinematicDialogueRecovery)$') { return @('runtime_dialogue_callbacks.cpp','B4','Same owner; gate before dialogueMutex; explicit cross-calls') }
    if ($name -match '^(TraceZoomTransition|TraceZoomIn|TraceZoomOut|ZoomTransitionFloatChanged)$') { return @('Remain: runtime.cpp','B7 optional','Zoom affinity/TLS unchanged; no extraction until justified') }
    if ($name -match '^(ValidateEnterBoundary|ValidateExitBoundary|ValidateIndexedExitBoundary|ResolveCinematicAspectStore|ResolveCinematicFovCallsites|InstallCinematicAspect|RestoreCinematicAspect|InstallCinematicAspectComponent|InstallCinematicFovComponent|CommitCinematicComponents|RollbackCinematicAspectComponent|RollbackCinematicFovComponent|InstallDialogueBoundary|InstallZoomTransitionObservation|VerifyExecutableAndInstruction)$') { return @('runtime_hook_installation.cpp','B7 unapproved','Same HookSet owner; disabled/commit/rollback order') }
    if ($name -match '^(IsValidAspect|IsWritable|IsValidFovValue|SafeRead|WriteAspectAndFlags|WriteAspectOnly|SafeMidHookEntry)$') { return @('Remain: private runtime utility; shared private header only if required','B1/B6 reviewed','No new owner; identical native checks/exception boundary') }
    if ($name -match '^(Log|LogImmediate|OutputCameraLogProbeDebug|FlushCameraLogProbe|DeferCameraLogProbeFlush|GetCinematicContinuationTrace|GetDialogueSourceProvenanceTrace|ReadDialogueWriterSnapshot|DialogueSourceStageName|EmitDialogueSourceEvent|RecordDialogueSourceCallback|RecordDialogueRecoverySource|RecordDialogueSourceEnd|TraceGameplayAspectRestorationState|CinematicDiagnosticActionName|LogCompletedCinematicContinuationTrace|FlushCinematicContinuationTrace|InitialEstablishmentEndReasonName|DiagnosticFovChanged|LogCinematicDiagnostic.*|GetCinematicBaselineWriterPublicationState|ReadCinematicBaselineWriterPublication|CaptureCinematicBaselineWriterPublication|CinematicBaselineOriginName|LogCinematicBaseline.*|LogForeignRecoverySource|LogGameplayFovSourceChange|LogGameplayFovChange|LogEnterWriterObservation|BeginEnterWriterCapture|CompleteEnterWriterCapture|RecordEnterWriterObservation|LogCameraModeChange|LogDialogueDiagnostic.*|ArmDialogueRecoveryEndpointTrace|ObserveDialogueRecoveryEndpointTrace|ReadHorPlusCinematicWriterState|HorPlusCinematicWriterSnapshotChanged|LogHorPlusCinematicWriterTrace|TraceHorPlusCinematicWriterBefore|TraceHorPlusCinematicWriterAfter|FovTraceValueChanged|TraceGameplayFovSourceSample|TraceHorPlusRecoverySample|HorPlusTelemetryOwnerName|ResolveHorPlusTelemetryOwner|TraceHorPlusFovState)$') { return @('runtime_diagnostics.cpp (coherent domain subsets if necessary)','B3 separate reviewed sub-batch','Original owner/TLS/lazy storage; existing guards; deferred emission after gate') }
    return @('Remain: runtime.cpp','No move in B1-B6','Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval')
}
$report = [System.Collections.Generic.List[string]]::new()
$report.Add('# Runtime decomposition — exact move ledger (Batch 0)')
$report.Add('')
$report.Add("Source: src/plugin/runtime.cpp; SHA-256 $expected. Generated from the audited indices and hash-checked current source. Destinations are proposals, not authorization for B1–B7. Line numbers are baseline-specific.")
$report.Add('')
$report.Add('All RuntimeState fields retain one resident g_runtime owner. Relocating its definition privately in B1 does not relocate ownership or introduce storage. Guards are source conditions, including dormant unsupported profiles; these profiles are not enabled for V0.')
$report.Add('')
$report.Add('## Functions/templates (one entry per audited definition)')
$report.Add('')
$report.Add('| Symbol | Line | Destination | Batch | State aliases referenced in body | Guard / contract |')
$report.Add('|---|---:|---|---|---|---|')
$functions = [regex]::Matches($functionIndex, '`(\w+)`:(\d+)')
foreach ($match in $functions) {
    $name = $match.Groups[1].Value; $number = [int]$match.Groups[2].Value
    if ($lines[$number-1] -notmatch "\b$name\b") { throw "Function inventory mismatch: $name at $number" }
    $destination = Destination $name $number
    $start = $number - 1
    while ($lines[$start] -notmatch '\{') { ++$start }
    $end = $start; $depth = 0
    do {
        $depth += ([regex]::Matches($lines[$end], '\{')).Count - ([regex]::Matches($lines[$end], '\}')).Count
        ++$end
    } while ($depth -gt 0 -and $end -lt $lines.Count)
    $aliases = @([regex]::Matches(($lines[$start..($end-1)] -join "`n"), '\bg_\w+\b') | ForEach-Object Value | Sort-Object -Unique) -join ', '
    $report.Add("| $name | $number | $($destination[0]) | $($destination[1]) | $aliases | $($guards[$number-1] -replace '\|','&#124;'); $($destination[2]) |")
}
$report.Add('')
$report.Add('## RuntimeState members (one entry per field)')
$report.Add('')
$report.Add('| Member | Line | Declaration/initializer first line | Owner / destination / batch | Source guard |')
$report.Add('|---|---:|---|---|---|')
$fields = [regex]::Matches($fieldIndex, '`(\w+)`:(\d+)')
if ($fields.Count -ne 159) { throw "Expected 159 fields, found $($fields.Count)" }
foreach ($match in $fields) {
    $name = $match.Groups[1].Value; $number = [int]$match.Groups[2].Value
    if ($lines[$number-1] -notmatch "\b$name\b") { throw "Field inventory mismatch: $name" }
    $report.Add("| $name | $number | $($lines[$number-1].Trim() -replace '\|','&#124;') | g_runtime only; resident runtime.cpp / private definition B1 | $($guards[$number-1] -replace '\|','&#124;') |")
}
$report.Add('')
$report.Add('## Aliases, data shapes, constants and static/TLS storage')
$report.Add('')
$report.Add('Alias bindings remain references to the original owner or HookSet submember, not independent state. Their eventual private visibility follows each consumer; initial B1 keeps a single binding site. Non-owner storage stays at its current lifetime and scope until its owning integration component moves. Local/TLS storage must never become a header-static copy. This table preserves exact declarations and conditions rather than inferring ownership from names.')
$report.Add('')
$report.Add('| Line | Declaration | Source guard | Initial disposition |')
$report.Add('|---:|---|---|---|')
for ($i=0; $i -lt $lines.Count; ++$i) {
    if ($lines[$i] -match '^\s*(auto& g_|RuntimeState& g_|(?:static )?thread_local |static (?!void|bool|float|const char\*)(?:.*)|constexpr |struct \w+|enum class HorPlusTelemetryOwner|plugin::RuntimeSettingsApi g_|std::mutex g_|std::deque<plugin::OverlayNotification>|std::uint64_t g_nextNotificationId|std::atomic.*\bg_\w+|(?:bool|float|SafetyHookMid) g_)') {
        $declaration = $lines[$i].Trim()
        if ($declaration.StartsWith('auto& g_')) {
            $last = $i
            while (-not $declaration.EndsWith(';')) { ++$last; $declaration += ' ' + $lines[$last].Trim() }
        }
        $report.Add("| $($i+1) | $($declaration -replace '\|','&#124;') | $($guards[$i] -replace '\|','&#124;') | Remain at original lifetime; bind same owner on approved consumer move |")
    }
}
$report.Add('')
$report.Add("Inventory: $($functions.Count) function/template definitions, $($fields.Count) RuntimeState members. Full declarations and initializers, transitive calls and hook/member layout are preserved in the recoverable baseline source; alias lists above are direct references, not an exhaustive call graph.")
$target = Join-Path $repo 'research/evidence/runtime-decomposition/MOVE_LEDGER_2026-10-08.md'
New-Item -ItemType Directory -Force (Split-Path -Parent $target) | Out-Null
$report | Set-Content -Encoding utf8 -LiteralPath $target
Write-Output "Ledger generated: $($functions.Count) functions, $($fields.Count) fields."
