param(
    [string]$OutputDirectory = 'build-artifacts/fp',
    [string]$BaseIniPath = 'build-artifacts/generated/STALKER2CameraTweaks.preview.ini',
    [switch]$Build
)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if ([IO.File]::ReadAllText((Join-Path $repo 'src/overlay/presenter/renderer_runtime.cpp')).Contains('bool Renderer::ReconcileCompositionAttachment()')) {
    throw 'Historical A/B/C preparation requires the pre-adoption baseline e756442. Current production already detaches; use its normal build for validation.'
}
$outputInput = if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $repo $OutputDirectory }
$output = [IO.Path]::GetFullPath($outputInput)
$allowed = [IO.Path]::GetFullPath((Join-Path $repo 'build-artifacts')) + [IO.Path]::DirectorySeparatorChar
if (!$output.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Experiment output must be under this repository build-artifacts directory.'
}
if (Test-Path -LiteralPath $output) { throw "Choose an unused experiment directory: $output" }
$iniInput = if ([IO.Path]::IsPathRooted($BaseIniPath)) { $BaseIniPath } else { Join-Path $repo $BaseIniPath }
$iniPath = [IO.Path]::GetFullPath($iniInput)
if (!(Test-Path -LiteralPath $iniPath)) { throw "Missing base INI: $iniPath" }
$ini = [IO.File]::ReadAllText($iniPath)
$overlayPattern = '(?ms)(^\[Overlay\]\r?\n(?:(?!^\[).)*?^Enabled\s*=\s*)(true|false)([^\r\n]*)'
if ([regex]::Matches($ini, $overlayPattern).Count -ne 1) { throw 'Expected exactly one Overlay.Enabled in base INI.' }
$snapshot = Join-Path $output 'source'
[IO.Directory]::CreateDirectory($snapshot) | Out-Null
$inventory = @(& git -C $repo ls-files -- src external assets locales tools tests docs CMakeLists.txt build.cmd test.cmd README.md .gitattributes .gitignore)
if ($LASTEXITCODE -ne 0) { throw 'Cannot read tracked source inventory.' }
$manifest = @()
foreach ($relative in $inventory) {
    $source = Join-Path $repo $relative
    $destination = Join-Path $snapshot $relative
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination
    $manifest += [pscustomobject]@{ path = $relative; sha256 = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash }
}
$encoding = [Text.UTF8Encoding]::new($false)
function Replace-Checked([string]$relative, [string]$old, [string]$new, [int]$count = 1) {
    $path = Join-Path $snapshot $relative
    $text = [IO.File]::ReadAllText($path).Replace("`r`n", "`n")
    $matches = [regex]::Matches($text, [regex]::Escape($old)).Count
    if ($matches -ne $count) { throw "$relative`: anchor expected $count, found $matches" }
    [IO.File]::WriteAllText($path, $text.Replace($old, $new), $encoding)
}
$presenter = 'src/overlay/presenter'
Copy-Item -LiteralPath (Join-Path $repo 'research/prototypes/overlay_attachment/attachment_probe_members.inl') -Destination (Join-Path $snapshot $presenter)
Copy-Item -LiteralPath (Join-Path $repo 'research/prototypes/overlay_attachment/attachment_probe_methods.inl') -Destination (Join-Path $snapshot $presenter)
Replace-Checked "$presenter/renderer_runtime.hpp" '        void Shutdown() noexcept;' "        void Shutdown() noexcept;`n        void LogAttachmentExperimentSnapshot(const char* reason) const noexcept;"
Replace-Checked "$presenter/renderer_runtime.hpp" '        CompositionPresenterState state_;' "#include `"attachment_probe_members.inl`"`n        CompositionPresenterState state_;"
Replace-Checked "$presenter/renderer_runtime.cpp" 'compositionTarget_->SetRoot(visual_.Get())' 'ProbeSetRoot(visual_.Get())' 3
Replace-Checked "$presenter/renderer_runtime.cpp" 'compositionTarget_->SetRoot(nullptr)' 'ProbeSetRoot(nullptr)'
Replace-Checked "$presenter/renderer_runtime.cpp" 'compositionDevice_->Commit()' 'ProbeCommit()' 5
Replace-Checked "$presenter/renderer_runtime.cpp" "        bool updated = false;" "        LogAttachmentExperimentSnapshot(`"geometry_begin`");`n        bool updated = false;"
Replace-Checked "$presenter/renderer_runtime.cpp" "        submittedNotificationCount_ = 0;`n        submittedDrawLists_.clear();" "        ++probeRenderCalls_;`n        submittedNotificationCount_ = 0;`n        submittedDrawLists_.clear();"
Replace-Checked "$presenter/renderer_runtime.cpp" "        if (!targetSurface || !device_ || !compositionDevice_ || !width || !height)" "        ++probeDrawCalls_;`n        if (!targetSurface || !device_ || !compositionDevice_ || !width || !height)"
Replace-Checked "$presenter/renderer_runtime.cpp" "    void Renderer::StartPendingNotificationLifetimes() noexcept`n    {" "    void Renderer::StartPendingNotificationLifetimes() noexcept`n    {`n        if (!ReconcileProbeAttachment()) return;"
Replace-Checked "$presenter/renderer_runtime.cpp" "        surfaceHeight_ = 0;`n        compositionReady_ = false;" "        surfaceHeight_ = 0;`n        compositionReady_ = false;`n        probeRootKnown_ = false;`n        probePendingRoot_ = false;"
$rendererPath = Join-Path $snapshot "$presenter/renderer_runtime.cpp"
[IO.File]::AppendAllText($rendererPath, "`n#include `"attachment_probe_methods.inl`"`n", $encoding)
Replace-Checked 'src/overlay/composition/composition_runtime.cpp' '            RendererInstance().SetVisible(input.visible());' "            RendererInstance().LogAttachmentExperimentSnapshot(`"toggle_before`");`n            RendererInstance().SetVisible(input.visible());"
Replace-Checked 'src/overlay/composition/composition_runtime.cpp' '        if (previous != active) SetEvent(g_frameClockControl);' @'
        if (previous != active) {
            Log(std::string("OVERLAY_ATTACHMENT_CLOCK_DEMAND active=") + (active ? "1" : "0"));
            SetEvent(g_frameClockControl);
        }
'@
$cmakePath = Join-Path $snapshot 'CMakeLists.txt'
[IO.File]::AppendAllText($cmakePath, @'

# Research-only source snapshot; never used by the canonical repository build.
if(NOT "$ENV{OVERLAY_ATTACHMENT_EXPERIMENT_DETACH}" MATCHES "^[01]$")
    message(FATAL_ERROR "Select attachment experiment B=0 or C=1")
endif()
add_compile_definitions(OVERLAY_ATTACHMENT_EXPERIMENT_DETACH=$ENV{OVERLAY_ATTACHMENT_EXPERIMENT_DETACH})
'@, $encoding)
$head = (& git -C $repo rev-parse HEAD).Trim()
[IO.File]::WriteAllText((Join-Path $output 'source_manifest.json'), (@{ revision = $head; files = $manifest; base_ini_sha256 = (Get-FileHash -LiteralPath $iniPath).Hash } | ConvertTo-Json -Depth 5), $encoding)
$changedPaths = @('CMakeLists.txt', "$presenter/renderer_runtime.hpp", "$presenter/renderer_runtime.cpp",
    'src/overlay/composition/composition_runtime.cpp', "$presenter/attachment_probe_members.inl", "$presenter/attachment_probe_methods.inl")
$experimentInputs = foreach ($relative in $changedPaths) {
    [pscustomobject]@{ path = $relative; sha256 = (Get-FileHash -LiteralPath (Join-Path $snapshot $relative)).Hash }
}
[IO.File]::WriteAllText((Join-Path $output 'experiment_inputs.json'), ($experimentInputs | ConvertTo-Json), $encoding)
foreach ($variant in @('A','B','C')) {
    $bundle = Join-Path $output $variant
    [IO.Directory]::CreateDirectory($bundle) | Out-Null
    $enabled = if ($variant -eq 'A') { 'false' } else { 'true' }
    $variantIni = [regex]::Replace($ini, $overlayPattern, { param($m) $m.Groups[1].Value + $enabled + $m.Groups[3].Value })
    [IO.File]::WriteAllText((Join-Path $bundle 'STALKER2CameraTweaks.ini'), $variantIni, $encoding)
}
if ($Build) {
    $envNames = @('OVERLAY_ATTACHMENT_EXPERIMENT_DETACH','CAMERA_TWEAKS_CMAKE_BUILD_ROOT','CAMERA_TWEAKS_BUILD_PROFILE','CAMERA_TWEAKS_STARTUP_TIMELINE')
    $saved = @{}
    foreach ($name in $envNames) { $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
    try {
        $env:CAMERA_TWEAKS_BUILD_PROFILE = 'production'
        $env:CAMERA_TWEAKS_STARTUP_TIMELINE = '0'
        foreach ($variant in @('B','C')) {
            $env:OVERLAY_ATTACHMENT_EXPERIMENT_DETACH = if ($variant -eq 'C') { '1' } else { '0' }
            $env:CAMERA_TWEAKS_CMAKE_BUILD_ROOT = Join-Path $output "compile-$variant"
            & (Join-Path $snapshot 'tools/build/build-cmake-candidate.cmd') 2>&1 | Tee-Object -FilePath (Join-Path $output "build-$variant.log")
            $code = $LASTEXITCODE
            if ($code -ne 0) { throw "Variant $variant build failed: exit $code" }
            $built = Join-Path $env:CAMERA_TWEAKS_CMAKE_BUILD_ROOT 'production-tOFF-dOFF/bin/STALKER2CameraTweaks.asi'
            Copy-Item -LiteralPath $built -Destination (Join-Path $output "$variant/STALKER2CameraTweaks.asi")
        }
        Copy-Item -LiteralPath (Join-Path $output 'B/STALKER2CameraTweaks.asi') -Destination (Join-Path $output 'A/STALKER2CameraTweaks.asi')
    } finally {
        foreach ($name in $envNames) { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    }
    $artifacts = foreach ($variant in @('A','B','C')) {
        [pscustomobject]@{ variant = $variant; asi_sha256 = (Get-FileHash -LiteralPath (Join-Path $output "$variant/STALKER2CameraTweaks.asi")).Hash; ini_sha256 = (Get-FileHash -LiteralPath (Join-Path $output "$variant/STALKER2CameraTweaks.ini")).Hash }
    }
    [IO.File]::WriteAllText((Join-Path $output 'artifacts.json'), ($artifacts | ConvertTo-Json), $encoding)
}
foreach ($entry in $manifest) {
    if ((Get-FileHash -LiteralPath (Join-Path $repo $entry.path)).Hash -ne $entry.sha256) { throw "Canonical input changed during experiment: $($entry.path)" }
}
Write-Output "Prepared isolated attachment experiment: $output"
