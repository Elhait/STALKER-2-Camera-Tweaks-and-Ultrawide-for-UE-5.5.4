param(
    [Parameter(Mandatory = $true)]
    [string] $RunnerPath,
    [string] $RegistryPath = 'tests/test_harnesses.json'
)

$ErrorActionPreference = 'Stop'
$runnerFullPath = (Resolve-Path -LiteralPath $RunnerPath).Path
$projectRoot = Split-Path -Parent $runnerFullPath
Set-Location -LiteralPath $projectRoot
$registryFullPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $RegistryPath))
$registry = Get-Content -LiteralPath $registryFullPath -Raw | ConvertFrom-Json
if ($registry.schema_version -ne 1) { throw 'Unsupported test harness registry schema.' }

function Resolve-RegistryPath([string] $Path) {
    return [IO.Path]::GetFullPath((Join-Path $projectRoot $Path))
}

$harnesses = @($registry.harnesses)
$runnerEntries = @($registry.runner)
$harnessNames = @($harnesses | ForEach-Object { $_.name })
$duplicateHarnessNames = @($harnessNames | Group-Object | Where-Object Count -ne 1)
$outputNames = @($harnesses | ForEach-Object { [IO.Path]::GetFileNameWithoutExtension($_.name) })
$duplicateOutputs = @($outputNames | Group-Object | Where-Object Count -ne 1)
$repositoryHarnesses = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'tests') -Recurse -File -Filter '*_harness.cpp' |
    ForEach-Object { $_.FullName.Substring($projectRoot.Length + 1).Replace('\', '/') } | Sort-Object -Unique)
$registryEntries = @($harnesses | ForEach-Object { $_.entry_source } | Sort-Object -Unique)
$missingHarnesses = @($repositoryHarnesses | Where-Object { $_ -notin $registryEntries })
$unknownHarnesses = @($registryEntries | Where-Object { $_ -notin $repositoryHarnesses })
$missingInputs = [System.Collections.Generic.List[string]]::new()

foreach ($harness in $harnesses) {
    if ($harness.name -ne [IO.Path]::GetFileNameWithoutExtension($harness.name)) {
        $missingInputs.Add("invalid_target_name:$($harness.name)")
    }
    $targetSources = @($harness.entry_source) + @($harness.additional_sources)
    if (@($targetSources | Sort-Object -Unique).Count -ne $targetSources.Count) {
        $missingInputs.Add("duplicate_source:$($harness.name)")
    }
    foreach ($source in $targetSources) {
        if (-not (Test-Path -LiteralPath (Resolve-RegistryPath $source) -PathType Leaf)) {
            $missingInputs.Add("missing_source:$($harness.name):$source")
        }
    }
    foreach ($include in $harness.include_directories) {
        if (-not (Test-Path -LiteralPath (Resolve-RegistryPath $include) -PathType Container)) {
            $missingInputs.Add("missing_include_directory:$($harness.name):$include")
        }
    }
    foreach ($library in $harness.link_libraries) {
        if ($library -notmatch '^[A-Za-z0-9_.-]+\.lib$') {
            $missingInputs.Add("invalid_link_library:$($harness.name):$library")
        }
    }
    if ($harness.compile_definitions -contains 'NDEBUG' -and
        $harness.name -notin @('runtime_native_adapter_harness', 'n06_m01_zoom_game_derived_harness')) {
        $missingInputs.Add("unexpected_NDEBUG:$($harness.name)")
    }
}

$requiredFlags = @('/nologo', '/std:c++latest', '/O1', '/MT', '/EHsc', '/W4', '/utf-8')
$missingFlags = @($requiredFlags | Where-Object { $_ -notin $registry.compiler_contract.common_flags })
if ($registry.compiler_contract.common_flags -contains '/DNDEBUG' -or
    $registry.compiler_contract.common_flags -contains 'NDEBUG') {
    $missingInputs.Add('global_NDEBUG_for_test_targets')
}
if ($registry.compiler_contract.runtime_library -ne 'MultiThreaded') {
    $missingInputs.Add('test_runtime_library_contract_mismatch')
}

$supportTargets = @($registry.support_targets)
if ($supportTargets.Count -ne 1 -or $supportTargets[0].name -ne 'test_cmd_exit_status_probe' -or
    -not (Test-Path -LiteralPath (Resolve-RegistryPath $supportTargets[0].source) -PathType Leaf) -or
    $supportTargets[0].output -ne ($registry.compiler_contract.output_directory + '/' + $supportTargets[0].name + '.exe')) {
    $missingInputs.Add('missing_exit_status_probe_support_target')
}

$productionRegistryPath = Join-Path $projectRoot 'tools/build/production_sources.json'
$productionRegistry = Get-Content -LiteralPath $productionRegistryPath -Raw | ConvertFrom-Json
if ($productionRegistry.schema_version -ne 1) { throw 'Unsupported production source registry schema.' }
$productionAdapterInputs = @($productionRegistry.sources |
    Where-Object { $_.role -notin @('runtime_owner', 'dll_entry') } |
    ForEach-Object { $_.path.Replace('\', '/') })
foreach ($name in @('runtime_native_adapter_harness', 'n06_m01_zoom_game_derived_harness')) {
    $harness = $harnesses | Where-Object name -eq $name
    $declaredAdapterInputs = @($harness.additional_sources | Where-Object { $_ -ne 'tests/plugin/runtime_owner_binding_probe.cpp' } | Sort-Object -Unique)
    if ($declaredAdapterInputs.Count -ne 59 -or
        (@($productionAdapterInputs | Where-Object { $_ -notin $declaredAdapterInputs }).Count -ne 0) -or
        (@($declaredAdapterInputs | Where-Object { $_ -notin $productionAdapterInputs }).Count -ne 0)) {
        $missingInputs.Add("production_adapter_source_parity:$name")
    }
    if ($harness.additional_sources -contains 'src/plugin/runtime.cpp' -or
        $harness.additional_sources -contains 'src/plugin/dll_entry.cpp') {
        $missingInputs.Add("runtime_owner_must_not_be_separately_compiled:$name")
    }
}
$adapterSource = Get-Content -LiteralPath (Join-Path $projectRoot 'tests/plugin/runtime_native_adapter_harness.cpp') -Raw
$zoomSource = Get-Content -LiteralPath (Join-Path $projectRoot 'tests/plugin/n06_m01_zoom_game_derived_harness.cpp') -Raw
if ($adapterSource -notmatch '#include\s+"\.\./\.\./src/plugin/runtime\.cpp"' -or
    $zoomSource -notmatch '#include\s+"runtime_native_adapter_harness\.cpp"') {
    $missingInputs.Add('runtime_owner_include_once_contract')
}

$resourceUsers = @($harnesses | Where-Object embedded_localization_resource | ForEach-Object name | Sort-Object)
$expectedResourceUsers = @('localization_harness', 'n06_m01_zoom_game_derived_harness', 'runtime_native_adapter_harness' | Sort-Object)
if (@(Compare-Object $resourceUsers $expectedResourceUsers).Count -ne 0) {
    $missingInputs.Add('embedded_resource_target_parity')
}
if (-not (Test-Path -LiteralPath (Resolve-RegistryPath $registry.compiler_contract.resource_source) -PathType Leaf) -or
    -not (Test-Path -LiteralPath (Resolve-RegistryPath $registry.compiler_contract.resource_id_header) -PathType Leaf)) {
    $missingInputs.Add('resource_source_or_id_header_missing')
}

$runErrors = [System.Collections.Generic.List[string]]::new()
foreach ($entry in $runnerEntries) {
    if ($entry.harness -notin $harnessNames) { $runErrors.Add("unbuilt_target:$($entry.harness)") }
    foreach ($argument in $entry.arguments) {
        if ($argument -match '\s') { $runErrors.Add("argument_must_be_separate_token:$($entry.harness):$argument") }
    }
}
if ($harnesses.Count -ne 59 -or $harnessNames.Count -ne 59 -or $duplicateHarnessNames.Count -ne 0 -or
    $duplicateOutputs.Count -ne 0 -or $runnerEntries.Count -ne 62 -or
    $missingHarnesses.Count -ne 0 -or $unknownHarnesses.Count -ne 0 -or $runErrors.Count -ne 0) {
    $missingInputs.Add('canonical_harness_or_runner_inventory_mismatch')
}
$adapterRuns = @($runnerEntries | Where-Object harness -eq 'runtime_native_adapter_harness')
$zoomRuns = @($runnerEntries | Where-Object harness -eq 'n06_m01_zoom_game_derived_harness')
$expectedZoomArguments = @('zoom-in off', 'zoom-in on', 'zoom-out off', 'zoom-out on')
$actualZoomArguments = @($zoomRuns | ForEach-Object { $_.arguments -join ' ' })
if ($adapterRuns.Count -ne 1 -or $zoomRuns.Count -ne 4 -or
    (@(Compare-Object $actualZoomArguments $expectedZoomArguments).Count -ne 0)) {
    $missingInputs.Add('native_adapter_or_N06_run_contract_mismatch')
}
$scenarioNames = @($registry.native_adapter_contract.scenario_names)
if ($scenarioNames.Count -ne 112 -or @($scenarioNames | Sort-Object -Unique).Count -ne 112 -or
    @($registry.native_adapter_contract.diagnostic_modes).Count -ne 2 -or
    $registry.native_adapter_contract.child_process_count -ne 224) {
    $missingInputs.Add('native_adapter_scenario_mode_inventory_mismatch')
}

$testCmdText = Get-Content -LiteralPath $runnerFullPath -Raw
$legacyCompileLines = @($testCmdText -split "`r?`n" | Where-Object { $_ -match '^cl .*?/OUT:build-artifacts\\tests\\' })
if ($legacyCompileLines.Count -gt 0) {
    $legacyCommon = [regex]::Match($testCmdText, '(?m)^set "COMMON=([^"]+)"\r?$').Groups[1].Value -split '\s+'
    if (@($requiredFlags | Where-Object { $_ -notin $legacyCommon }).Count -ne 0) {
        $missingInputs.Add('legacy_common_compiler_flags_mismatch')
    }
    $legacyCompileNames = @($legacyCompileLines | ForEach-Object {
        [regex]::Match($_, '/OUT:build-artifacts\\tests\\([^\\]+)\.exe').Groups[1].Value
    })
    if ($legacyCompileNames.Count -ne 59 -or @(Compare-Object ($legacyCompileNames | Sort-Object) ($harnessNames | Sort-Object)).Count -ne 0) {
        $missingInputs.Add('legacy_compile_contract_parity')
    }
    foreach ($compileLine in $legacyCompileLines) {
        $expandedCompileLine = $compileLine.Replace('%COMMON%', ($legacyCommon -join ' '))
        $outputMatch = [regex]::Match($expandedCompileLine, '/OUT:build-artifacts\\tests\\([^\\]+)\.exe')
        $targetName = $outputMatch.Groups[1].Value
        $registeredTarget = $harnesses | Where-Object name -eq $targetName
        $compileText = ($expandedCompileLine -split '\s+/link\s+')[0]
        $legacyEntry = [regex]::Match($compileText, 'tests\\[^ ]+_harness\.cpp').Value.Replace('\', '/')
        $legacySources = @([regex]::Matches($compileText, '(?:tests|src|external)\\[^ ]+\.(?:cpp|c)') |
            ForEach-Object { $_.Value.Replace('\', '/') })
        if ($compileText -match '@build-artifacts\\tests\\runtime_adapter_sources\.rsp') {
            $legacySources += $productionAdapterInputs
        }
        $registeredSources = @($registeredTarget.entry_source) + @($registeredTarget.additional_sources)
        if ($legacyEntry -ne $registeredTarget.entry_source -or
            ($legacySources -join "`n") -cne ($registeredSources -join "`n")) {
            $missingInputs.Add("legacy_source_inputs_mismatch:$targetName")
        }
        $legacyDefinitions = @([regex]::Matches($compileText, '(?:^|\s)/D([^\s]+)') | ForEach-Object { $_.Groups[1].Value })
        if (($legacyDefinitions -join "`n") -cne (@($registeredTarget.compile_definitions) -join "`n")) {
            $missingInputs.Add("legacy_compile_definitions_mismatch:$targetName")
        }
        $legacyIncludes = @([regex]::Matches($compileText, '(?:^|\s)/I([^\s]+)') | ForEach-Object { $_.Groups[1].Value.Replace('\', '/') } | Select-Object -Unique)
        if (($legacyIncludes -join "`n") -cne (@($registeredTarget.include_directories) -join "`n")) {
            $missingInputs.Add("legacy_include_directories_mismatch:$targetName")
        }
        $legacyLinkText = [regex]::Match($compileLine, '/link\s+(.*?)\s+/OUT:', [Text.RegularExpressions.RegexOptions]::IgnoreCase).Groups[1].Value
        $legacyLibraries = @([regex]::Matches($legacyLinkText, '(?<!\S)([A-Za-z0-9_.-]+\.lib)') | ForEach-Object { $_.Groups[1].Value })
        if (($legacyLibraries -join "`n") -cne (@($registeredTarget.link_libraries) -join "`n") -or
            (($legacyLinkText -match 'localization_resources\.res') -ne [bool]$registeredTarget.embedded_localization_resource)) {
            $missingInputs.Add("legacy_link_contract_mismatch:$targetName")
        }
    }
    $legacyRunLines = @($testCmdText -split "`r?`n" | Where-Object { $_ -match '^build-artifacts\\tests\\[^ ]+\.exe(?:\s+.*)?$' })
    $legacyRuns = @($legacyRunLines | ForEach-Object {
        $match = [regex]::Match($_, '^build-artifacts\\tests\\([^ ]+)\.exe(?:\s+(.*))?$')
        [pscustomobject]@{ harness = [IO.Path]::GetFileNameWithoutExtension($match.Groups[1].Value); arguments = @($match.Groups[2].Value -split '\s+' | Where-Object { $_ }) }
    })
    if ($legacyRuns.Count -ne $runnerEntries.Count) { $missingInputs.Add('legacy_runner_multiplicity_mismatch') }
    else {
        for ($index = 0; $index -lt $runnerEntries.Count; ++$index) {
            if ($legacyRuns[$index].harness -ne $runnerEntries[$index].harness -or
                (@(Compare-Object $legacyRuns[$index].arguments $runnerEntries[$index].arguments).Count -ne 0)) {
                $missingInputs.Add("legacy_runner_order_or_arguments_mismatch:$index")
                break
            }
        }
    }
} else {
    if ($testCmdText -notmatch 'test_cmd_orchestrator\.ps1') {
        $missingInputs.Add('canonical_entry_point_not_using_hybrid_targets_and_runner')
    }
    $orchestratorScript = Get-Content -LiteralPath (Join-Path $projectRoot 'tests/runner/test_cmd_orchestrator.ps1') -Raw
    $runnerScript = Get-Content -LiteralPath (Join-Path $projectRoot 'tests/runner/test_harness_runner.ps1') -Raw
    $supervisorScript = Get-Content -LiteralPath (Join-Path $projectRoot 'tests/runner/process_tree_supervisor.psm1') -Raw
    $exitContractScript = Get-Content -LiteralPath (Join-Path $projectRoot 'tests/runner/test_cmd_exit_code_contract.ps1') -Raw
    $adapterHarness = Get-Content -LiteralPath (Join-Path $projectRoot 'tests/plugin/runtime_native_adapter_harness.cpp') -Raw
    if ($runnerScript -notmatch '\.runner' -or
        $runnerScript -notmatch '\$OutputDirectory' -or
        $runnerScript -notmatch 'Invoke-TestProcessTree' -or
        $runnerScript -notmatch '\$result\.ExitCode' -or
        $runnerScript -notmatch '\$exitCode\s*-ne\s*0' -or
        $runnerScript -notmatch 'exit 124' -or
        $runnerScript -notmatch 'CAMERA_TWEAKS_TEST_SUITE_TIMEOUT_MS' -or
        $runnerScript -notmatch 'StreamOutput' -or
        $runnerScript -match '\.Kill\s*\(' -or
        $orchestratorScript -notmatch 'Invoke-TestProcessTree' -or
        $orchestratorScript -notmatch "'-C'" -or
        $orchestratorScript -notmatch "buildDirectory, 'test_harnesses', '-j', '1'" -or
        $orchestratorScript -notmatch '\-OutputDirectory' -or
        $orchestratorScript -notmatch '\$ninjaExecutable' -or
        $orchestratorScript -notmatch 'test_harness_runner\.ps1' -or
        $supervisorScript -notmatch 'CreateJobObject' -or
        $supervisorScript -notmatch 'CreateSuspended' -or
        $supervisorScript -notmatch 'CreateProcess' -or
        $supervisorScript -notmatch 'AssignProcessToJobObject' -or
        $supervisorScript -notmatch 'JobObjectLimitKillOnJobClose' -or
        $supervisorScript -notmatch 'TerminateJobObject' -or
        $supervisorScript -notmatch 'WaitForTreeEmpty' -or
        $supervisorScript -notmatch 'CleanupVerified' -or
        $exitContractScript -match 'WaitForExit\(\)' -or
        $exitContractScript -match '\.Kill\s*\(' -or
        $exitContractScript -notmatch 'parent_child_grandchild_timeout' -or
        $exitContractScript -notmatch 'IsProcessRunning' -or
        $adapterHarness -notmatch 'cleanupWait\s*!=\s*WAIT_OBJECT_0' -or
        $adapterHarness -notmatch 'child cleanup unverified') {
        $missingInputs.Add('serial_registry_runner_contract_missing')
    }
}

$legacyPositiveOnlyGuards = @($testCmdText -split "`r?`n" | Where-Object { $_.Trim() -match '^if errorlevel\s+1\s+goto\s+:fail$' })
if ($legacyPositiveOnlyGuards.Count -ne 0) { $missingInputs.Add('positive_only_exit_guard_remains') }

Write-Output (("registry_targets={0} repository_harnesses={1} runner_invocations={2} " +
    "native_cases={3} native_children={4} resource_targets={5} missing_inputs={6}") -f
    $harnesses.Count, $repositoryHarnesses.Count, $runnerEntries.Count,
    $scenarioNames.Count, $registry.native_adapter_contract.child_process_count,
    $resourceUsers.Count, $missingInputs.Count)

if ($missingHarnesses.Count -ne 0) { Write-Output 'unregistered_harnesses:'; $missingHarnesses }
if ($unknownHarnesses.Count -ne 0) { Write-Output 'unknown_registry_harnesses:'; $unknownHarnesses }
if ($missingFlags.Count -ne 0) { Write-Output 'missing_required_flags:'; $missingFlags }
if ($missingInputs.Count -ne 0) { Write-Output 'registry_audit_failures:'; $missingInputs; exit 1 }
Write-Output 'canonical_test_registry=PASS'
exit 0
