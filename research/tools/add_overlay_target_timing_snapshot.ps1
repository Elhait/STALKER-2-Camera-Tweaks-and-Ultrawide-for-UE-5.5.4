param([Parameter(Mandatory=$true)][string]$Snapshot)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$root=[IO.Path]::GetFullPath($Snapshot)
$allowed=[IO.Path]::GetFullPath((Join-Path $repo 'build-artifacts'))+[IO.Path]::DirectorySeparatorChar
if(!$root.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Snapshot must be under build-artifacts.'}
$encoding=[Text.UTF8Encoding]::new($false)
function Patch([string]$path,[string]$old,[string]$new,[int]$count=1){
    $file=Join-Path $root $path
    $text=[IO.File]::ReadAllText($file).Replace("`r`n","`n")
    if([regex]::Matches($text,[regex]::Escape($old)).Count -ne $count){throw "Timing anchor changed: $path / $old"}
    [IO.File]::WriteAllText($file,$text.Replace($old,$new),$encoding)
}
$r='src/overlay/presenter/renderer_runtime.cpp'
$c='src/overlay/composition/composition_runtime.cpp'
Copy-Item -LiteralPath (Join-Path $repo 'research/prototypes/overlay_attachment/target_timing.hpp') -Destination (Join-Path $root 'src/overlay/presenter/target_timing.hpp')
Patch $r '#include "renderer_runtime.hpp"' "#include `"renderer_runtime.hpp`"`n#include `"target_timing.hpp`""
Patch $c '#include "composition_runtime.hpp"' "#include `"composition_runtime.hpp`"`n#include `"../presenter/target_timing.hpp`""
Patch $r 'compositionDevice_->Commit()' 'target_timing_experiment::Invoke(target_timing_experiment::Operation::Commit, [&]() { return compositionDevice_->Commit(); })' 6
Patch $r 'return drew && SUCCEEDED(target_timing_experiment::Invoke(target_timing_experiment::Operation::Commit,' 'return drew && SUCCEEDED(target_timing_experiment::Invoke(target_timing_experiment::Operation::SurfaceCommit,'
Patch $r '[this]() { return SUCCEEDED(target_timing_experiment::Invoke(target_timing_experiment::Operation::Commit,' '[this]() { return SUCCEEDED(target_timing_experiment::Invoke(target_timing_experiment::Operation::AttachmentCommit,'
Patch $r 'compositionTarget_->SetRoot(visual_.Get())' 'target_timing_experiment::Invoke(target_timing_experiment::Operation::SetRoot, [&]() { return compositionTarget_->SetRoot(visual_.Get()); })' 3
Patch $r 'compositionTarget_->SetRoot(nullptr)' 'target_timing_experiment::Invoke(target_timing_experiment::Operation::SetRoot, [&]() { return compositionTarget_->SetRoot(nullptr); })'
Patch $r "compositionTarget_->SetRoot(`n                                attach ? visual_.Get() : nullptr)" 'target_timing_experiment::Invoke(target_timing_experiment::Operation::SetRoot, [&]() { return compositionTarget_->SetRoot(attach ? visual_.Get() : nullptr); })'
Patch $r 'compositionTarget_.Reset();' 'target_timing_experiment::Invoke(target_timing_experiment::Operation::ReleaseTarget, [&]() { compositionTarget_.Reset(); });' 2
$file=Join-Path $root $c
$text=[IO.File]::ReadAllText($file)
$begin=$text.IndexOf('    LRESULT CALLBACK OverlayWindowProc(')
$end=$text.IndexOf('    bool InstallInputHook(', $begin)
if($begin -lt 0 -or $end -lt 0){throw 'Window procedure anchors missing.'}
$part=$text.Substring($begin,$end-$begin)
$old='std::scoped_lock lock{g_rendererMutex};'
if([regex]::Matches($part,[regex]::Escape($old)).Count -ne 2){throw 'Expected exactly two window-procedure renderer locks.'}
$index=$part.IndexOf($old)
$part=$part.Substring(0,$index)+'overlay::target_timing_experiment::WindowLock lock{g_rendererMutex, overlay::target_timing_experiment::Operation::WindowMutexBefore};'+$part.Substring($index+$old.Length)
$part=$part.Replace($old,'overlay::target_timing_experiment::WindowLock lock{g_rendererMutex, overlay::target_timing_experiment::Operation::WindowMutexAfter};')
[IO.File]::WriteAllText($file,$text.Substring(0,$begin)+$part+$text.Substring($end),$encoding)
Patch $c '        g_uiThreadId = GetCurrentThreadId();' "        g_uiThreadId = GetCurrentThreadId();`n        overlay::target_timing_experiment::Initialize();"
Patch $c '            TranslateMessage(&message);' "            overlay::target_timing_experiment::Drain(&Log);`n            TranslateMessage(&message);"
