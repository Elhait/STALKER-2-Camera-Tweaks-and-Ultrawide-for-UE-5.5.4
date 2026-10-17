param(
    [Parameter(Mandatory)][string]$CsvPath,
    [Parameter(Mandatory)][string]$MetricColumn,
    [string]$OverlayLogPath
)
$ErrorActionPreference = 'Stop'
$rows = @(Import-Csv -LiteralPath $CsvPath)
if ($rows.Count -eq 0 -or $MetricColumn -notin $rows[0].PSObject.Properties.Name) { throw 'Metric column missing or CSV empty.' }
$groupColumns = @('Application','ProcessID','SwapChainAddress','FrameType') | Where-Object { $_ -in $rows[0].PSObject.Properties.Name }
if ('SwapChainAddress' -notin $groupColumns) { throw 'Capture must identify swapchains; do not pool unrelated streams.' }
$summaries = foreach ($group in ($rows | Group-Object -Property $groupColumns)) {
    $values = @()
    foreach ($row in $group.Group) {
        $v = 0.0
        if ([double]::TryParse($row.$MetricColumn, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$v) -and
            ![double]::IsNaN($v) -and ![double]::IsInfinity($v) -and $v -gt 0) { $values += $v }
    }
    $values = @($values | Sort-Object)
    if ($values.Count -eq 0) { continue }
    $median = $values[[math]::Max(0, [int][math]::Ceiling($values.Count * 0.5) - 1)]
    [pscustomobject]@{
        stream = $group.Name; metric = $MetricColumn; valid_samples = $values.Count; excluded_samples = $group.Count - $values.Count
        mean_ms = ($values | Measure-Object -Average).Average; median_ms = $median
        p95_ms = $values[[math]::Ceiling($values.Count * 0.95) - 1]; p99_ms = $values[[math]::Ceiling($values.Count * 0.99) - 1]
        max_ms = $values[-1]; above_2x_median = @($values | Where-Object { $_ -gt 2 * $median }).Count
        above_50ms = @($values | Where-Object { $_ -gt 50 }).Count; above_100ms = @($values | Where-Object { $_ -gt 100 }).Count
    }
}
$idleIntervals = @()
if ($OverlayLogPath) {
    $start = $null
    foreach ($line in (Get-Content -LiteralPath $OverlayLogPath)) {
        if ($line -notmatch 'OVERLAY_ATTACHMENT_PROBE ') { continue }
        $fields = @{}
        foreach ($match in [regex]::Matches($line, '(\w+)=([^\s]+)')) { $fields[$match.Groups[1].Value] = $match.Groups[2].Value }
        if ($fields.reason -eq 'idle_begin') { $start = $fields }
        elseif ($start -and $fields.reason -eq 'toggle_before' -and $fields.visible -eq '0') {
            $idleIntervals += [pscustomobject]@{
                variant = $fields.variant; elapsed_ms = [long]$fields.tick_ms - [long]$start.tick_ms
                render_calls = [long]$fields.render_calls - [long]$start.render_calls
                draw_calls = [long]$fields.draw_calls - [long]$start.draw_calls
                commit_attempts = [long]$fields.commit_attempts - [long]$start.commit_attempts
                root_commit_accepted_at_start = $start.root_commit_accepted; root_commit_accepted_at_end = $fields.root_commit_accepted
            }
            $start = $null
        }
    }
}
@{ metric_summaries = @($summaries); observed_idle_intervals = $idleIntervals; percentile_method = 'nearest rank'; caveat = 'Metric meaning and FG frame visibility depend on capture version/provider; missing displayed/generated frames are not inferred.' } | ConvertTo-Json -Depth 6
