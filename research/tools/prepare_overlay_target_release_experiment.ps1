param([string]$OutputDirectory = 'build-artifacts/tr', [switch]$Build, [switch]$Timing)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output = [IO.Path]::GetFullPath((Join-Path $repo $OutputDirectory))
$allowed = [IO.Path]::GetFullPath((Join-Path $repo 'build-artifacts')) + [IO.Path]::DirectorySeparatorChar
if (!$output.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must be under build-artifacts.' }
if (Test-Path -LiteralPath $output) { throw 'Choose an unused output directory.' }
$snapshot = Join-Path $output 'source'
[IO.Directory]::CreateDirectory($snapshot) | Out-Null
$encoding = [Text.UTF8Encoding]::new($false)
$inventory = @(& git -C $repo ls-files -- src external assets locales tools tests docs CMakeLists.txt build.cmd test.cmd README.md .gitattributes .gitignore)
if ($LASTEXITCODE -ne 0) { throw 'Source inventory unavailable.' }
$manifest = foreach ($relative in $inventory) {
    $source = Join-Path $repo $relative
    $destination = Join-Path $snapshot $relative
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination
    [pscustomobject]@{ path = $relative; sha256 = (Get-FileHash -LiteralPath $source).Hash }
}
$path = Join-Path $snapshot 'src/overlay/presenter/renderer_runtime.cpp'
$text = [IO.File]::ReadAllText($path).Replace("`r`n", "`n")
$start = '    bool Renderer::ReconcileCompositionAttachment() noexcept'
$end = '    bool Renderer::StartPendingNotificationLifetimes() noexcept'
if ([regex]::Matches($text, [regex]::Escape($start)).Count -ne 1 -or
    [regex]::Matches($text, [regex]::Escape($end)).Count -ne 1) { throw 'Reconciliation anchors changed.' }
$begin = $text.IndexOf($start)
$finish = $text.IndexOf($end, $begin)
$replacement = [IO.File]::ReadAllText((Join-Path $repo 'research/prototypes/overlay_attachment/target_release_reconcile.inl')).Replace("`r`n", "`n")
$text = $text.Substring(0, $begin) + $replacement + "`n`n" + $text.Substring($finish)
$old = @'
        HRESULT result = compositionDevice_->CreateTargetForHwnd(window, TRUE,
            compositionTarget_.GetAddressOf());
        if (FAILED(result)) {
            Log("OVERLAY_DCOMP_TOPMOST_TARGET_UNAVAILABLE; using non-topmost target");
            result = compositionDevice_->CreateTargetForHwnd(window, FALSE,
                compositionTarget_.GetAddressOf());
        }
        return SUCCEEDED(result) && SUCCEEDED(compositionDevice_->CreateVisual(
            visual_.GetAddressOf()));
'@
$new = @'
        HRESULT result = compositionDevice_->CreateTargetForHwnd(window, TRUE,
            compositionTarget_.GetAddressOf());
        Log("OVERLAY_TARGET_EXPERIMENT create layer=TRUE hr=" +
            std::to_string(static_cast<long>(result)));
        if (FAILED(result)) {
            Log("OVERLAY_DCOMP_TOPMOST_TARGET_UNAVAILABLE; using non-topmost target");
            result = compositionDevice_->CreateTargetForHwnd(window, FALSE,
                compositionTarget_.GetAddressOf());
            Log("OVERLAY_TARGET_EXPERIMENT create layer=FALSE hr=" +
                std::to_string(static_cast<long>(result)));
        }
        if (FAILED(result)) return false;
        return visual_ || SUCCEEDED(compositionDevice_->CreateVisual(
            visual_.GetAddressOf()));
'@
if ([regex]::Matches($text, [regex]::Escape($old)).Count -ne 1) { throw 'Target creation anchor changed.' }
[IO.File]::WriteAllText($path, $text.Replace($old, $new), $encoding)
if ($Timing) { & (Join-Path $PSScriptRoot 'add_overlay_target_timing_snapshot.ps1') -Snapshot $snapshot }
$head = (& git -C $repo rev-parse HEAD).Trim()
$patchedPaths = @('src/overlay/presenter/renderer_runtime.cpp')
if ($Timing) { $patchedPaths += @('src/overlay/composition/composition_runtime.cpp','src/overlay/presenter/target_timing.hpp') }
$patchedInputs = foreach ($relative in $patchedPaths) {
    [pscustomobject]@{ path=$relative; sha256=(Get-FileHash -LiteralPath (Join-Path $snapshot $relative)).Hash }
}
[IO.File]::WriteAllText((Join-Path $output 'source_manifest.json'), (@{
    revision = $head; files = @($manifest); patched_path = 'src/overlay/presenter/renderer_runtime.cpp';
    patched_sha256 = (Get-FileHash -LiteralPath $path).Hash; timing_enabled = [bool]$Timing; patched_inputs = @($patchedInputs)
} | ConvertTo-Json -Depth 5), $encoding)
if ($Build) {
    $names = @('CAMERA_TWEAKS_CMAKE_BUILD_ROOT','CAMERA_TWEAKS_BUILD_PROFILE','CAMERA_TWEAKS_STARTUP_TIMELINE')
    $saved = @{}
    foreach ($name in $names) { $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
    try {
        $env:CAMERA_TWEAKS_CMAKE_BUILD_ROOT = Join-Path $output 'compile-E'
        $env:CAMERA_TWEAKS_BUILD_PROFILE = 'production'
        $env:CAMERA_TWEAKS_STARTUP_TIMELINE = '0'
        & (Join-Path $snapshot 'tools/build/build-cmake-candidate.cmd') 2>&1 | Tee-Object -FilePath (Join-Path $output 'build.log')
        if ($LASTEXITCODE -ne 0) { throw "Isolated build failed: $LASTEXITCODE" }
        Copy-Item -LiteralPath (Join-Path $env:CAMERA_TWEAKS_CMAKE_BUILD_ROOT 'production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi') -Destination $output
        (Get-FileHash -LiteralPath (Join-Path $output 'STALKER2CameraTweaks.asi')).Hash | Write-Output
    } finally {
        foreach ($name in $names) { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    }
}
foreach ($entry in $manifest) {
    if ((Get-FileHash -LiteralPath (Join-Path $repo $entry.path)).Hash -ne $entry.sha256) { throw "Canonical input changed: $($entry.path)" }
}
Write-Output "Prepared target-release experiment: $output"
