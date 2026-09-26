$ErrorActionPreference = 'Stop'

function Read-StrictUtf8([string] $Path) {
    $decoder = [System.Text.UTF8Encoding]::new($false, $true)
    return $decoder.GetString([System.IO.File]::ReadAllBytes($Path))
}

function Get-LeafKeys($Object, [string] $Prefix = '') {
    foreach ($property in $Object.PSObject.Properties) {
        $path = if ($Prefix) { "$Prefix.$($property.Name)" } else { $property.Name }
        if ($property.Value -is [System.Management.Automation.PSCustomObject]) {
            Get-LeafKeys $property.Value $path
        } else {
            $path
        }
    }
}

function Test-SelectorCaptureContract([string] $Source) {
    $body = [regex]::Match($Source,
        'SelectorComboResult(?:<[^>]+>)?\s+DrawSelectorCombo\([\s\S]*?(?=\n\s*ImVec4\s+FeatureStatusColor)').Value
    if (-not $body) { return $false }
    $begin = $body.IndexOf('ImGui::BeginCombo(')
    $hover = $body.IndexOf('ImGui::IsItemHovered(')
    $active = $body.IndexOf('ImGui::IsItemActive(')
    $options = $body.IndexOf('ImGuiListClipper')
    $end = $body.IndexOf('ImGui::EndCombo(')
    return $begin -ge 0 -and $hover -gt $begin -and $active -gt $begin -and
        $options -gt $hover -and $options -gt $active -and $end -gt $options -and
        $body -match 'popupOpen\s*=\s*ImGui::BeginCombo' -and
        $body -match 'interaction\s*\{[\s\S]*?popupOpen'
}

function Test-CameraStateReadOnlyBoundary([string] $Header, [string] $Source) {
    return $Header -match 'const\s+plugin::OverlaySemanticSnapshot&\s+semantic' -and
        $Source -match 'void\s+DrawCameraStateView\(' -and
        $Source -match 'semantic\.cameraState' -and
        $Source -notmatch '\b(?:ApplySetting|RuntimeSettingMutation|Persist|SaveConfig|WriteConfig)\s*\('
}

function Test-LocalizedPresentationPolicy([string[]] $Sources) {
    $directUserCopy = [regex]::Matches(($Sources -join "`n"),
        'ImGui::Text(?:Unformatted|Disabled|Wrapped)?\("(?!(?:##|Overlay Rendering POC|Presentation path: OK|D3D12 queue: DIRECT|Buffers: %u))[A-Za-z][^"%]{2,}"')
    return $directUserCopy.Count -eq 0
}

function Test-NativePassThroughBoundary([string] $Source) {
    $present = [regex]::Match($Source,
        'HRESULT STDMETHODCALLTYPE HookPresent\([\s\S]*?(?=\n\s*HRESULT STDMETHODCALLTYPE HookResizeBuffers)').Value
    $resize = [regex]::Match($Source,
        'HRESULT STDMETHODCALLTYPE HookResizeBuffers\([\s\S]*?(?=\n\s*void InstallSwapchainHooks)').Value
    $window = [regex]::Match($Source,
        'LRESULT CALLBACK OverlayWindowProc\([\s\S]*?(?=\n\s*void InstallInputHook)').Value
    return $present -match 'RunOptionalOverlayWork\([\s\S]*?\},\s*&DisableAfterOverlayException\);[\s\S]{0,400}PresentFn original[\s\S]*?return original\(self' -and
        $resize -match 'RunOptionalOverlayWork\([\s\S]*?\},\s*&DisableAfterOverlayException\);[\s\S]{0,400}if \(!original\)[\s\S]*?const HRESULT result = original\(' -and
        $resize -match 'const HRESULT result = original\([\s\S]*?RunOptionalOverlayWork\([\s\S]*?return result;' -and
        $window -match 'RunOptionalOverlayWork\([\s\S]*?\},\s*&DisableAfterOverlayException\);[\s\S]*?CallWindowProcW\('
}

function Test-HotkeyStableIdentity([string] $Body) {
    return $Body -match 'stableButtonLabel\s*\+=\s*"##hotkey_binding"' -and
        $Body -match 'PushID\(static_cast<int>\(action\)\)' -and
        $Body -match 'Button\(stableButtonLabel\.c_str\(\),\s*ImVec2\(buttonWidth,\s*0\.0f\)\)' -and
        $Body -match 'CalcTextSize\(buttonLabel\)'
}

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
# Source-policy checks below are deliberate, not substitutes for the runtime
# catalog/resource and harness tests: registry/RC/ID wiring must stay aligned.
$registryPath = Join-Path $projectRoot 'src\localization\locale_registry.hpp'
$registrySource = Get-Content -LiteralPath $registryPath -Raw
$descriptorMatches = [regex]::Matches($registrySource,
    'LocaleDescriptor\s*\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*(IDR_LOCALE_\d+)\s*,\s*(true|false)\s*\}')
$descriptors = @($descriptorMatches | ForEach-Object {
    [pscustomobject]@{
        DisplayName = $_.Groups[1].Value
        Code = $_.Groups[2].Value
        FileName = $_.Groups[3].Value
        FontProfile = $_.Groups[4].Value
        InitialFontSize = [int]$_.Groups[5].Value
        Resource = $_.Groups[6].Value
        Canonical = $_.Groups[7].Value -ceq 'true'
    }
})
$canonicalDescriptors = @($descriptors | Where-Object Canonical)
$registryValid = $descriptors.Count -ge 2 -and $canonicalDescriptors.Count -eq 1 -and
    $descriptors.Count -eq 18 -and $canonicalDescriptors[0].Code -ceq 'en' -and
    (@($descriptors.Code | Sort-Object -Unique).Count -eq $descriptors.Count) -and
    (@($descriptors.DisplayName | Sort-Object -Unique).Count -eq $descriptors.Count) -and
    (@($descriptors.Resource | Sort-Object -Unique).Count -eq $descriptors.Count) -and
    (@($descriptors.FileName | Sort-Object -Unique).Count -eq $descriptors.Count) -and
    (@($descriptors | Where-Object {
        $_.InitialFontSize -lt 12 -or $_.InitialFontSize -gt 24
    }).Count -eq 0) -and
    (@($descriptors | Where-Object {
        $_.Code -notmatch '^[a-z]{2,3}(?:-(?:[A-Z]{2}|[0-9]{3}|[A-Z][a-z]{3}|[a-z0-9]{4,8}))*$' -or
        $_.FileName -cne ($_.Code + '.json') -or
        [string]::IsNullOrWhiteSpace($_.FontProfile) -or
        [string]::IsNullOrWhiteSpace($_.DisplayName)
    }).Count -eq 0)

$loadedCatalogs = @()
$catalogReadErrors = @()
foreach ($descriptor in $descriptors) {
    if ([IO.Path]::GetFileName($descriptor.FileName) -cne $descriptor.FileName) {
        $catalogReadErrors += "$($descriptor.Code): invalid catalog filename"
        continue
    }
    $catalogPath = Join-Path $projectRoot (Join-Path 'locales' $descriptor.FileName)
    try {
        $localeCatalog = Read-StrictUtf8 $catalogPath | ConvertFrom-Json
        $localeKeys = @(Get-LeafKeys $localeCatalog | Sort-Object)
        $loadedCatalogs += [pscustomobject]@{
            Descriptor = $descriptor
            Catalog = $localeCatalog
            Keys = $localeKeys
        }
    } catch {
        $catalogReadErrors += "$($descriptor.Code): $($_.Exception.Message)"
    }
}
$canonicalCode = if ($canonicalDescriptors.Count -eq 1) {
    $canonicalDescriptors[0].Code
} else { '' }
$canonicalEntry = @($loadedCatalogs | Where-Object { $_.Descriptor.Code -ceq $canonicalCode }) |
    Select-Object -First 1
$catalog = if ($canonicalEntry) { $canonicalEntry.Catalog } else { [pscustomobject]@{} }
$catalogKeys = if ($canonicalEntry) { $canonicalEntry.Keys } else { @() }

$keySource = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\localization_keys.hpp') -Raw
$typedKeys = @([regex]::Matches($keySource, '\{"([a-zA-Z0-9_.]+)"\}') |
    ForEach-Object { $_.Groups[1].Value } | Sort-Object)
$missing = @($typedKeys | Where-Object { $_ -notin $catalogKeys })
$extra = @($catalogKeys | Where-Object { $_ -notin $typedKeys })
$technicalIdentifierMismatches = @()
$catalogParityErrors = @()
$placeholderMismatches = @()
$technicalTokenPattern = '(?<![A-Za-z0-9])(?:AspectRecalculation|GameplayHorPlus|NativeHorPlus|HorPlus|Hotkeys\.Enabled|ADS|FOV|16:9|21:9|32:9)(?![A-Za-z0-9])'
foreach ($entry in $loadedCatalogs) {
    if ($entry.Descriptor.Canonical) { continue }
    $localeMissing = @($catalogKeys | Where-Object { $_ -notin $entry.Keys })
    $localeExtra = @($entry.Keys | Where-Object { $_ -notin $catalogKeys })
    if ($localeMissing.Count -or $localeExtra.Count) {
        $catalogParityErrors += "$($entry.Descriptor.Code): missing=[$($localeMissing -join ',')] extra=[$($localeExtra -join ',')]"
    }
    foreach ($key in $catalogKeys) {
        if ($key -notin $entry.Keys) { continue }
        $canonicalNode = $catalog
        $localeNode = $entry.Catalog
        foreach ($part in $key.Split('.')) {
            $canonicalNode = $canonicalNode.$part
            $localeNode = $localeNode.$part
        }
        $canonicalPlaceholders = @([regex]::Matches([string]$canonicalNode,
            '\{([A-Za-z][A-Za-z0-9_]*)\}') | ForEach-Object { $_.Groups[1].Value } | Sort-Object)
        $localePlaceholders = @([regex]::Matches([string]$localeNode,
            '\{([A-Za-z][A-Za-z0-9_]*)\}') | ForEach-Object { $_.Groups[1].Value } | Sort-Object)
        if (($canonicalPlaceholders -join '|') -cne ($localePlaceholders -join '|')) {
            $placeholderMismatches += "$($entry.Descriptor.Code):$key"
        }
        $requiredTokens = @([regex]::Matches([string]$canonicalNode, $technicalTokenPattern) |
            ForEach-Object { $_.Value } | Sort-Object -Unique)
        $missingTokens = @($requiredTokens | Where-Object {
            -not [regex]::IsMatch([string]$localeNode,
                '(?<![A-Za-z0-9])' + [regex]::Escape($_) + '(?![A-Za-z0-9])')
        })
        if ($missingTokens.Count) {
            $technicalIdentifierMismatches +=
                "$($entry.Descriptor.Code):$key[$($missingTokens -join ',')]"
        }
    }
}
$sourceFiles = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'src') -Recurse -File |
    Where-Object { $_.Extension -in @('.cpp', '.hpp') })
$sourceText = ($sourceFiles | ForEach-Object { Get-Content -LiteralPath $_.FullName -Raw }) -join "`n"
$legacyReferences = [regex]::Matches($sourceText,
    'GetEnglishCatalog|EnglishCatalog|englishCatalog_|ukrainianCatalog_|englishResourceId|ukrainianResourceId|Locale::(English|Ukrainian)|OverlayLanguage::')
$filesystemLocaleReferences = [regex]::Matches($sourceText,
    'LoadFile\s*\(|OverlayLocalePath|locales[\\/]en\.json')
$resourceScript = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\localization_resources.rc') -Raw
$resourceIds = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\localization_resource_ids.h') -Raw
# Structural policy: each registry resource maps to the declared embedded file
# and numeric resource ID; binary resource existence is also exercised by the
# localization harness and production resource compilation.
$resourcePresent = $registryValid
foreach ($descriptor in $descriptors) {
    $resourcePresent = $resourcePresent -and
        ($resourceScript -match ([regex]::Escape($descriptor.Resource) + '\s+RCDATA\s+"locales/' + [regex]::Escape($descriptor.FileName) + '"')) -and
        ($resourceIds -match ('#define\s+' + [regex]::Escape($descriptor.Resource) + '\s+\d+'))
}
$resourcePresent = $resourcePresent -and
    ($resourceScript -match 'IDR_FONT_CYRILLIC\s+RCDATA\s+"assets/fonts/ProggyVector/ProggyVector-Regular\.ttf"') -and
    ($resourceScript -match 'IDR_PROGGY_VECTOR_LICENSE\s+RCDATA\s+"assets/fonts/ProggyVector/ProggyVector_Licence\.txt"') -and
    ($resourceScript -match 'IDR_FONT_CATALOG_COMMON\s+RCDATA\s+"assets/fonts/CatalogSubsets/NotoSansMono-Catalog\.ttf"') -and
    ($resourceScript -match 'IDR_FONT_ARABIC\s+RCDATA\s+"assets/fonts/CatalogSubsets/NotoSansArabic-Catalog\.ttf"') -and
    ($resourceScript -match 'IDR_FONT_JAPANESE\s+RCDATA\s+"assets/fonts/CatalogSubsets/NotoSansJP-Catalog\.ttf"') -and
    ($resourceScript -match 'IDR_FONT_KOREAN\s+RCDATA\s+"assets/fonts/CatalogSubsets/NotoSansKR-Catalog\.ttf"') -and
    ($resourceScript -match 'IDR_FONT_CHINESE_SIMPLIFIED\s+RCDATA\s+"assets/fonts/CatalogSubsets/NotoSansSC-Catalog\.ttf"') -and
    ($resourceScript -match 'IDR_FONT_CHINESE_TRADITIONAL\s+RCDATA\s+"assets/fonts/CatalogSubsets/NotoSansTC-Catalog\.ttf"') -and
    ($resourceScript -match 'IDR_NOTO_ARABIC_OFL\s+RCDATA\s+"assets/fonts/CatalogSubsets/OFL-Arabic\.txt"') -and
    ($resourceScript -match 'IDR_NOTO_MONO_OFL\s+RCDATA\s+"assets/fonts/NotoSansMono/OFL\.txt"') -and
    ($resourceScript -match 'IDR_NOTO_CJK_NOTICE\s+RCDATA\s+"assets/fonts/CatalogSubsets/CJK-Copyright\.txt"') -and
    ($resourceIds -match '#define\s+IDR_FONT_CYRILLIC\s+103') -and
    ($resourceIds -match '#define\s+IDR_FONT_CATALOG_COMMON\s+121') -and
    ($resourceIds -match '#define\s+IDR_FONT_ARABIC\s+122') -and
    ($resourceIds -match '#define\s+IDR_FONT_JAPANESE\s+123') -and
    ($resourceIds -match '#define\s+IDR_FONT_KOREAN\s+124') -and
    ($resourceIds -match '#define\s+IDR_FONT_CHINESE_SIMPLIFIED\s+125') -and
    ($resourceIds -match '#define\s+IDR_FONT_CHINESE_TRADITIONAL\s+126') -and
    ($resourceIds -match '#define\s+IDR_NOTO_ARABIC_OFL\s+128') -and
    ($resourceIds -match '#define\s+IDR_NOTO_MONO_OFL\s+127') -and
    ($resourceIds -match '#define\s+IDR_NOTO_CJK_NOTICE\s+129') -and
    ($resourceIds -notmatch 'IDR_LOCALE_EN|IDR_LOCALE_UK|IDR_UI_FONT')
$fontLoader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\localization_font.cpp') -Raw
$fontLoaderHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\localization_font.hpp') -Raw
# Intentional source policy: reject filesystem/system-font fallbacks and keep
# script-profile/glyph-range wiring explicit. The localization harness checks
# the resulting embedded glyph/profile behavior across all registered locales.
$embeddedFontLoading = $fontLoader -match 'AddFontDefault\s*\(' -and
    $fontLoader -notmatch 'segoeui\.ttf|GetWindowsDirectory|AddFontFromFileTTF|Windows\\Fonts' -and
    $fontLoader -match 'AddFontFromMemoryTTF' -and
    $fontLoader -match 'IDR_FONT_CYRILLIC' -and
    $fontLoader -match 'IDR_FONT_CATALOG_COMMON' -and
    $fontLoader -match 'IDR_FONT_ARABIC' -and
    $fontLoader -match 'IDR_FONT_JAPANESE' -and
    $fontLoader -match 'IDR_FONT_KOREAN' -and
    $fontLoader -match 'IDR_FONT_CHINESE_SIMPLIFIED' -and
    $fontLoader -match 'IDR_FONT_CHINESE_TRADITIONAL' -and
    $fontLoader -match 'profileCode == "base"' -and
    $fontLoader -match 'for\s*\(const auto& profile : ScriptFontProfiles\)' -and
    $fontLoader -match 'SizePixels\s*=\s*static_cast<float>\(fontSizePixels\)' -and
    $fontLoader -match 'MergeMode\s*=\s*true' -and
    $fontLoader -match 'PixelSnapH\s*=\s*true' -and
    $fontLoader -match 'OversampleH\s*=\s*1' -and
    $fontLoader -match 'OversampleV\s*=\s*1' -and
    $fontLoader -match 'LocaleRegistry' -and
    $fontLoader -match 'locale\.displayName' -and
    $fontLoader -match 'BuildSelectorGlyphRanges' -and
    $fontLoader -match 'selectorGlyphRanges' -and
    $fontLoaderHeader -match 'AddLocalizationSelectorFonts' -and
    $fontLoaderHeader -match 'LocalizationSelectorFont' -and
    $fontLoader -match '0x0400,\s*0x052F' -and
    $fontLoader -match '0x0600,\s*0x06FF' -and
    $fontLoader -match '0x3000,\s*0x30FF' -and
    $fontLoader -match '0xAC00,\s*0xD7AF' -and
    $fontLoader -notmatch '0x0020,\s*0x00FF'
$renderer = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\renderer_runtime.cpp') -Raw
$hotkeyBindingBody = [regex]::Match($renderer,
    'void DrawHotkeyBinding\([\s\S]*?(?=\n\s*FeatureStatusTone CameraTransitionTone)').Value
$cameraStateView = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\camera_state_view.cpp') -Raw
$cameraStateViewHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\camera_state_view.hpp') -Raw
$inputStateHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\input_state.hpp') -Raw
$inputStateSource = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\input_state.cpp') -Raw
$selectorDocumentationView = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\selector_documentation_view.cpp') -Raw
$selectorDocumentationViewHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\selector_documentation_view.hpp') -Raw
$tooltipRowLayout = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\tooltip_row_layout.hpp') -Raw
$placementSaveState = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\placement_save_state.hpp') -Raw
$selectorTooltipState = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\selector_tooltip_state.hpp') -Raw
$imguiWidgets = Get-Content -LiteralPath (Join-Path $projectRoot 'external\imgui\imgui_widgets.cpp') -Raw
$settingTooltipContent = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\setting_tooltip_content.hpp') -Raw
$discoveryRuntime = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\discovery_runtime.cpp') -Raw
$nativePassThroughBoundary = Test-NativePassThroughBoundary $discoveryRuntime
$nativeBoundaryNegativeFixture = -not (Test-NativePassThroughBoundary @'
HRESULT STDMETHODCALLTYPE HookPresent() { return original(); RunOptionalOverlayWork(work, fail); }
HRESULT STDMETHODCALLTYPE HookResizeBuffers() { return original(); RunOptionalOverlayWork(work, fail); }
LRESULT CALLBACK OverlayWindowProc() { return CallWindowProcW(proc); }
void InstallInputHook
'@)
$mutexExceptionBoundary = $inputStateHeader -match 'void Close\(\);' -and
    $inputStateHeader -match 'HandleRebindMessage\([\s\S]*?\);' -and
    $inputStateHeader -notmatch 'HandleRebindMessage\([^;]*\)\s*noexcept' -and
    $inputStateSource -match 'void InputState::Close\(\)' -and
    $inputStateSource -match 'std::lock_guard lock\(rebindMutex_\)' -and
    $renderer -match 'GetInputState\(\)\.Close\(\);[\s\S]*?catch \(\.\.\.\)'
$languageReader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\game_language_reader.cpp') -Raw
$manager = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\localization_manager.cpp') -Raw
$localizationHarness = Get-Content -LiteralPath (Join-Path $projectRoot 'tests\overlay\localization_harness.cpp') -Raw
$selectorGeometry = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\selector_examples_geometry.hpp') -Raw
$featurePresentation = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\feature_presentation.cpp') -Raw
$featurePresentationHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\overlay\feature_presentation.hpp') -Raw
# The remaining UI source checks are narrowly scoped UI/source policies (typed
# localization keys, no legacy catalogs, no raw UI literals, accepted tooltip
# and notification presentation, and the restored layout contract). Semantic
# status/catalog/font behavior is covered by dedicated C++ harnesses; ImGui
# rendering is not available as a reliable headless visual oracle here.
# Auto-sync's Win32 post/message route is retained as a structural boundary
# check, while its visible+Auto decision now has a direct constexpr behavior test.
$integerFontSizing = $renderer -notmatch 'FontGlobalScale' -and
    $renderer -match 'RebuildFontAtlas\(\s*frameSettings\.overlayFontSize,\s*fontProfileCode\)' -and
    $renderer -match 'OVERLAY_FONT_ATLAS_RESTORED'
$cameraStateViewBoundary = (Test-CameraStateReadOnlyBoundary $cameraStateViewHeader $cameraStateView) -and
    $cameraStateView -match 'camera::EvidenceProvenanceName' -and
    $cameraStateView -match 'ImGui::BeginTable\(' -and
    $renderer -match 'ImGui::TreeNodeEx\(cameraStateLabel\.c_str\(' -and
    $renderer -match 'DrawCameraStateView\(localization_, semantic\)' -and
    $renderer -notmatch 'void DrawCameraState\('
$buildImGuiBody = [regex]::Match($renderer,
    'bool Renderer::BuildImGui\(\)[\s\S]*?(?=\n\s*bool Renderer::WaitForGpu)').Value
$rendererInitializeBody = [regex]::Match($renderer,
    'bool Renderer::InitializeImpl\([\s\S]*?(?=\n\s*bool Renderer::BuildResources)').Value
$localizationFailClosed = $buildImGuiBody -match 'if\s*\(!localization_\.Initialize\([\s\S]*?return false;' -and
    $rendererInitializeBody -match 'BuildImGui\(\)\)\s*return fail\(\)' -and
    $discoveryRuntime -match 'if\s*\(g_renderer\.Initialize\([\s\S]*?InstallInputHook\(window\)'
$registryDrivenSelector = $renderer -match 'for\s*\(const auto& locale : localization_\.locales\(\)\)' -and
    $renderer -match 'ImGui::Selectable\(locale\.displayName\.data\(\)' -and
    $renderer -match 'LocalizationSelectorFont\(locale\.fontProfileCode\)' -and
    $renderer -match 'AddLocalizationSelectorFonts\(fontSizePixels\)'
$autoSelectionSync = $renderer -match 'RuntimeSettingMutation::AutoLocale\(true\)' -and
    $renderer -match 'RequestAutoLanguageSynchronization\(window_\)' -and
    $discoveryRuntime -match 'message == overlay::AutoLanguageSyncMessage' -and
    $discoveryRuntime -match 'SynchronizeAutoLanguageIfConfigured\(\)' -and
    $languageReader -match 'PostMessageW\(window, AutoLanguageSyncMessage' -and
    $languageReader -match 'ShouldSynchronizeAutoLanguage\(overlayVisible,' -and
    $renderer -match 'StartupLocaleReady\(frameSettings\.overlayLocaleAuto,' -and
    $renderer -match 'overlayLocaleAutoSynchronized'
$inlineHotkeyRenderBody = [regex]::Match($renderer,
    'void DrawInlineHotkeyMessage\([\s\S]*?(?=\n\s*void DrawNotificationHeader)').Value
$notificationInlineWrap = $inlineHotkeyRenderBody -match 'afterWidth > ImGui::GetContentRegionAvail\(\)\.x' -and
    $inlineHotkeyRenderBody -match 'ImGui::NewLine\(\)' -and
    $inlineHotkeyRenderBody -match 'ImGui::TextWrapped\("%\.\*s",'
$canonicalSettingOptionNames = $settingTooltipContent -match 'AspectRecalculation' -and
    $settingTooltipContent -match 'HorPlus' -and $settingTooltipContent -match 'Forced16x9' -and
    $settingTooltipContent -match 'GameplayHorPlus' -and $settingTooltipContent -match 'Adaptive' -and
    $settingTooltipContent -match 'canonicalName' -and
    $settingTooltipContent -notmatch 'nameKey|loc::option::' -and
    $renderer -match 'GameplaySelectorOptions\(' -and
    $renderer -match 'CinematicAspectSelectorOptions\(' -and
    $renderer -match 'CinematicFovSelectorOptions\(' -and
    $renderer -match 'DialogueSelectorOptions\(' -and
    $renderer -notmatch 'static_cast<config::(?:GameplayMode|CinematicAspectPolicy|CinematicFovMode|DialogueZoomPolicy)>\('
$dialogueModeIdentifiers = $renderer -match 'ActiveDialoguePolicyLabel\(config::DialogueZoomPolicy policy\)' -and
    $renderer -match 'return config::DialogueZoomPolicyName\(policy\)'
$settingTooltipBody = [regex]::Match($selectorDocumentationView,
    'void ShowSettingOptionsTooltip\([\s\S]*?\n    \}\r?\n\}').Value
$settingTooltipRenderBody = $settingTooltipBody
$alignedTooltipBody = $tooltipRowLayout
$optionListTooltipBody = [regex]::Match($renderer,
    'void ShowOptionListTooltip\([\s\S]*?(?=\n\s*template <std::size_t Count>\s*\n\s*void DrawRuntimeInfoRow)').Value
$examplesBlockBody = [regex]::Match($selectorDocumentationView,
    'void DrawTooltipExamplesBlock\([\s\S]*?(?=\n        \}\r?\n    \})').Value
$selectorTooltipColumns = $alignedTooltipBody -match 'std::max' -and
    $alignedTooltipBody -match 'SameLine\([^)]*separator' -and
    $alignedTooltipBody -match 'SameLine\([^)]*description' -and
    $alignedTooltipBody -match 'drawDescriptionAt\([^;]*description' -and
    $settingTooltipBody -match 'tooltip_row_layout::DrawAlignedTooltipRows\(' -and
    $optionListTooltipBody -match 'tooltip_row_layout::DrawAlignedTooltipRows\(' -and
    $settingTooltipBody -match 'DrawTooltipExamplesBlock\('
$examplesGeometryContract = $selectorDocumentationView -match 'CalculateFrameGeometry\(' -and
    $selectorDocumentationView -match 'geometry\.contentMin' -and
    $selectorDocumentationView -match 'completedGeometry\.frameMax' -and
    $selectorGeometry -match 'horizontalInset' -and $selectorGeometry -match 'verticalInset' -and
    $localizationHarness -match '10\.0f \* scale' -and
    $localizationHarness -match '8\.0f \* scale' -and
    $localizationHarness -match 'fontSize : std::array\{14\.0f, 21\.0f\}'
$selectorTooltipSpacingScope = $settingTooltipBody -match 'PushStyleVar\(ImGuiStyleVar_ItemSpacing,' -and
    $settingTooltipBody -match 'ImVec2\(ImGui::GetStyle\(\)\.ItemSpacing\.x, 0\.0f\)' -and
    $settingTooltipBody -match 'tooltip_row_layout::DrawAlignedTooltipRows\(options\.size\(\)' -and
    $settingTooltipBody -match 'ImGui::PopStyleVar\(\)'
$selectorComboBody = [regex]::Match($renderer,
    'SelectorComboResult DrawSelectorCombo\([\s\S]*?(?=\n\s*ImVec4 FeatureStatusColor)').Value
$selectorTooltipInteraction = (Test-SelectorCaptureContract $renderer) -and
    $selectorTooltipState -match 'state\.hovered && !state\.active && !state\.popupOpen' -and
    $settingTooltipRenderBody -match 'IsSelectorTooltipEligible\(selectorState\)' -and
    $settingTooltipRenderBody -notmatch 'IsItemHovered|IsItemActive|GetItemID|GetItemRect' -and
    $selectorDocumentationViewHeader -match 'std::span<const setting_tooltip_content::Option> options' -and
    $renderer -match 'ShowSettingOptionsTooltip\(localization_, setting_tooltip_content::Gameplay\(\),\s*gameplayCombo\.interaction\)' -and
    $renderer -match 'ShowSettingOptionsTooltip\(localization_, setting_tooltip_content::CinematicAspect\(\),\s*cinematicAspectCombo\.interaction\)' -and
    $renderer -match 'ShowSettingOptionsTooltip\(localization_, setting_tooltip_content::CinematicFov\(\),\s*cinematicFovCombo\.interaction\)' -and
    $renderer -match 'ShowSettingOptionsTooltip\(localization_, setting_tooltip_content::Dialogue\(\),\s*dialogueCombo\.interaction\)' -and
    $renderer -notmatch 'SELECTOR_TOOLTIP_TRACE|OverlayCaptureDialogueZoomComboTrace' -and
    $imguiWidgets -notmatch 'SELECTOR_TOOLTIP_TRACE|OverlayCaptureDialogueZoomComboTrace'
$selectorCaptureNegativeFixture = -not (Test-SelectorCaptureContract @'
SelectorComboResult DrawSelectorCombo() {
 const bool popupOpen = ImGui::BeginCombo(id, preview);
 ImGuiListClipper clipper;
 const bool hovered = ImGui::IsItemHovered();
 ImGui::EndCombo();
}
ImVec4 FeatureStatusColor
'@)
$cameraStateNegativeFixtureResult = Test-CameraStateReadOnlyBoundary -Header 'const plugin::OverlaySemanticSnapshot& semantic' -Source 'void DrawCameraStateView() { ApplySetting(RuntimeSettingMutation{}); }'
$cameraStateNegativeFixture = -not $cameraStateNegativeFixtureResult
$localizedPresentationModules = @($renderer, $cameraStateView, $selectorDocumentationView)
$localizedProseFixture = @'
ImGui::TextUnformatted("Hard-coded user-facing prose");
'@
$localizedPresentationPolicy = (Test-LocalizedPresentationPolicy $localizedPresentationModules) -and
    $featurePresentationHeader -match 'ProjectGameplayPresentation' -and
    $featurePresentationHeader -match 'ProjectCinematicPresentation' -and
    -not (Test-LocalizedPresentationPolicy @($localizedProseFixture))
$presentationOwnership = $featurePresentation -match 'ProjectGameplayPresentation\(' -and
    $featurePresentation -match 'ProjectCinematicPresentation\(' -and
    $renderer -match 'DrawFeatureStatus\(' -and
    $renderer -match 'DrawCameraStateView\(' -and
    $renderer -match 'ShowSettingOptionsTooltip\('
$conditionalExamplesOptionGap = $settingTooltipBody -match 'examples\.empty\(\)' -and
    $settingTooltipBody -match 'index\s*\+\s*1\s*<\s*options\.size\(\)' -and
    $settingTooltipBody -match 'ImGui::Dummy\('
$namedVisualContracts = $tooltipRowLayout -match 'WrapWidthInFontUnits' -and
    $selectorDocumentationView -match 'tooltip_row_layout::WrapWidthInFontUnits' -and
    $selectorGeometry -match 'BaseFontSizePixels' -and
    $selectorGeometry -match 'HorizontalInnerInsetAtBaseScale' -and
    $selectorGeometry -match 'VerticalInnerInsetAtBaseScale' -and
    $selectorDocumentationView -match 'NextOptionGapAtBaseScale' -and
    $renderer -match 'FeatureStatusColor\(statusTone\)'
$placementSaveContract = $renderer -match 'placementSaveState_\.PositionChanged\(\)' -and
    $renderer -match 'placementSaveState_\.ShouldAttemptSave\(leftMouseButtonDown\)' -and
    $renderer -match 'placementSaveState_\.CompleteAttempt\(saved\)' -and
    $placementSaveState -match 'FailedUntilNextMovement' -and
    $placementSaveState -match 'PendingAfterMovement'
$tooltipContentOwnership = ($renderer + $selectorDocumentationView) -notmatch 'Recommended\. Preserves native Gameplay FOV|Applies HorPlus to the authored cinematic FOV|Use half of the Adaptive optical zoom strength'
$featureStatusBody = [regex]::Match($renderer,
    'void DrawFeatureStatus\([\s\S]*?(?=\n\s*void DrawActiveStatus)').Value
$statusPresentation = $renderer -notmatch 'void ShowStatusTooltip\(' -and
    $renderer -notmatch 'gameplayStatusOptions|cinematicStatusOptions|dialogueStatusOptions' -and
    $renderer -match 'DrawFeatureStatus\(localization_, ProjectGameplayPresentation\(semantic\)\)' -and
    $featureStatusBody -notmatch 'ImGui::NewLine\(\)' -and
    $featureStatusBody -match 'if \(!status\.detail\.path\.empty\(\)\)' -and
    $renderer -notmatch 'ImGui::TextDisabled\("\(\?\)"\)' -and
    $renderer -notmatch 'ShowCameraTransitionTooltip' -and
    $renderer -match 'SeparatorText\(transitionHeader\.c_str\(\)\)' -and
    $renderer -match 'CameraTransitionDetail\(integration\.overall\)'
$restoredOriginalLayout = $renderer -notmatch 'DrawSectionHeading|LocalizedLabelWidth|controlWidthLimit|hotkeyKeyColumn|minOverlayWidth' -and
    $renderer -match 'ImGuiTableFlags_SizingFixedSame \|\s*ImGuiTableFlags_BordersInnerV' -and
    $renderer -match 'SetNextWindowSizeConstraints\(ImVec2\(0\.0f, 0\.0f\)' -and
    $renderer -match 'SetNextItemWidth\(commonSelectWidth\)' -and
    $renderer -match 'SeparatorText\(runtimeHeader\.c_str\(\)\)' -and
    $renderer -match 'for\s*\(const auto& binding : HotkeyBindingPresentations\)' -and
    $renderer -match 'DrawHotkeyBinding\(localization_, binding\.label, binding\.id,'
$stableHotkeyWidgetIdentity = Test-HotkeyStableIdentity $hotkeyBindingBody
$hotkeyIdentityNegativeFixture = -not (Test-HotkeyStableIdentity @'
const char* buttonLabel = currentLabel;
if (ImGui::Button(buttonLabel, ImVec2(buttonWidth, 0.0f))) {}
'@)
$inlineHotkeyBody = [regex]::Match($renderer,
    'void DrawInlineHotkeyMessage\([\s\S]*?(?=\n\s*void )').Value
$inlineHotkeyPresentation = $renderer -match 'void DrawInlineHotkeyMessage\(' -and
    $renderer -match 'ImGui::TextColored\(ImVec4\(0\.18f, 0\.78f, 1\.0f, 1\.0f\)' -and
    $inlineHotkeyBody -notmatch 'ImGui::Button'
$notificationHeader = $renderer -match 'void DrawNotificationHeader\(' -and
    $renderer -match 'AddRectFilled\(windowPos[\s\S]{0,300}ImGuiCol_TitleBgActive' -and
    $renderer -match 'DrawNotificationHeader\(localizedTitle, alpha\)'
$localizedHotkeyCopy = $true
$overlayTooltipCopy = $true
foreach ($entry in $loadedCatalogs) {
    $localizedHotkeyCopy = $localizedHotkeyCopy -and
        -not [string]::IsNullOrWhiteSpace([string]$entry.Catalog.tooltip.hotkeys.change) -and
        -not [string]::IsNullOrWhiteSpace([string]$entry.Catalog.tooltip.hotkeys.change_overlay)
    $overlayTooltipCopy = $overlayTooltipCopy -and
        [string]$entry.Catalog.tooltip.overlay.always_active -notmatch 'Hotkeys\.Enabled'
}
$genericManager = $manager -match 'for\s*\(const auto& descriptor : localization::LocaleRegistry\)' -and
    $manager -match 'FindLocaleDescriptor\(localeCode\)'
$rawRendererKeys = [regex]::Matches($renderer, '\bTr\s*\(\s*["'']')
$declaredNames = @([regex]::Matches($keySource, 'inline constexpr Key (\w+)\{"([\w.]+)"\}') |
    ForEach-Object { [pscustomobject]@{ Name = $_.Groups[1].Value; Path = $_.Groups[2].Value } })
$usedNames = [regex]::Matches($sourceText, 'loc::(?:[A-Za-z_]\w*::)*([A-Za-z_]\w*)') |
    ForEach-Object { $_.Groups[1].Value }
$obsolete = @($declaredNames | Where-Object { $_.Name -notin $usedNames })
$pocText = [regex]::Matches($renderer, 'ImGui::Text(?:Unformatted)?\("(?:Overlay Rendering POC|Presentation path: OK|D3D12 queue: DIRECT|Buffers: %u)')
$unexpectedDirectUiText = [regex]::Matches(($localizedPresentationModules -join "`n"), 'ImGui::Text(?:Unformatted|Disabled|Wrapped)?\("(?!##|Overlay Rendering POC|Presentation path: OK|D3D12 queue: DIRECT|Buffers: %u)[A-Za-z][^"]{2,}"')

if (-not $registryValid -or $catalogReadErrors.Count -or $missing.Count -or $extra.Count -or
    $catalogParityErrors.Count -or $placeholderMismatches.Count -or $technicalIdentifierMismatches.Count -or
    $legacyReferences.Count -or $filesystemLocaleReferences.Count -or -not $resourcePresent -or
    -not $embeddedFontLoading -or -not $integerFontSizing -or -not $cameraStateViewBoundary -or -not $genericManager -or
    -not $registryDrivenSelector -or $rawRendererKeys.Count -or $obsolete.Count -or
    -not $autoSelectionSync -or -not $dialogueModeIdentifiers -or
    -not $canonicalSettingOptionNames -or -not $selectorTooltipColumns -or
    -not $examplesGeometryContract -or -not $selectorTooltipSpacingScope -or
    -not $selectorTooltipInteraction -or -not $selectorCaptureNegativeFixture -or
    -not $cameraStateNegativeFixture -or -not $localizedPresentationPolicy -or
    -not $presentationOwnership -or -not $nativePassThroughBoundary -or
    -not $nativeBoundaryNegativeFixture -or
    -not $mutexExceptionBoundary -or
    -not $localizationFailClosed -or
    -not $namedVisualContracts -or
    -not $placementSaveContract -or
    -not $conditionalExamplesOptionGap -or
    -not $tooltipContentOwnership -or -not $statusPresentation -or
    -not $restoredOriginalLayout -or
    -not $stableHotkeyWidgetIdentity -or -not $hotkeyIdentityNegativeFixture -or
    -not $inlineHotkeyPresentation -or -not $notificationInlineWrap -or
    -not $notificationHeader -or
    -not $localizedHotkeyCopy -or -not $overlayTooltipCopy -or
    $unexpectedDirectUiText.Count -or $pocText.Count -ne 4) {
    Write-Output ('registry_valid=' + $registryValid + ' locale_count=' + $descriptors.Count +
        ' canonical=' + $canonicalCode + ' catalog_errors=' + ($catalogReadErrors -join '|'))
    Write-Output ('canonical_missing=' + ($missing -join ',') + ' canonical_extra=' + ($extra -join ',') +
        ' locale_parity=' + ($catalogParityErrors -join '|') +
        ' placeholder_mismatches=' + ($placeholderMismatches -join ',') +
        ' technical_identifier_mismatches=' + ($technicalIdentifierMismatches -join '|'))
    Write-Output ('legacy_references=' + $legacyReferences.Count + ' filesystem_locale_refs=' +
        $filesystemLocaleReferences.Count + ' embedded_resources=' + $resourcePresent +
        ' generic_manager=' + $genericManager + ' registry_selector=' + $registryDrivenSelector +
        ' auto_selection_sync=' + $autoSelectionSync + ' dialogue_ids=' + $dialogueModeIdentifiers +
        ' canonical_setting_option_names=' + $canonicalSettingOptionNames +
        ' selector_tooltip_columns=' + $selectorTooltipColumns +
        ' examples_geometry_contract=' + $examplesGeometryContract +
        ' native_pass_through_boundary=' + $nativePassThroughBoundary +
        ' native_boundary_negative_fixture=' + $nativeBoundaryNegativeFixture +
        ' mutex_exception_boundary=' + $mutexExceptionBoundary +
        ' localization_fail_closed=' + $localizationFailClosed +
        ' named_visual_contracts=' + $namedVisualContracts +
        ' placement_save_contract=' + $placementSaveContract +
        ' selector_tooltip_spacing=' + $selectorTooltipSpacingScope +
        ' selector_tooltip_interaction=' + $selectorTooltipInteraction +
        ' selector_capture_negative_fixture=' + $selectorCaptureNegativeFixture +
        ' camera_state_negative_fixture=' + $cameraStateNegativeFixture +
        ' localized_presentation_policy=' + $localizedPresentationPolicy +
        ' presentation_ownership=' + $presentationOwnership +
        ' examples_geometry_contract=' + $examplesGeometryContract +
        ' conditional_examples_gap=' + $conditionalExamplesOptionGap +
        ' tooltip_content_ownership=' + $tooltipContentOwnership +
        ' status_presentation=' + $statusPresentation +
        ' restored_original_layout=' + $restoredOriginalLayout +
        ' stable_hotkey_widget_identity=' + $stableHotkeyWidgetIdentity +
        ' hotkey_identity_negative_fixture=' + $hotkeyIdentityNegativeFixture +
        ' inline_hotkeys=' + $inlineHotkeyPresentation +
        ' notification_inline_wrap=' + $notificationInlineWrap +
        ' notification_header=' + $notificationHeader + ' hotkey_copy=' + $localizedHotkeyCopy +
        ' overlay_tooltip=' + $overlayTooltipCopy +
        ' raw_renderer_keys=' + $rawRendererKeys.Count + ' obsolete_keys=' + (($obsolete.Path) -join ',') +
        ' unexpected_ui_literals=' + $unexpectedDirectUiText.Count + ' developer_poc_literals=' + $pocText.Count)
    Write-Output ('embedded_font_loading=' + $embeddedFontLoading + ' integer_font_sizes=' + $integerFontSizing +
        ' camera_state_view_boundary=' + $cameraStateViewBoundary)
    exit 1
}

Write-Output ('locales=' + $descriptors.Count + ' canonical=' + $canonicalCode +
    ' keys=' + $catalogKeys.Count + ' registry_lookup=PASS locale_parity=PASS placeholders=PASS utf8=PASS ' +
    'embedded_resources=PASS generic_manager=PASS registry_selector=PASS single_asi=PASS ' +
    'embedded_font_profiles=PASS locale_metadata_glyphs=PASS selector_font_faces=PASS ' +
    'camera_state_view_boundary=PASS ' +
    'proggyclean_merge=PASS integer_font_sizes=PASS')
Write-Output ('font_rendering_scope=offline embedded glyph coverage; ' +
    'Arabic shaping/bidi/RTL and runtime visual rendering not established')
