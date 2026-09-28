#include "renderer_runtime.hpp"
#include "optional_overlay_boundary.hpp"
#include "../diagnostics/diagnostic_runtime.hpp"
#include "../diagnostics/startup_journal.hpp"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>
#include "input_state.hpp"
#include "game_language_reader.hpp"
#include "localization_font.hpp"
#include "selector_tooltip_state.hpp"
#include "setting_tooltip_content.hpp"
#include "selector_documentation_view.hpp"
#include "tooltip_row_layout.hpp"
#include "camera_state_view.hpp"
#include "overlay_layout_metrics.hpp"
#include "placement_config.hpp"
#ifdef OVERLAY_SETTINGS_FRONTEND
#include "hotkey_binding_presentation.hpp"
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
#include "font_playground_experiment.hpp"
#endif
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
#include "localization_font_rasterization_experiment.hpp"
#endif

extern "C" void PublishOverlayPopupState(bool open) noexcept;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam);

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <chrono>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <initializer_list>
#ifdef OVERLAY_SETTINGS_FRONTEND
#include "../plugin/runtime.hpp"
#include "../config/feature_config.hpp"
#include "camera_integration.hpp"
#include "feature_presentation.hpp"
#endif
#include <string>
#include <string_view>

namespace overlay
{
    namespace
    {
        constexpr std::string_view kInlineHotkeyToken = "__OVERLAY_HOTKEY_BADGE__";

        bool CloseActiveComboForEscapeRequest()
        {
            if (!GetInputState().ConsumeEscapePopupDismissRequest())
                return false;
            ImGui::CloseCurrentPopup();
            return true;
        }

        void DrawInlineHotkeyMessage(std::string_view message,
            std::string_view keyToken, std::string_view keyLabel)
        {
            const auto keyPosition = message.find(keyToken);
            if (keyPosition == std::string_view::npos) {
                ImGui::TextWrapped("%.*s", static_cast<int>(message.size()), message.data());
                return;
            }

            const std::string beforeKey(message.substr(0, keyPosition));
            const std::string afterKey(message.substr(keyPosition + keyToken.size()));
            if (!beforeKey.empty()) {
                ImGui::TextWrapped("%s", beforeKey.c_str());
                ImGui::SameLine(0.0f, 4.0f);
            }
            const auto punctuationBytes = inline_hotkey_layout::
                LeadingPunctuationBytes(afterKey);
            const std::string keyText = std::string(keyLabel) +
                afterKey.substr(0, punctuationBytes);
            const auto tail = inline_hotkey_layout::TrimLeadingWhitespace(
                std::string_view(afterKey).substr(punctuationBytes));
            const float keyGroupWidth = ImGui::CalcTextSize(keyText.c_str()).x;
            if (!beforeKey.empty() &&
                keyGroupWidth > ImGui::GetContentRegionAvail().x)
                ImGui::NewLine();
            ImGui::TextColored(ImVec4(0.18f, 0.78f, 1.0f, 1.0f), "%s",
                keyText.c_str());
            if (!tail.empty()) {
                ImGui::SameLine(0.0f, 4.0f);
                ImGui::TextWrapped("%.*s", static_cast<int>(tail.size()), tail.data());
            }
        }

        void DrawNotificationHeader(std::string_view title, float alpha)
        {
            const ImVec2 windowPos = ImGui::GetWindowPos();
            const float headerHeight = ImGui::GetTextLineHeight() +
                ImGui::GetStyle().FramePadding.y * 2.0f;
            ImGui::GetWindowDrawList()->AddRectFilled(windowPos,
                ImVec2(windowPos.x + ImGui::GetWindowWidth(),
                    windowPos.y + headerHeight),
                ImGui::GetColorU32(ImGuiCol_TitleBgActive, alpha));
            ImGui::SetCursorScreenPos(ImVec2(windowPos.x +
                ImGui::GetStyle().FramePadding.x, windowPos.y +
                ImGui::GetStyle().FramePadding.y));
            ImGui::PushStyleColor(ImGuiCol_Text,
                ImVec4(0.96f, 0.98f, 1.0f, alpha));
            ImGui::TextUnformatted(title.data(), title.data() + title.size());
            ImGui::PopStyleColor();
            ImGui::SetCursorScreenPos(ImVec2(windowPos.x +
                ImGui::GetStyle().WindowPadding.x, windowPos.y + headerHeight +
                ImGui::GetStyle().FramePadding.y));
        }

        void OverlayModulePathAnchor() noexcept
        {
        }

        std::filesystem::path OverlayModulePath()
        {
            HMODULE module{};
            if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&OverlayModulePathAnchor), &module))
                return {};

            wchar_t modulePath[MAX_PATH]{};
            const DWORD length = GetModuleFileNameW(module, modulePath, MAX_PATH);
            if (length == 0 || length >= MAX_PATH) return {};
            return std::filesystem::path(modulePath);
        }

        HMODULE OverlayResourceModule()
        {
            HMODULE module{};
            return GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&OverlayModulePathAnchor), &module) ? module : nullptr;
        }

        ImVec2 ClampOverlayWindowPosition(const ImVec2& position,
            const ImVec2& windowSize, const ImGuiViewport* viewport,
            float dpiScale) noexcept
        {
            if (!viewport) return position;
            const auto clamped = layout_metrics::ClampPosition(
                {position.x, position.y}, {viewport->Pos.x, viewport->Pos.y},
                {viewport->Size.x, viewport->Size.y},
                {windowSize.x, windowSize.y}, dpiScale);
            return ImVec2(clamped.x, clamped.y);
        }

#ifdef OVERLAY_SETTINGS_FRONTEND
        const std::string Tr(const LocalizationManager& manager, loc::Key key)
        { return manager.Text(key); }

        const std::string Tr(const LocalizationManager& manager, loc::Key key,
            const TextArguments& arguments)
        { return manager.Format(key, arguments); }

        float IntrinsicOptionWidth(std::string_view option)
        {
            const ImGuiStyle& style = ImGui::GetStyle();
            return ImGui::CalcTextSize(option.data(),
                option.data() + option.size()).x +
                style.FramePadding.x * 2.0f + ImGui::GetFrameHeight();
        }

        float IntrinsicComboWidth(const char* const* items, int count)
        {
            float width = 0.0f;
            for (int index = 0; index < count; ++index)
                width = (std::max)(width, IntrinsicOptionWidth(items[index]));
            return width;
        }

        template <typename Value>
        float IntrinsicComboWidth(std::span<const setting_tooltip_content::SelectorOptionBinding<Value>> options)
        {
            float width = 0.0f;
            for (const auto& option : options)
                width = (std::max)(width,
                    IntrinsicOptionWidth(option.documentation->canonicalName));
            return width;
        }

        template <typename Value>
        struct SelectorComboResult
        {
            bool valueChanged{};
            Value value{};
            SelectorInteractionState interaction{};
        };

        template <typename Value>
        SelectorComboResult<Value> DrawSelectorCombo(const char* id, Value selectedValue,
            std::span<const setting_tooltip_content::SelectorOptionBinding<Value>> options)
        {
            int selectedIndex = -1;
            for (std::size_t index = 0; index < options.size(); ++index) {
                if (options[index].value == selectedValue) {
                    selectedIndex = static_cast<int>(index);
                    break;
                }
            }
            const char* preview = selectedIndex >= 0
                ? options[static_cast<std::size_t>(selectedIndex)].documentation->canonicalName.data()
                : nullptr;
            // Capture the Combo item's interaction immediately: BeginCombo may
            // open its popup and later option/group rendering replaces ImGui's
            // LastItemData. BeginCombo's result here means its popup body was
            // successfully begun in this rendering path, not a generic popup query.
            const bool popupOpen = ImGui::BeginCombo(id, preview);
            const SelectorInteractionState interaction{
                ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip),
                ImGui::IsItemActive(),
                popupOpen
            };
            if (!popupOpen)
                return {false, selectedValue, interaction};

            // Keyboard navigation stays disabled globally, so ImGui's default
            // NavUpdateCancelRequest cannot dismiss Combo popups with Escape.
            // Consume the window-procedure request only inside the active popup.
            bool valueChanged = false;
            if (!CloseActiveComboForEscapeRequest()) {
                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(options.size()));
                if (selectedIndex >= 0)
                    clipper.IncludeItemByIndex(selectedIndex);
                while (clipper.Step()) {
                    for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index) {
                        const auto& option = options[static_cast<std::size_t>(index)];
                        ImGui::PushID(option.documentation->canonicalName.data());
                        const bool selected = option.value == selectedValue;
                        if (ImGui::Selectable(
                                option.documentation->canonicalName.data(), selected) && !selected) {
                            valueChanged = true;
                            selectedValue = option.value;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndCombo();
            return {valueChanged, selectedValue, interaction};
        }

        ImVec4 FeatureStatusColor(FeatureStatusTone tone)
        {
            switch (tone) {
            case FeatureStatusTone::Positive: return ImVec4(0.35f, 0.82f, 0.45f, 1.0f);
            case FeatureStatusTone::Warning: return ImVec4(0.95f, 0.72f, 0.25f, 1.0f);
            case FeatureStatusTone::Error: return ImVec4(0.95f, 0.32f, 0.30f, 1.0f);
            case FeatureStatusTone::Neutral: return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
            }
            return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
        }

        void DrawParameterLabel(const LocalizationManager& i18n, loc::Key label)
        {
            const auto text = Tr(i18n, label);
            ImGui::TextUnformatted(text.c_str());
        }

        struct TooltipOption
        {
            std::string_view fixedName;
            loc::Key nameKey;
            loc::Key descriptionKey;
        };

        void DrawFeatureStatus(const LocalizationManager& i18n,
            const FeaturePresentation& status)
        {
            const auto label = Tr(i18n, status.label);
            ImGui::TextColored(FeatureStatusColor(status.tone), "%s", label.c_str());
            if (!status.detail.path.empty()) {
                const auto detail = Tr(i18n, status.detail);
                ImGui::TextWrapped("%s", detail.c_str());
            }
        }

        void DrawActiveStatus(const LocalizationManager& i18n, FeatureStatusTone tone,
            loc::Key detailKey, const std::string& activeValue)
        {
            const auto value = Tr(i18n, loc::transition::ActiveValue,
                {{"value", activeValue}});
            ImGui::TextColored(FeatureStatusColor(tone), "%s", value.c_str());
            const auto detail = Tr(i18n, detailKey);
            ImGui::TextWrapped("%s", detail.c_str());
        }

        template <std::size_t Count>
        void ShowOptionListTooltip(const LocalizationManager& i18n,
            const TooltipOption (&options)[Count])
        {
            if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) || !ImGui::BeginTooltip())
                return;

            tooltip_row_layout::DrawAlignedTooltipRows(Count,
                [&i18n, &options](std::size_t index) {
                    const auto& option = options[index];
                    return option.nameKey.path.empty()
                        ? std::string(option.fixedName) : Tr(i18n, option.nameKey);
                },
                [&i18n, &options](std::size_t index, float) {
                    const auto description = Tr(i18n, options[index].descriptionKey);
                    ImGui::TextWrapped("%s", description.c_str());
                },
                [](std::size_t) {});
            ImGui::EndTooltip();
        }

        template <std::size_t Count>
        void DrawRuntimeInfoRow(const LocalizationManager& i18n, loc::Key label, const char* value,
            const TooltipOption (&options)[Count])
        {
            ImGui::BeginGroup();
            const auto labelText = Tr(i18n, label);
            const float rowWidth = ImGui::GetContentRegionAvail().x;
            const float labelWidth = ImGui::CalcTextSize(labelText.c_str()).x;
            const float valueWidth = ImGui::CalcTextSize(value).x;
            if (labelWidth + ImGui::GetStyle().ItemSpacing.x + valueWidth <= rowWidth) {
                ImGui::TextUnformatted(labelText.c_str());
                ImGui::SameLine();
                ImGui::TextUnformatted(value);
                ImGui::SameLine(0.0f, 0.0f);
                const float remainingWidth = ImGui::GetContentRegionAvail().x;
                if (remainingWidth > 1.0f)
                    ImGui::Dummy(ImVec2(remainingWidth, ImGui::GetTextLineHeight()));
            } else {
                ImGui::TextUnformatted(labelText.c_str());
                ImGui::TextWrapped("%s", value);
            }
            ImGui::EndGroup();
            ShowOptionListTooltip(i18n, options);
        }

        void ShowGameplayEnabledTooltip(const LocalizationManager& i18n)
        {
            static constexpr TooltipOption options[] = {
                {{}, loc::common::Enabled, loc::tooltip::gameplay::Enabled},
                {{}, loc::common::Disabled, loc::tooltip::gameplay::Disabled},
            };
            ShowOptionListTooltip(i18n, options);
        }

        void ShowHotkeysEnabledTooltip(const LocalizationManager& i18n)
        {
            static constexpr TooltipOption options[] = {
                {{}, loc::common::Enabled, loc::tooltip::hotkeys::Enabled},
                {{}, loc::common::Disabled, loc::tooltip::hotkeys::Disabled},
            };
            ShowOptionListTooltip(i18n, options);
        }

        void ShowHotkeyBindingTooltipContent(const LocalizationManager& i18n,
            loc::Key semantics)
        {
            const TooltipOption options[] = {
                {{}, {}, semantics},
                {{}, {}, loc::tooltip::hotkeys::Change},
                {{}, {}, loc::tooltip::hotkeys::Rebind},
                {{}, {}, loc::tooltip::hotkeys::SingleKey},
            };
            ShowOptionListTooltip(i18n, options);
        }

        void ShowGameplayHotkeyTooltip(const LocalizationManager& i18n)
        { ShowHotkeyBindingTooltipContent(i18n, loc::tooltip::hotkeys::GameplayCycle); }
        void ShowCinematicAspectHotkeyTooltip(const LocalizationManager& i18n)
        { ShowHotkeyBindingTooltipContent(i18n, loc::tooltip::hotkeys::CinematicAspectCycle); }
        void ShowCinematicFovHotkeyTooltip(const LocalizationManager& i18n)
        { ShowHotkeyBindingTooltipContent(i18n, loc::tooltip::hotkeys::CinematicFovCycle); }
        void ShowDialogueZoomHotkeyTooltip(const LocalizationManager& i18n)
        { ShowHotkeyBindingTooltipContent(i18n, loc::tooltip::hotkeys::DialogueCycle); }
        void ShowOverlayToggleHotkeyTooltip(const LocalizationManager& i18n)
        {
            static constexpr TooltipOption options[] = {
                {{}, {}, loc::tooltip::overlay::AlwaysActive},
                {{}, {}, loc::tooltip::hotkeys::ChangeOverlay},
                {{}, {}, loc::tooltip::hotkeys::Rebind},
                {{}, {}, loc::tooltip::hotkeys::SingleKey},
            };
            ShowOptionListTooltip(i18n, options);
        }

        bool IsCapturingAction(HotkeyAction action)
        {
            return GetInputState().IsCapturing(action);
        }

        void DrawHotkeyBinding(const LocalizationManager& i18n, loc::Key label,
            HotkeyAction action, int key)
        {
            ImGui::BeginGroup();
            const auto localizedLabel = Tr(i18n, label);
            const std::string captureLabel = Tr(i18n, loc::rebind::PressKey);
            const char* buttonLabel = IsCapturingAction(action)
                ? captureLabel.c_str() : config::HotkeyName(key);
            const float buttonWidth = ImGui::CalcTextSize(buttonLabel).x +
                ImGui::GetStyle().FramePadding.x * 2.0f;
            std::string stableButtonLabel{buttonLabel};
            stableButtonLabel += "##hotkey_binding";
            ImGui::TextUnformatted(localizedLabel.c_str());
            ImGui::SameLine();
            ImGui::PushID(static_cast<int>(action));
            const bool clicked = ImGui::Button(stableButtonLabel.c_str(),
                ImVec2(buttonWidth, 0.0f));
            ImGui::PopID();
            if (clicked) {
                GetInputState().BeginRebind(action, key);
                plugin::SetHotkeyRebindCaptureActive(true);
            }
            ImGui::EndGroup();
            switch (action) {
            case HotkeyAction::GameplayMode: ShowGameplayHotkeyTooltip(i18n); break;
            case HotkeyAction::CinematicAspect: ShowCinematicAspectHotkeyTooltip(i18n); break;
            case HotkeyAction::CinematicFov: ShowCinematicFovHotkeyTooltip(i18n); break;
            case HotkeyAction::DialogueZoom: ShowDialogueZoomHotkeyTooltip(i18n); break;
            case HotkeyAction::OverlayToggle: ShowOverlayToggleHotkeyTooltip(i18n); break;
            }
        }

        FeatureStatusTone CameraTransitionTone(
            OverallIntegrationAssessment assessment)
        {
            switch (assessment) {
            case OverallIntegrationAssessment::Waiting:
                return FeatureStatusTone::Warning;
            case OverallIntegrationAssessment::ConfigurationAligned:
                return FeatureStatusTone::Positive;
            case OverallIntegrationAssessment::NotAligned:
                return FeatureStatusTone::Warning;
            case OverallIntegrationAssessment::CannotAssess:
                return FeatureStatusTone::Neutral;
            }
            return FeatureStatusTone::Neutral;
        }

        loc::Key CameraTransitionDetail(
            OverallIntegrationAssessment assessment)
        {
            switch (assessment) {
            case OverallIntegrationAssessment::Waiting:
                return loc::transition::DetailWaiting;
            case OverallIntegrationAssessment::ConfigurationAligned:
                return loc::transition::DetailAligned;
            case OverallIntegrationAssessment::NotAligned:
                return loc::transition::DetailNotAligned;
            case OverallIntegrationAssessment::CannotAssess:
                return loc::transition::DetailCannotAssess;
            }
            return loc::transition::DetailCannotAssess;
        }

        std::string ActiveDialoguePolicyLabel(config::DialogueZoomPolicy policy)
        {
            return config::DialogueZoomPolicyName(policy);
        }

        loc::Key AspectAssessmentLabel(AspectAssessment assessment)
        {
            switch (assessment) {
            case AspectAssessment::Matched: return loc::transition::Aligned;
            case AspectAssessment::Mismatch: return loc::transition::NotAligned;
            case AspectAssessment::CannotAssess: return loc::common::CannotAssess;
            }
            return loc::common::CannotAssess;
        }

        loc::Key OverallIntegrationLabel(OverallIntegrationAssessment assessment)
        {
            switch (assessment) {
            case OverallIntegrationAssessment::Waiting:
                return loc::transition::Waiting;
            case OverallIntegrationAssessment::ConfigurationAligned:
                return loc::transition::Aligned;
            case OverallIntegrationAssessment::NotAligned: return loc::transition::NotAligned;
            case OverallIntegrationAssessment::CannotAssess: return loc::common::CannotAssess;
            }
            return loc::common::CannotAssess;
        }

        bool ApplySetting(const plugin::RuntimeSettingMutation& mutation,
            const Renderer::LogFunction& logger)
        {
            auto& api = plugin::GetRuntimeSettingsApi();
            const auto result = api.Apply(mutation);
            if (!result.accepted) {
                if (logger) logger("OVERLAY_SETTING_REJECTED");
                return false;
            }
            if (!api.Persist(mutation) && logger)
                logger("OVERLAY_SETTING_PERSIST_FAILED");
            return true;
        }

        loc::Key NotificationActionLabel(plugin::OverlayNotificationAction action)
        {
            switch (action) {
            case plugin::OverlayNotificationAction::GameplayMode: return loc::setting::GameplayMode;
            case plugin::OverlayNotificationAction::CinematicAspect: return loc::setting::CinematicAspect;
            case plugin::OverlayNotificationAction::CinematicFov: return loc::setting::CinematicFov;
            case plugin::OverlayNotificationAction::DialogueZoom: return loc::setting::DialogueZoom;
            case plugin::OverlayNotificationAction::OverlayToggle: return loc::setting::OverlayToggle;
            case plugin::OverlayNotificationAction::AnotherSetting: return loc::setting::Mode;
            }
            return loc::setting::Mode;
        }
#endif
    }

    void Renderer::Log(const std::string& message) const
    {
        if (logger_) logger_(message);
    }

    namespace
    {
        void LogDiagnostic(const Renderer::LogFunction& logger,
            const std::string& message)
        {
            if (logger && diagnostics::Enabled()) logger(message);
        }
    }

    bool Renderer::Initialize(HWND window, UINT width, UINT height, UINT dpi) noexcept
    {
        bool initialized = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            initialized = InitializeImpl(window, width, height, dpi);
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && initialized;
    }

    bool Renderer::RebindWindow(HWND window, UINT width, UINT height,
        UINT dpi) noexcept
    {
        bool rebound = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            if (!window || !IsWindow(window) || !width || !height ||
                disabled() || !state_.cameraCoreReady())
                return;
            SurfaceUpdateToken token{};
            if (!state_.BeginSurfaceUpdate({width, height}, token)) return;
            if (startupNotification_.pending())
                startupAutoLanguageSyncRequested_ = false;
            DetachImGui();
            ResetComposition();
            window_ = window;
            if (!ApplyWindowDpi(dpi)) {
                Disable("window_rebind_dpi_setup_failed");
                return;
            }
            if (!CreateGraphicsDevice() || !CreateCompositionTarget(window_) ||
                !BuildImGui() || !CreateSurface(width, height, surface_) ||
                FAILED(visual_->SetContent(surface_.Get())) ||
                FAILED(compositionTarget_->SetRoot(visual_.Get())) ||
                !DrawImGuiSurface(surface_.Get(), width, height, nullptr,
                    {0, 0, static_cast<int>(width), static_cast<int>(height)}) ||
                !state_.PublishSurfaceUpdate(token, true, true)) {
                Disable("window_rebind_failed");
                return;
            }
            surfaceWidth_ = width;
            surfaceHeight_ = height;
            surfaceGeneration_ = state_.generation();
            compositionReady_ = true;
            surfaceDirty_ = true;
            forceFullDamage_ = true;
            damageTracker_.Invalidate();
            firstFrameLogged_ = false;
            Log("OVERLAY_DCOMP_WINDOW_REBOUND generation=" +
                std::to_string(surfaceGeneration_));
            rebound = RenderImpl(surface_.Get(), width, height);
            if (rebound) StartPendingNotificationLifetimes();
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && rebound;
    }

    bool Renderer::InitializeImpl(HWND window, UINT width, UINT height, UINT dpi)
    {
        state_.MarkCameraCoreReady();
        if (!window || !IsWindow(window) || !width || !height ||
            !state_.MarkWindowAvailable({width, height}))
            return false;
        window_ = window;
        if (!ApplyWindowDpi(dpi)) {
            Disable("window_dpi_setup_failed");
            return false;
        }
        Log("OVERLAY_DCOMP_INIT_BEGIN");
        if (!CreateGraphicsDevice() || !CreateCompositionTarget(window_) ||
            !BuildImGui()) {
            Disable("composition_initialization_failed");
            return false;
        }
        if (!CreateSurface(width, height, surface_) ||
            FAILED(visual_->SetContent(surface_.Get())) ||
            FAILED(compositionTarget_->SetRoot(visual_.Get()))) {
            Disable("composition_surface_create_failed");
            return false;
        }
        if (!DrawImGuiSurface(surface_.Get(), width, height, nullptr,
                {0, 0, static_cast<int>(width), static_cast<int>(height)})) {
            Disable("composition_initial_commit_failed");
            return false;
        }
        SurfaceUpdateToken token{};
        if (!state_.BeginSurfaceUpdate({width, height}, token) ||
            !state_.PublishSurfaceUpdate(token, true, true)) {
            Disable("composition_initial_generation_failed");
            return false;
        }
        surfaceWidth_ = width;
        surfaceHeight_ = height;
        surfaceGeneration_ = state_.generation();
        compositionReady_ = true;
        surfaceDirty_ = true;
        forceFullDamage_ = true;
        damageTracker_.Invalidate();
        PrepareStartupNotification();
        Log("OVERLAY_DCOMP_SURFACE_READY generation=" +
            std::to_string(surfaceGeneration_) + " width=" +
            std::to_string(width) + " height=" + std::to_string(height));
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(6, "COMPOSITION_SURFACE_READY",
            "full_draw_commit", surfaceGeneration_);
        diagnostics::startup_journal::MarkOnce(7, "RENDERER_ACTIVATED",
            "directcomposition_surface_committed", surfaceGeneration_);
        diagnostics::startup_journal::Flush();
#endif
        const bool rendered = RenderImpl(surface_.Get(), width, height);
        if (rendered) StartPendingNotificationLifetimes();
        return rendered;
    }

    bool Renderer::CreateGraphicsDevice()
    {
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_FEATURE_LEVEL selected{};
        constexpr D3D_FEATURE_LEVEL levels[]{D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0};
        HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
            nullptr, flags, levels, static_cast<UINT>(std::size(levels)),
            D3D11_SDK_VERSION, device_.GetAddressOf(), &selected, nullptr);
        if (result == E_INVALIDARG) {
            result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
                nullptr, flags, &levels[1], 1, D3D11_SDK_VERSION,
                device_.GetAddressOf(), &selected, nullptr);
        }
        if (FAILED(result) || FAILED(device_.As(&dxgiDevice_))) return false;
        result = DCompositionCreateDevice2(dxgiDevice_.Get(),
            __uuidof(IDCompositionDesktopDevice),
            reinterpret_cast<void**>(compositionDevice_.GetAddressOf()));
        if (FAILED(result)) return false;
        Log("OVERLAY_DCOMP_DEVICE_READY featureLevel=" +
            std::to_string(static_cast<unsigned>(selected)));
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(5, "COMPOSITION_DEVICE_READY",
            "private_d3d11_device");
        diagnostics::startup_journal::Flush();
#endif
        return true;
    }

    bool Renderer::CreateCompositionTarget(HWND window)
    {
        if (!compositionDevice_ || !window || !IsWindow(window)) return false;
        HRESULT result = compositionDevice_->CreateTargetForHwnd(window, TRUE,
            compositionTarget_.GetAddressOf());
        if (FAILED(result)) {
            Log("OVERLAY_DCOMP_TOPMOST_TARGET_UNAVAILABLE; using non-topmost target");
            result = compositionDevice_->CreateTargetForHwnd(window, FALSE,
                compositionTarget_.GetAddressOf());
        }
        return SUCCEEDED(result) && SUCCEEDED(compositionDevice_->CreateVisual(
            visual_.GetAddressOf()));
    }

    bool Renderer::CreateSurface(UINT width, UINT height,
        Microsoft::WRL::ComPtr<IDCompositionSurface>& surface) noexcept
    {
        if (!compositionDevice_ || !width || !height ||
            width > 16384 || height > 16384)
            return false;
        return SUCCEEDED(compositionDevice_->CreateSurface(width, height,
            DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED,
            surface.GetAddressOf()));
    }

    bool Renderer::ApplyWindowDpi(UINT dpi) noexcept
    {
        const UINT normalizedDpi = dpi ? dpi : 96u;
        if (normalizedDpi == windowDpi_) return true;
        windowDpi_ = normalizedDpi;
        dpiScale_ = static_cast<float>(windowDpi_) / 96.0f;
        if (ImGui::GetCurrentContext() && baseStyleCaptured_) {
            ImGui::GetStyle() = baseStyle_;
            ImGui::GetStyle().ScaleAllSizes(dpiScale_);
        }
        forceFullDamage_ = true;
        surfaceDirty_ = true;
        if (imguiReady_) {
            plugin::RuntimeSettingsSnapshot settings{};
            if (!plugin::GetRuntimeSettingsApi().Snapshot(settings)) return false;
            const auto* descriptor = localization::FindLocaleDescriptor(
                settings.overlayLocaleCode);
            const std::string_view profile = descriptor
                ? descriptor->fontProfileCode : std::string_view{"base"};
            if (!RebuildFontAtlas(settings.overlayFontSize, profile)) return false;
        }
        Log("OVERLAY_DPI_CHANGED dpi=" + std::to_string(windowDpi_) +
            " scale=" + std::to_string(dpiScale_) +
            " coordinate_contract=physical_client_pixels");
        return true;
    }

    bool Renderer::RecoverGraphicsDevice(UINT width, UINT height) noexcept
    {
        try {
            if (!window_ || !IsWindow(window_) || !width || !height ||
                !ImGui::GetCurrentContext())
                return false;
            auto& input = GetInputState();
            auto& inputBridge = GetInputEventBridge();
            const auto inputOwnershipBefore = CaptureInputOwnership(input,
                inputBridge);
            if (!inputOwnershipBefore.Coherent()) return false;
            imguiRenderer_.Shutdown(ImGui::GetIO().Fonts);
            ResetComposition();
            if (!CreateGraphicsDevice() || !CreateCompositionTarget(window_) ||
                !imguiRenderer_.Initialize(device_.Get(), ImGui::GetIO().Fonts) ||
                !CreateSurface(width, height, surface_) ||
                FAILED(visual_->SetContent(surface_.Get())) ||
                FAILED(compositionTarget_->SetRoot(visual_.Get())) ||
                !DrawImGuiSurface(surface_.Get(), width, height, nullptr,
                    {0, 0, static_cast<int>(width), static_cast<int>(height)}))
                return false;
            if (!InputOwnershipUnchanged(inputOwnershipBefore, input,
                    inputBridge) ||
                !state_.CompleteDeviceRecovery(true, {width, height}))
                return false;
            surfaceWidth_ = width;
            surfaceHeight_ = height;
            surfaceGeneration_ = state_.generation();
            compositionReady_ = true;
            surfaceDirty_ = true;
            forceFullDamage_ = true;
            damageTracker_.Invalidate();
            Log("OVERLAY_DCOMP_DEVICE_RECOVERED generation=" +
                std::to_string(surfaceGeneration_));
            return true;
        } catch (...) {
            state_.CompleteDeviceRecovery(false, {});
            return false;
        }
    }

    bool Renderer::BuildImGui()
    {
        if (!device_ || !window_) return false;
        const auto resourceModule = OverlayResourceModule();
#ifdef OVERLAY_SETTINGS_FRONTEND
        std::string localeError;
        if (!localization_.Initialize(localization::CanonicalLocaleCode,
            resourceModule, localeError)) {
            Log("OVERLAY_CATALOGS_UNAVAILABLE: " + localeError);
            return false;
        }
#endif
        if (!ImGui::GetCurrentContext()) {
            ImGui::CreateContext();
            imguiContextCreated_ = true;
            ImGuiIO& io = ImGui::GetIO();
            int fontSizeSetting = config::OverlayFontSizeDefault;
#ifdef OVERLAY_SETTINGS_FRONTEND
            plugin::RuntimeSettingsSnapshot settings{};
            if (plugin::GetRuntimeSettingsApi().Snapshot(settings))
                fontSizeSetting = settings.overlayFontSize;
#endif
            const int fontSizePixels = presentation::EffectiveFontPixels(
                fontSizeSetting, windowDpi_);
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            io.FontDefault = font_playground_experiment::AddSelectedFont(fontSizePixels);
#elif defined(OVERLAY_FONT_RASTERIZATION_EXPERIMENT)
            io.FontDefault = AddProggyVectorOnly(fontSizePixels);
#else
            std::string_view profileCode{"base"};
#ifdef OVERLAY_SETTINGS_FRONTEND
            if (plugin::GetRuntimeSettingsApi().Snapshot(settings)) {
                if (const auto* descriptor = localization::FindLocaleDescriptor(
                        settings.overlayLocaleCode))
                    profileCode = descriptor->fontProfileCode;
                localization_.SetLocale(settings.overlayLocaleCode);
                activeLocaleCode_ = settings.overlayLocaleCode;
            }
#endif
            io.FontDefault = AddLocalizationFont(fontSizePixels, profileCode);
            if (io.FontDefault && !AddLocalizationSelectorFonts(fontSizePixels))
                io.FontDefault = nullptr;
            activeFontProfileCode_ = profileCode;
#endif
            if (!io.FontDefault) {
                Log("OVERLAY_UI_FONT_UNAVAILABLE");
                return false;
            }
            if (!io.Fonts->Build()) return false;
            activeFontSizePixels_ = fontSizePixels;
            activeFontSetting_ = fontSizeSetting;
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
            activeFontRasterizationMode_ = font_rasterization_experiment::CurrentMode();
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            activeFontChoice_ = font_playground_experiment::CurrentFontChoice();
#endif
        }
        if (!overlayPlacementLoaded_) {
            bool migrated = false;
            auto modulePath = OverlayModulePath();
            if (modulePath.empty()) {
                Log("OVERLAY_POSITION_LOAD_FAILED");
                overlayPlacementLoaded_ = true;
            } else {
                overlayConfigPath_ = modulePath.replace_filename(
                    L"STALKER2CameraTweaks.ini").wstring();
                const auto legacyPath = modulePath.replace_filename(
                    L"STALKER2CameraTweaksOverlay.ini").wstring();
                if (!placement_config::Load(overlayConfigPath_, legacyPath,
                        overlayPositionX_, overlayPositionY_, migrated))
                    Log("OVERLAY_POSITION_LOAD_FAILED");
                else if (migrated)
                    Log("OVERLAY_POSITION_MIGRATED_FROM_LEGACY_INI");
                overlayPlacementLoaded_ = true;
            }
        }
        ImGui::StyleColorsDark();
        baseStyle_ = ImGui::GetStyle();
        baseStyleCaptured_ = true;
        ImGui::GetStyle().ScaleAllSizes(dpiScale_);
        if (!imguiReady_) {
            if (!ImGui_ImplWin32_Init(window_)) return false;
            if (!imguiRenderer_.Initialize(device_.Get(), ImGui::GetIO().Fonts)) {
                ImGui_ImplWin32_Shutdown();
                return false;
            }
            imguiReady_ = true;
        }
        return true;
    }

    bool Renderer::RebuildFontAtlas(int fontSizeSetting,
        std::string_view fontProfileCode)
    {
        if (!imguiReady_ || !ImGui::GetCurrentContext() ||
            fontSizeSetting < config::OverlayFontSizeMin ||
            fontSizeSetting > config::OverlayFontSizeMax)
            return false;
        const int fontSizePixels = presentation::EffectiveFontPixels(
            fontSizeSetting, windowDpi_);
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
        const int requestedRasterizationMode =
            font_rasterization_experiment::CurrentMode();
        const bool rasterizationChanged =
            requestedRasterizationMode != activeFontRasterizationMode_;
#else
        const bool rasterizationChanged = false;
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
        const int requestedFontChoice =
            font_playground_experiment::CurrentFontChoice();
        const bool fontChoiceChanged = requestedFontChoice != activeFontChoice_;
#else
        const bool fontChoiceChanged = false;
#endif
        const bool fontProfileChanged = fontProfileCode != activeFontProfileCode_;
        if (fontSizePixels == activeFontSizePixels_ &&
            fontProfileCode == activeFontProfileCode_ && !rasterizationChanged &&
            !fontChoiceChanged) {
            activeFontSetting_ = fontSizeSetting;
            return true;
        }
        imguiRenderer_.InvalidateFontTexture(ImGui::GetIO().Fonts);
        ImFontAtlas* atlas = ImGui::GetIO().Fonts;
        atlas->Clear();
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
        ImGui::GetIO().FontDefault =
            font_playground_experiment::AddSelectedFont(fontSizePixels);
#elif defined(OVERLAY_FONT_RASTERIZATION_EXPERIMENT)
        ImGui::GetIO().FontDefault = AddProggyVectorOnly(fontSizePixels);
#else
        ImGui::GetIO().FontDefault = AddLocalizationFont(fontSizePixels,
            fontProfileCode);
        if (ImGui::GetIO().FontDefault &&
            !AddLocalizationSelectorFonts(fontSizePixels))
            ImGui::GetIO().FontDefault = nullptr;
#endif
        bool rebuilt = ImGui::GetIO().FontDefault && atlas->Build() &&
            imguiRenderer_.CreateFontTexture(atlas);
        if (rebuilt) {
            activeFontSizePixels_ = fontSizePixels;
            activeFontSetting_ = fontSizeSetting;
            activeFontProfileCode_ = fontProfileCode;
            forceFullDamage_ = true;
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
            activeFontRasterizationMode_ = requestedRasterizationMode;
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            activeFontChoice_ = requestedFontChoice;
#endif
            Log("OVERLAY_FONT_ATLAS_REBUILT size=" + std::to_string(fontSizePixels) +
                " profile=" + std::string(activeFontProfileCode_));
            return true;
        }

        Log("OVERLAY_FONT_ATLAS_REBUILD_FAILED size=" + std::to_string(fontSizePixels));
        imguiRenderer_.InvalidateFontTexture(atlas);
        atlas->Clear();
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
        font_playground_experiment::SetCurrentFontChoice(activeFontChoice_);
        ImGui::GetIO().FontDefault =
            font_playground_experiment::AddSelectedFont(activeFontSizePixels_);
#elif defined(OVERLAY_FONT_RASTERIZATION_EXPERIMENT)
        font_rasterization_experiment::SetCurrentMode(activeFontRasterizationMode_);
        ImGui::GetIO().FontDefault = AddProggyVectorOnly(activeFontSizePixels_);
#else
        ImGui::GetIO().FontDefault = AddLocalizationFont(activeFontSizePixels_,
            activeFontProfileCode_);
        if (ImGui::GetIO().FontDefault &&
            !AddLocalizationSelectorFonts(activeFontSizePixels_))
            ImGui::GetIO().FontDefault = nullptr;
#endif
        const bool restored = ImGui::GetIO().FontDefault && atlas->Build() &&
            imguiRenderer_.CreateFontTexture(atlas);
        if (!restored) {
            Log("OVERLAY_FONT_ATLAS_RESTORE_FAILED");
            return false;
        }

        if (fontProfileChanged) {
            Log("OVERLAY_FONT_PROFILE_CHANGE_REJECTED profile=" +
                std::string(fontProfileCode));
            return false;
        }

#ifdef OVERLAY_SETTINGS_FRONTEND
        ApplySetting(plugin::RuntimeSettingMutation::FontSize(activeFontSetting_), logger_);
#endif
        Log("OVERLAY_FONT_ATLAS_RESTORED size=" + std::to_string(activeFontSizePixels_));
        return true;
    }

    bool Renderer::DrawImGuiSurface(IDCompositionSurface* targetSurface,
        UINT width, UINT height, const ImDrawData* drawData,
        presentation::Rect damage) noexcept
    {
        if (!targetSurface || !device_ || !compositionDevice_ || !width || !height)
            return false;
        const RECT fullRect{0, 0, static_cast<LONG>(width),
            static_cast<LONG>(height)};
        const RECT requested{damage.left, damage.top, damage.right, damage.bottom};
        const bool fullUpdate = requested.left == 0 && requested.top == 0 &&
            requested.right == fullRect.right && requested.bottom == fullRect.bottom;
        const auto drawUpdate = [&](const RECT& updateRect, bool updateWholeSurface) {
            Microsoft::WRL::ComPtr<IDXGISurface> updateSurface;
            POINT offset{};
            const RECT* updatePointer = updateWholeSurface ? nullptr : &updateRect;
            const HRESULT beginResult = targetSurface->BeginDraw(updatePointer,
                __uuidof(IDXGISurface),
                reinterpret_cast<void**>(updateSurface.GetAddressOf()), &offset);
            if (FAILED(beginResult)) return false;

            Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
            bool drew = false;
            if (SUCCEEDED(updateSurface.As(&texture)) &&
                SUCCEEDED(device_->CreateRenderTargetView(texture.Get(), nullptr,
                    target.GetAddressOf()))) {
                if (drawData) {
                    drew = imguiRenderer_.Render(drawData, target.Get(), width,
                        height, updateRect, offset);
                } else {
                    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
                    Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context1;
                    device_->GetImmediateContext(context.GetAddressOf());
                    if (context && SUCCEEDED(context.As(&context1))) {
                        const float transparent[]{0, 0, 0, 0};
                        const LONG updateWidth = updateRect.right - updateRect.left;
                        const LONG updateHeight = updateRect.bottom - updateRect.top;
                        const D3D11_RECT clearRect{offset.x, offset.y,
                            offset.x + updateWidth, offset.y + updateHeight};
                        context1->ClearView(target.Get(), transparent, &clearRect, 1);
                        context->OMSetRenderTargets(0, nullptr, nullptr);
                        drew = true;
                    } else if (fullUpdate && context) {
                        const float transparent[]{0, 0, 0, 0};
                        context->ClearRenderTargetView(target.Get(), transparent);
                        context->OMSetRenderTargets(0, nullptr, nullptr);
                        drew = true;
                    }
                }
            }
            target.Reset();
            texture.Reset();
            updateSurface.Reset();
            const HRESULT endResult = targetSurface->EndDraw();
            return drew && SUCCEEDED(endResult);
        };

        bool drew = drawUpdate(requested, fullUpdate);
        if (!drew && !fullUpdate) {
            Log("OVERLAY_DAMAGE_UPDATE_FALLBACK reason=partial_draw_unavailable");
            drew = drawUpdate(fullRect, true);
        }
        return drew && SUCCEEDED(compositionDevice_->Commit());
    }

    bool Renderer::OnWindowGeometry(HWND window, UINT width, UINT height,
        bool minimized, UINT dpi) noexcept
    {
        bool updated = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            if (!OwnsWindow(window)) return;
            const bool dpiChanged = dpi != windowDpi_;
            if (dpiChanged && !ApplyWindowDpi(dpi)) {
                Disable("window_dpi_update_failed");
                return;
            }
            if (minimized) {
                const bool wasMinimized = state_.phase() == PresenterPhase::Minimized;
                state_.SetMinimized(true);
                if (!wasMinimized) Log("OVERLAY_WINDOW_MINIMIZED");
                updated = true;
                return;
            }
            if (!width || !height) return;
            state_.SetMinimized(false);
            if (width == surfaceWidth_ && height == surfaceHeight_) {
                if (dpiChanged) forceFullDamage_ = true;
                surfaceDirty_ = true;
                updated = RenderImpl(surface_.Get(), width, height);
                if (updated) StartPendingNotificationLifetimes();
                return;
            }
            SurfaceUpdateToken token{};
            if (!state_.BeginSurfaceUpdate({width, height}, token)) return;
            surfaceDirty_ = true;
            forceFullDamage_ = true;
            Microsoft::WRL::ComPtr<IDCompositionSurface> replacement;
            if (!CreateSurface(width, height, replacement) ||
                !RenderImpl(replacement.Get(), width, height)) {
                Log("OVERLAY_DCOMP_RESIZE_PREPARE_FAILED width=" +
                    std::to_string(width) + " height=" + std::to_string(height));
                Disable("composition_resize_prepare_failed");
                return;
            }
            HRESULT result = visual_->SetContent(replacement.Get());
            if (SUCCEEDED(result)) result = compositionDevice_->Commit();
            if (FAILED(result)) {
                visual_->SetContent(surface_.Get());
                compositionDevice_->Commit();
                Log("OVERLAY_DCOMP_RESIZE_COMMIT_FAILED hr=" +
                    std::to_string(static_cast<long>(result)));
                Disable("composition_resize_commit_failed");
                return;
            }
            if (!PublishCommittedSurfaceResource(state_, token, surface_,
                    std::move(replacement), true, true)) {
                visual_->SetContent(surface_.Get());
                compositionDevice_->Commit();
                Log("OVERLAY_DCOMP_RESIZE_GENERATION_REJECTED");
                Disable("composition_resize_generation_rejected");
                return;
            }
            surfaceWidth_ = width;
            surfaceHeight_ = height;
            surfaceGeneration_ = state_.generation();
            surfaceDirty_ = false;
            damageTracker_.Invalidate();
            forceFullDamage_ = true;
            StartPendingNotificationLifetimes();
            Log("OVERLAY_DCOMP_SURFACE_RESIZED generation=" +
                std::to_string(surfaceGeneration_) + " width=" +
                std::to_string(width) + " height=" + std::to_string(height));
            updated = true;
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && updated;
    }

    void Renderer::ResetComposition() noexcept
    {
        if (compositionTarget_) {
            compositionTarget_->SetRoot(nullptr);
            if (compositionDevice_) compositionDevice_->Commit();
        }
        surface_.Reset();
        visual_.Reset();
        compositionTarget_.Reset();
        compositionDevice_.Reset();
        dxgiDevice_.Reset();
        device_.Reset();
        surfaceWidth_ = 0;
        surfaceHeight_ = 0;
        compositionReady_ = false;
    }

    void Renderer::DetachImGui() noexcept
    {
        if (ImGui::GetCurrentContext()) {
            auto& io = ImGui::GetIO();
            if (imguiReady_) imguiRenderer_.Shutdown(io.Fonts);
            if (io.BackendPlatformUserData) ImGui_ImplWin32_Shutdown();
            if (imguiContextCreated_) ImGui::DestroyContext();
        } else {
            imguiRenderer_.Shutdown(nullptr);
        }
        imguiReady_ = false;
        imguiContextCreated_ = false;
    }

    void Renderer::UpdateNotifications() noexcept
    {
        try {
            std::vector<plugin::OverlayNotification> incoming;
            plugin::DrainOverlayNotifications(incoming);
            for (auto& notification : incoming) {
                if (notifications_.size() >= 3)
                    notifications_.erase(notifications_.begin());
                notifications_.push_back({std::move(notification), {}});
            }
            const auto now = static_cast<std::uint64_t>(GetTickCount64());
            const auto oldSize = notifications_.size();
            notifications_.erase(std::remove_if(notifications_.begin(), notifications_.end(),
                [now](const auto& active) {
                    return active.lifetime.IsExpired(now,
                        active.notification.durationMs);
                }), notifications_.end());
            if (notifications_.size() != oldSize) surfaceDirty_ = true;
        } catch (...) {
            // Toasts are optional and must never affect rendering or settings.
        }
    }

    void Renderer::EnsureStartupNotification(
        const plugin::RuntimeSettingsSnapshot& settings) noexcept
    {
        if (!startupNotification_.pending()) return;
        try {
            plugin::OverlayNotification hint{};
            hint.kind = plugin::OverlayNotificationKind::StartupOverlayHint;
            hint.action = plugin::OverlayNotificationAction::OverlayToggle;
            hint.primaryValue = config::HotkeyName(settings.overlayToggleKey);
            hint.status = plugin::OverlayNotificationStatus::Ready;
            hint.durationMs = StartupHintDurationMs;
            hint.id = 0;
            if (notifications_.size() >= 3)
                notifications_.erase(notifications_.begin());
            notifications_.push_back({std::move(hint), {}});
            startupNotification_.MarkCreated(ready(), true);
            surfaceDirty_ = true;
            Log("OVERLAY_STARTUP_HINT_CREATED presenter_lifecycle=1");
        } catch (...) {
            Log("OVERLAY_STARTUP_HINT_CREATE_FAILED");
        }
    }

    void Renderer::PrepareStartupNotification() noexcept
    {
        if (!startupNotification_.ShouldWakePresenter(ready())) return;
        try {
            plugin::RuntimeSettingsSnapshot settings{};
            if (!plugin::GetRuntimeSettingsApi().Snapshot(settings)) return;
            if (settings.overlayLocaleAuto &&
                !settings.overlayLocaleAutoSynchronized &&
                !startupAutoLanguageSyncRequested_) {
                startupAutoLanguageSyncRequested_ =
                    RequestAutoLanguageSynchronization(window_);
                if (!startupAutoLanguageSyncRequested_ &&
                    !startupAutoLanguageSyncRequestFailureLogged_) {
                    Log("OVERLAY_STARTUP_AUTO_LANGUAGE_SYNC_REQUEST_FAILED");
                    startupAutoLanguageSyncRequestFailureLogged_ = true;
                }
                if (!startupAutoLanguageSyncRequested_) {
                    const bool fallbackApplied =
                        plugin::SetDetectedOverlayLocale("en");
                    if (!fallbackApplied) return;
                    if (!plugin::GetRuntimeSettingsApi().Snapshot(settings)) {
                        settings.overlayLocaleCode = "en";
                        settings.overlayLocaleAutoSynchronized = true;
                    }
                }
            }
            if (StartupHintMustWaitForAutoLocale(settings.overlayLocaleAuto,
                    settings.overlayLocaleAutoSynchronized,
                    startupAutoLanguageSyncRequested_)) {
                Log("OVERLAY_STARTUP_HINT_WAITING_FOR_AUTO_LOCALE");
                return;
            }
            EnsureStartupNotification(settings);
        } catch (...) {
            Log("OVERLAY_STARTUP_PREPARATION_FAILED");
        }
    }

    void Renderer::StartPendingNotificationLifetimes() noexcept
    {
        const auto committedAt = static_cast<std::uint64_t>(GetTickCount64());
        for (auto& active : notifications_) {
            if (StartNotificationLifetimeAfterCommit(active.lifetime,
                    active.notification.id,
                    std::span<const std::uint64_t>{submittedNotificationIds_.data(),
                        submittedNotificationCount_}, true, committedAt)) {
                try {
                    Log("OVERLAY_NOTIFICATION_LIFETIME_STARTED id=" +
                        std::to_string(active.notification.id));
                    if (active.notification.kind ==
                            plugin::OverlayNotificationKind::StartupOverlayHint &&
                        !startupHintFrameLogged_) {
                        startupHintFrameLogged_ = true;
                        Log("OVERLAY_STARTUP_HINT_DRAWN_AND_COMMITTED");
                    }
                } catch (...) {
                    // Notification diagnostics are optional.
                }
            }
        }
    }

    void Renderer::DrawNotifications()
    {
        const auto now = static_cast<std::uint64_t>(GetTickCount64());
        float y = 30.0f;
        for (auto& active : notifications_) {
            active.frameDrawListToken = 0;
            const auto& notification = active.notification;
            if (!active.lifetime.IsDrawable(now, notification.durationMs)) continue;
            const auto age = active.lifetime.AgeMs(now);
            const float fadeIn = static_cast<float>(age) / 220.0f < 1.0f
                ? static_cast<float>(age) / 220.0f : 1.0f;
            const auto fadeStart = notification.durationMs > 400
                ? notification.durationMs - 400 : 0;
            const float fadeOut = age >= fadeStart
                ? (1.0f - static_cast<float>(age - fadeStart) / 400.0f > 0.0f
                    ? 1.0f - static_cast<float>(age - fadeStart) / 400.0f : 0.0f)
                : 1.0f;
            const float alpha = fadeIn * fadeOut;
            if (alpha <= 0.0f) continue;

            const std::string windowId = "##overlay_notification_" +
                std::to_string(notification.id);
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, y),
                ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            ImGui::SetNextWindowSizeConstraints(ImVec2(250.0f * dpiScale_, 0.0f),
                ImVec2(440.0f * dpiScale_, FLT_MAX));
            ImGui::SetNextWindowBgAlpha(0.92f * alpha);
            ImGui::Begin(windowId.c_str(), nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_AlwaysAutoResize);
            active.frameDrawListToken = reinterpret_cast<std::uintptr_t>(
                ImGui::GetWindowDrawList());
            std::string localizedValue;
            std::string localizedTitle;
            if (notification.kind == plugin::OverlayNotificationKind::StartupOverlayHint) {
                localizedTitle = Tr(localization_, loc::app::Title);
                localizedValue = Tr(localization_, loc::notification::OpenOverlayHint,
                    {{"key", std::string(kInlineHotkeyToken)}});
            } else if (notification.kind == plugin::OverlayNotificationKind::HotkeyConflict) {
                const auto actionLabel = Tr(localization_, NotificationActionLabel(notification.action));
                localizedTitle = Tr(localization_, loc::notification::HotkeyConflict);
                localizedValue = Tr(localization_, loc::notification::ConflictValue,
                    {{"key", notification.primaryValue}, {"setting", actionLabel}});
            } else if (notification.kind == plugin::OverlayNotificationKind::BindingChanged) {
                const auto actionLabel = Tr(localization_, NotificationActionLabel(notification.action));
                localizedTitle = Tr(localization_, loc::notification::HotkeyTitle,
                    {{"setting", actionLabel}});
                localizedValue = Tr(localization_, loc::notification::BindingValue,
                    {{"oldKey", notification.primaryValue}, {"newKey", notification.secondaryValue}});
            } else {
                const auto actionLabel = Tr(localization_, NotificationActionLabel(notification.action));
                localizedTitle = actionLabel;
                localizedValue = Tr(localization_, loc::notification::SettingChanged,
                    {{"oldValue", notification.primaryValue}, {"newValue", notification.secondaryValue}});
            }
            DrawNotificationHeader(localizedTitle, alpha);
            if (notification.kind == plugin::OverlayNotificationKind::StartupOverlayHint) {
                DrawInlineHotkeyMessage(localizedValue, kInlineHotkeyToken,
                    notification.primaryValue);
            } else {
                ImGui::TextUnformatted(localizedValue.c_str());
            }
            if (notification.kind != plugin::OverlayNotificationKind::StartupOverlayHint) {
            const FeatureStatusTone statusTone = notification.status ==
                plugin::OverlayNotificationStatus::Error
                ? FeatureStatusTone::Error
                : notification.status == plugin::OverlayNotificationStatus::Pending
                    ? FeatureStatusTone::Warning : FeatureStatusTone::Positive;
            ImVec4 statusColor = FeatureStatusColor(statusTone);
            statusColor.w = alpha;
            const auto statusKey = notification.status == plugin::OverlayNotificationStatus::Applied
                ? loc::common::Applied : notification.status == plugin::OverlayNotificationStatus::Pending
                    ? loc::common::Pending : notification.status == plugin::OverlayNotificationStatus::Ready
                        ? loc::common::Ready : loc::common::Error;
            const auto localizedStatus = Tr(localization_, statusKey);
            ImGui::TextColored(statusColor, "%s", localizedStatus.c_str());
            }
            y += ImGui::GetWindowSize().y + 8.0f;
            ImGui::End();
        }
    }

    bool Renderer::Render() noexcept
    {
        bool rendered = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            if (needsRenderWork() && surface_) {
                rendered = RenderImpl(surface_.Get(), surfaceWidth_, surfaceHeight_);
                if (rendered) StartPendingNotificationLifetimes();
            }
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && rendered;
    }

    void Renderer::ApplyInputEvents(UINT width, UINT height)
    {
        std::vector<InputEvent> events;
        GetInputEventBridge().Drain(events);
        if (!ImGui::GetCurrentContext()) return;
        auto& io = ImGui::GetIO();
        for (const auto& event : events) {
            switch (event.kind) {
            case InputEventKind::Reset:
                io.AddFocusEvent(false);
                for (int button = 0; button < 5; ++button)
                    io.AddMouseButtonEvent(button, false);
                virtualCursorPosition_.Reset();
                io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
                break;
            case InputEventKind::AbsoluteMousePosition:
                virtualCursorPosition_.SetAbsolute(event.x, event.y,
                    static_cast<int>(width), static_cast<int>(height));
                if (!firstAbsoluteMouseAppliedLogged_) {
                    firstAbsoluteMouseAppliedLogged_ = true;
                    Log("OVERLAY_INPUT_ABSOLUTE_MOUSE_APPLIED x=" +
                        std::to_string(virtualCursorPosition_.point().x) + " y=" +
                        std::to_string(virtualCursorPosition_.point().y));
                }
                break;
            case InputEventKind::RelativeMouseMotion:
                virtualCursorPosition_.ApplyRelative(event.x, event.y,
                    static_cast<int>(width), static_cast<int>(height));
                if (!firstRawMouseAppliedLogged_) {
                    firstRawMouseAppliedLogged_ = true;
                    Log("OVERLAY_INPUT_RAW_MOUSE_APPLIED dx=" +
                        std::to_string(event.x) + " dy=" +
                        std::to_string(event.y) + " x=" +
                        std::to_string(virtualCursorPosition_.point().x) + " y=" +
                        std::to_string(virtualCursorPosition_.point().y));
                }
                break;
            case InputEventKind::MouseButton: {
                int button = -1;
                bool down = true;
                switch (event.message) {
                case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
                    button = 0; down = event.message != WM_LBUTTONUP; break;
                case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
                    button = 1; down = event.message != WM_RBUTTONUP; break;
                case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
                    button = 2; down = event.message != WM_MBUTTONUP; break;
                case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
                    button = HIWORD(event.wParam) == XBUTTON1 ? 3 : 4;
                    down = event.message != WM_XBUTTONUP;
                    break;
                default: break;
                }
                if (button >= 0) io.AddMouseButtonEvent(button, down);
                break;
            }
            case InputEventKind::MouseWheel:
                if (event.message == WM_MOUSEWHEEL)
                    io.AddMouseWheelEvent(0.0f,
                        static_cast<float>(static_cast<short>(HIWORD(event.wParam))) /
                            WHEEL_DELTA);
                else
                    io.AddMouseWheelEvent(
                        static_cast<float>(static_cast<short>(HIWORD(event.wParam))) /
                            WHEEL_DELTA, 0.0f);
                break;
            case InputEventKind::NativeKeyboardMessage:
                ImGui_ImplWin32_WndProcHandler(window_, event.message,
                    event.wParam, event.lParam);
                break;
            case InputEventKind::Focus:
                io.AddFocusEvent(event.wParam != FALSE);
                break;
            }
        }
        if (visible() && virtualCursorPosition_.known()) {
            const POINT position = virtualCursorPosition_.point();
            io.AddMousePosEvent(static_cast<float>(position.x),
                static_cast<float>(position.y));
        }
    }

    void Renderer::BeginImGuiFrame(UINT width, UINT height) noexcept
    {
        auto& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(width),
            static_cast<float>(height));
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = hasImGuiFrameTime_
            ? std::chrono::duration<float>(now - lastImGuiFrameTime_).count()
            : 1.0f / 60.0f;
        io.DeltaTime = (std::max)(elapsed, 1.0e-4f);
        lastImGuiFrameTime_ = now;
        hasImGuiFrameTime_ = true;
    }

    bool Renderer::RenderImpl(IDCompositionSurface* targetSurface,
        UINT width, UINT height)
    {
        if (!ready() || !targetSurface || !width || !height || !imguiReady_)
            return false;
        submittedNotificationCount_ = 0;
        submittedDrawLists_.clear();
        for (auto& notification : notifications_)
            notification.frameDrawListToken = 0;
        plugin::RuntimeSettingsSnapshot frameSettings{};
        bool frameSettingsAvailable = false;
        PrepareStartupNotification();
        UpdateNotifications();
        const bool overlayVisible = visible_.load(std::memory_order_acquire);
        if (!overlayVisible && notifications_.empty() && !surfaceDirty_)
            return false;
        if (ImGui::GetCurrentContext())
            ImGui::GetIO().MouseDrawCursor = overlayVisible;
#ifdef OVERLAY_SETTINGS_FRONTEND
        if (!frameSettingsAvailable)
            frameSettingsAvailable =
                plugin::GetRuntimeSettingsApi().Snapshot(frameSettings);
        if (frameSettingsAvailable) {
            const auto* localeDescriptor = localization::FindLocaleDescriptor(
                frameSettings.overlayLocaleCode);
            const std::string_view fontProfileCode = localeDescriptor
                ? localeDescriptor->fontProfileCode : std::string_view{"base"};
            bool fontSettingsChanged =
                frameSettings.overlayFontSize != activeFontSetting_ ||
                fontProfileCode != activeFontProfileCode_;
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
            fontSettingsChanged = fontSettingsChanged ||
                font_rasterization_experiment::CurrentMode() != activeFontRasterizationMode_;
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            fontSettingsChanged = fontSettingsChanged ||
                font_playground_experiment::CurrentFontChoice() != activeFontChoice_;
#endif
            if (fontSettingsChanged && !RebuildFontAtlas(
                    frameSettings.overlayFontSize, fontProfileCode)) {
                Disable("font_atlas_rebuild_failed");
                return false;
            }
            if (frameSettings.overlayLocaleCode != activeLocaleCode_)
                forceFullDamage_ = true;
            activeLocaleCode_ = frameSettings.overlayLocaleCode;
            localization_.SetLocale(frameSettings.overlayLocaleCode);
        }
#endif
        BeginImGuiFrame(width, height);
        ApplyInputEvents(width, height);
        ImGui::NewFrame();
        if (!ImGui::IsPopupOpen(static_cast<ImGuiID>(0), ImGuiPopupFlags_AnyPopup))
            GetInputState().ConsumeEscapePopupDismissRequest();
        ImGui::GetStyle().CellPadding.x = 6.0f * dpiScale_;
#ifdef OVERLAY_SETTINGS_FRONTEND
#endif
        if (overlayVisible) {
            const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
            const float maxOverlayWidth = layout_metrics::AvailableWindowWidth(
                mainViewport->Size.x, dpiScale_);
            ImGui::SetNextWindowPos(ImVec2(overlayPositionX_, overlayPositionY_),
                ImGuiCond_Once);
            ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f),
                ImVec2(maxOverlayWidth, FLT_MAX));
            const auto overlayTitle = Tr(localization_, loc::app::Title);
            ImGui::Begin(overlayTitle.c_str(), nullptr,
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoCollapse);
            const ImVec2 clampedPosition = ClampOverlayWindowPosition(
                ImGui::GetWindowPos(), ImGui::GetWindowSize(),
                ImGui::GetMainViewport(), dpiScale_);
            if (clampedPosition.x != ImGui::GetWindowPos().x ||
                clampedPosition.y != ImGui::GetWindowPos().y)
                ImGui::SetWindowPos(clampedPosition, ImGuiCond_Always);
            if (!overlayPositionObserved_) {
                lastOverlayPositionX_ = clampedPosition.x;
                lastOverlayPositionY_ = clampedPosition.y;
                overlayPositionObserved_ = true;
            } else if (clampedPosition.x != lastOverlayPositionX_ ||
                clampedPosition.y != lastOverlayPositionY_) {
                placementSaveState_.PositionChanged();
                lastOverlayPositionX_ = clampedPosition.x;
                lastOverlayPositionY_ = clampedPosition.y;
            }
            const bool leftMouseButtonDown =
                ImGui::IsMouseDown(ImGuiMouseButton_Left);
            if (placementSaveState_.ShouldAttemptSave(leftMouseButtonDown)) {
                const bool saved = !overlayConfigPath_.empty() &&
                    placement_config::Save(overlayConfigPath_,
                        static_cast<int>(clampedPosition.x),
                        static_cast<int>(clampedPosition.y));
                if (!saved) Log("OVERLAY_POSITION_PERSIST_FAILED");
                placementSaveState_.CompleteAttempt(saved);
            }
#ifdef OVERLAY_SETTINGS_FRONTEND
        plugin::OverlaySemanticSnapshot semantic{};
        const bool semanticReady = plugin::GetOverlaySemanticSnapshot(semantic) &&
            semantic.gameplayEnabled.valid && semantic.gameplayMode.valid &&
            semantic.cinematicAspectPolicy.valid && semantic.cinematicFovMode.valid &&
            semantic.dialogueZoomPolicy.valid;
        const bool settingsReady = semanticReady && frameSettingsAvailable;
        if (!settingsReady) {
            const auto unavailable = Tr(localization_, loc::common::Unavailable);
            ImGui::TextColored(FeatureStatusColor(FeatureStatusTone::Error), "%s", unavailable.c_str());
        } else {
            const auto runtimeSettingsLabel = Tr(localization_, loc::section::RuntimeSettings);
            const std::string toggleKeyName = config::HotkeyName(frameSettings.overlayToggleKey);
            const auto mouseCaptureNotice = Tr(localization_, loc::notice::MouseCapture,
                {{"key", std::string(kInlineHotkeyToken)}});
            ImGui::TextUnformatted(runtimeSettingsLabel.c_str());
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.94f, 0.98f, 1.0f));
            DrawInlineHotkeyMessage(mouseCaptureNotice, kInlineHotkeyToken,
                toggleKeyName);
            ImGui::PopStyleColor();
            const auto integration = ProjectCameraIntegration(semantic);
            const auto transitionHeader = Tr(localization_, loc::section::CameraTransition);
            ImGui::SeparatorText(transitionHeader.c_str());
            ImGui::BeginGroup();
            ImGui::TextColored(FeatureStatusColor(
                CameraTransitionTone(integration.overall)), "%s",
                Tr(localization_, OverallIntegrationLabel(integration.overall)).c_str());
            ImGui::EndGroup();
            const auto transitionDetail = Tr(localization_, CameraTransitionDetail(integration.overall));
            ImGui::TextWrapped("%s", transitionDetail.c_str());
            const auto gameplayModes = setting_tooltip_content::GameplaySelectorOptions();
            const auto cinematicAspects = setting_tooltip_content::CinematicAspectSelectorOptions();
            const auto cinematicFovs = setting_tooltip_content::CinematicFovSelectorOptions();
            const auto dialoguePolicies = setting_tooltip_content::DialogueSelectorOptions();
            float commonSelectWidth = IntrinsicComboWidth(gameplayModes);
            commonSelectWidth = (std::max)(commonSelectWidth,
                IntrinsicComboWidth(cinematicAspects));
            commonSelectWidth = (std::max)(commonSelectWidth,
                IntrinsicComboWidth(cinematicFovs));
            commonSelectWidth = (std::max)(commonSelectWidth,
                IntrinsicComboWidth(dialoguePolicies));
            constexpr char autoGameLanguageLabel[] = "Auto (Game Language)";
            commonSelectWidth = (std::max)(commonSelectWidth,
                IntrinsicOptionWidth(autoGameLanguageLabel));
            for (const auto& locale : localization_.locales()) {
                commonSelectWidth = (std::max)(commonSelectWidth,
                    IntrinsicOptionWidth(locale.displayName));
            }

            const ImGuiTableFlags layoutFlags = ImGuiTableFlags_SizingFixedSame |
                ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoSavedSettings;
            if (ImGui::BeginTable("settings_runtime_layout", 2, layoutFlags)) {
                ImGui::TableSetupColumn("settings");
                ImGui::TableSetupColumn("runtime");
                ImGui::TableNextColumn();
            const auto gameplayHeader = Tr(localization_, loc::section::Gameplay);
            ImGui::SeparatorText(gameplayHeader.c_str());
            ImGui::BeginGroup();
            bool gameplayEnabled = frameSettings.gameplayEnabled;
            if (ImGui::Checkbox("##gameplay_enabled", &gameplayEnabled) &&
                !ApplySetting(plugin::RuntimeSettingMutation::GameplayEnabled(gameplayEnabled), logger_))
                gameplayEnabled = frameSettings.gameplayEnabled;
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::common::Enabled);
            ImGui::EndGroup();
            ShowGameplayEnabledTooltip(localization_);
            ImGui::BeginGroup();
            ImGui::SetNextItemWidth(commonSelectWidth);
            const auto gameplayCombo = DrawSelectorCombo("##gameplay_mode",
                frameSettings.gameplayMode, gameplayModes);
            if (gameplayCombo.valueChanged)
                ApplySetting(plugin::RuntimeSettingMutation::Gameplay(gameplayCombo.value), logger_);
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::Mode);
            ImGui::EndGroup();
            ShowSettingOptionsTooltip(localization_, setting_tooltip_content::Gameplay(),
                gameplayCombo.interaction);
            DrawFeatureStatus(localization_, ProjectGameplayPresentation(semantic));

            const auto cinematicsHeader = Tr(localization_, loc::section::Cinematics);
            ImGui::SeparatorText(cinematicsHeader.c_str());
            ImGui::BeginGroup();
            ImGui::SetNextItemWidth(commonSelectWidth);
            const auto cinematicAspectCombo = DrawSelectorCombo("##cinematic_aspect",
                frameSettings.cinematicAspectPolicy, cinematicAspects);
            if (cinematicAspectCombo.valueChanged)
                ApplySetting(plugin::RuntimeSettingMutation::CinematicAspect(
                    cinematicAspectCombo.value), logger_);
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::AspectRatio);
            ImGui::EndGroup();
            ShowSettingOptionsTooltip(localization_, setting_tooltip_content::CinematicAspect(),
                cinematicAspectCombo.interaction);
            ImGui::BeginGroup();
            ImGui::SetNextItemWidth(commonSelectWidth);
            const auto cinematicFovCombo = DrawSelectorCombo("##cinematic_fov",
                frameSettings.cinematicFovMode, cinematicFovs);
            if (cinematicFovCombo.valueChanged)
                ApplySetting(plugin::RuntimeSettingMutation::CinematicFov(
                    cinematicFovCombo.value), logger_);
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::FovMode);
            ImGui::EndGroup();
            ShowSettingOptionsTooltip(localization_, setting_tooltip_content::CinematicFov(),
                cinematicFovCombo.interaction);
            const auto cinematicStatus = ProjectCinematicPresentation(semantic);
            if (cinematicStatus.kind == FeatureStatusKind::Active) {
                DrawActiveStatus(localization_, cinematicStatus.tone,
                    loc::status::CinematicActive,
                    std::string(config::CinematicAspectPolicyName(semantic.activeCinematicAspectPolicy.value)) +
                        " / " + config::CinematicFovModeName(semantic.activeCinematicFovMode.value));
            } else {
                DrawFeatureStatus(localization_, cinematicStatus);
            }

            const auto dialogueHeader = Tr(localization_, loc::section::Dialogue);
            ImGui::SeparatorText(dialogueHeader.c_str());
            ImGui::BeginGroup();
            ImGui::SetNextItemWidth(commonSelectWidth);
            const auto dialogueCombo = DrawSelectorCombo("##dialogue_zoom",
                frameSettings.dialogueZoomPolicy, dialoguePolicies);
            if (dialogueCombo.valueChanged)
                ApplySetting(plugin::RuntimeSettingMutation::Dialogue(
                    dialogueCombo.value), logger_);
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::Zoom);
            ImGui::EndGroup();
            ShowSettingOptionsTooltip(localization_, setting_tooltip_content::Dialogue(),
                dialogueCombo.interaction);
            const auto dialogueStatus = ProjectDialoguePresentation(semantic);
            if (dialogueStatus.kind == FeatureStatusKind::Active) {
                DrawActiveStatus(localization_, dialogueStatus.tone,
                    loc::status::DialogueActive,
                    ActiveDialoguePolicyLabel(semantic.activeDialogueZoomPolicy.value));
            } else {
                DrawFeatureStatus(localization_, dialogueStatus);
            }

            const auto overlayHeader = Tr(localization_, loc::section::Overlay);
            ImGui::SeparatorText(overlayHeader.c_str());
            ImGui::BeginGroup();
            const auto* selectedLocale = localization::FindLocaleDescriptor(
                frameSettings.overlayLocaleCode);
            ImGui::SetNextItemWidth(commonSelectWidth);
            if (ImGui::BeginCombo("##overlay_language",
                    frameSettings.overlayLocaleAuto ? "Auto (Game Language)" :
                    selectedLocale ? selectedLocale->displayName.data() : "")) {
                if (!CloseActiveComboForEscapeRequest()) {
                    const bool autoSelected = frameSettings.overlayLocaleAuto;
                    if (ImGui::Selectable("Auto (Game Language)", autoSelected)) {
                        if (ApplySetting(plugin::RuntimeSettingMutation::AutoLocale(true), logger_)) {
                            frameSettings.overlayLocaleAuto = true;
                            if (!RequestAutoLanguageSynchronization(window_) && logger_)
                                logger_("OVERLAY_AUTO_LANGUAGE_SYNC_REQUEST_FAILED");
                        }
                    }
                    if (autoSelected) ImGui::SetItemDefaultFocus();
                    for (const auto& locale : localization_.locales()) {
                        const bool isSelected =
                            !frameSettings.overlayLocaleAuto &&
                            locale.code == frameSettings.overlayLocaleCode;
                        ImFont* selectorFont = LocalizationSelectorFont(locale.fontProfileCode);
                        if (selectorFont) ImGui::PushFont(selectorFont);
                        if (ImGui::Selectable(locale.displayName.data(), isSelected)) {
                            const std::string targetLocaleCode(locale.code);
                            if (ApplySetting(plugin::RuntimeSettingMutation::LocaleCode(
                                    targetLocaleCode), logger_)) {
                                frameSettings.overlayLocaleCode = targetLocaleCode;
                                frameSettings.overlayLocaleAuto = false;
                                frameSettings.overlayFontSize = locale.initialFontSize;
                                pendingFontSizePixels_ = locale.initialFontSize;
                            }
                        }
                        if (selectorFont) ImGui::PopFont();
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::Language);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            if (pendingFontSizePixels_ == 0)
                pendingFontSizePixels_ = frameSettings.overlayFontSize;
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("  24 px  ").x +
                ImGui::GetStyle().FramePadding.x * 2.0f + 36.0f);
            ImGui::SliderInt("##overlay_font_size", &pendingFontSizePixels_,
                config::OverlayFontSizeMin, config::OverlayFontSizeMax, "%d px");
            const bool fontSizeEditFinished = ImGui::IsItemDeactivatedAfterEdit();
            if (fontSizeEditFinished && pendingFontSizePixels_ != frameSettings.overlayFontSize) {
                const auto mutation = plugin::RuntimeSettingMutation::FontSize(
                    pendingFontSizePixels_);
                if (ApplySetting(mutation, logger_)) {
                    frameSettings.overlayFontSize = pendingFontSizePixels_;
                }
            } else if (!ImGui::IsItemActive() && !fontSizeEditFinished) {
                pendingFontSizePixels_ = frameSettings.overlayFontSize;
            }
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::FontSize);
            ImGui::EndGroup();
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            ImGui::BeginGroup();
            int fontChoice = font_playground_experiment::CurrentFontChoice();
            static constexpr const char* fontChoices[] = {
                "ProggyClean + Proggy Vector Cyrillic",
                "Proggy Vector",
                "Gohu",
                "Terminus"
            };
            ImGui::SetNextItemWidth(IntrinsicComboWidth(fontChoices,
                IM_ARRAYSIZE(fontChoices)));
            if (ImGui::Combo("Font##font_playground_choice", &fontChoice,
                    fontChoices, IM_ARRAYSIZE(fontChoices)))
                font_playground_experiment::SetCurrentFontChoice(fontChoice);
            ImGui::EndGroup();
            ImGui::LabelText("##font_rasterizer", "Rasterizer", "stb Pixel Crisp");
#endif
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
            ImGui::BeginGroup();
            int rasterizationMode = font_rasterization_experiment::CurrentMode();
            static constexpr const char* rasterizationModes[] = {
                "Pixel Crisp", "Smooth 2x", "Smooth H2", "Smooth 3x"
            };
            ImGui::SetNextItemWidth(IntrinsicComboWidth(rasterizationModes,
                IM_ARRAYSIZE(rasterizationModes)));
            if (ImGui::Combo("Rasterization##font_rasterization_mode", &rasterizationMode,
                    rasterizationModes, IM_ARRAYSIZE(rasterizationModes)))
                font_rasterization_experiment::SetCurrentMode(rasterizationMode);
            ImGui::EndGroup();
#endif
            if (const auto* binding = FindHotkeyPresentation(
                    config::HotkeyBindingId::OverlayToggle))
                DrawHotkeyBinding(localization_, binding->label, binding->id,
                    frameSettings.*(binding->settingKey));

            const auto hotkeysHeader = Tr(localization_, loc::section::Hotkeys);
            ImGui::SeparatorText(hotkeysHeader.c_str());
            ImGui::BeginGroup();
            bool hotkeysEnabled = frameSettings.hotkeysEnabled;
            if (ImGui::Checkbox("##hotkeys_enabled", &hotkeysEnabled)) {
                const auto mutation = plugin::RuntimeSettingMutation::HotkeysEnabled(
                    hotkeysEnabled);
                ApplySetting(mutation, logger_);
            }
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::common::Enabled);
            ImGui::EndGroup();
            ShowHotkeysEnabledTooltip(localization_);
            for (const auto& binding : HotkeyBindingPresentations) {
                if (binding.group != HotkeyBindingGroup::Hotkeys) continue;
                DrawHotkeyBinding(localization_, binding.label, binding.id,
                    frameSettings.*(binding.settingKey));
            }

            ImGui::TableNextColumn();
            const auto runtimeHeader = Tr(localization_, loc::section::Runtime);
            ImGui::SeparatorText(runtimeHeader.c_str());
            const auto unavailableValue = Tr(localization_, loc::common::UnavailableValue);
            static constexpr TooltipOption viewportAspectTooltip[] = {
                {{}, loc::ui::ViewportAspect, loc::tooltip::runtime::Viewport},
                {"Auto", {}, loc::tooltip::runtime::ViewportAuto},
                {{}, loc::common::Unavailable, loc::tooltip::runtime::ViewportUnavailable},
            };
            char viewportAspectValue[64]{};
            if (semantic.runtimeViewportAspect.valid)
                std::snprintf(viewportAspectValue, sizeof(viewportAspectValue), ": %.4f",
                    semantic.runtimeViewportAspect.value);
            else
                std::snprintf(viewportAspectValue, sizeof(viewportAspectValue), ": %s",
                    unavailableValue.c_str());
            DrawRuntimeInfoRow(localization_, loc::ui::ViewportAspect, viewportAspectValue,
                viewportAspectTooltip);

            const auto cameraIntegrationHeader = Tr(localization_, loc::section::CameraIntegration);
            ImGui::SeparatorText(cameraIntegrationHeader.c_str());
            static constexpr TooltipOption aspectTooltip[] = {
                {{}, loc::transition::Aligned, loc::tooltip::runtime::AspectMatched},
                {{}, loc::transition::NotAligned, loc::tooltip::runtime::AspectMismatch},
                {{}, loc::common::CannotAssess, loc::tooltip::runtime::AspectCannotAssess},
            };
            char aspectValue[64]{};
            std::snprintf(aspectValue, sizeof(aspectValue), ": %s",
                Tr(localization_, AspectAssessmentLabel(integration.aspect)).c_str());
            DrawRuntimeInfoRow(localization_, loc::camera_state::Aspect, aspectValue, aspectTooltip);

            static constexpr TooltipOption cinematicAspectTooltip[] = {
                {{}, loc::setting::CinematicAspect, loc::tooltip::runtime::CinematicAspect},
                {{}, loc::common::Unavailable, loc::tooltip::runtime::CinematicAspectUnavailable},
            };
            char cinematicAspectValue[96]{};
            if (integration.resolvedCinematicAspectValid &&
                semantic.runtimeViewportAspect.valid) {
                std::snprintf(cinematicAspectValue, sizeof(cinematicAspectValue),
                    ": %.4f -> viewport %.4f", integration.resolvedCinematicAspect,
                    semantic.runtimeViewportAspect.value);
            } else {
                std::snprintf(cinematicAspectValue, sizeof(cinematicAspectValue), ": %s",
                    unavailableValue.c_str());
            }
            DrawRuntimeInfoRow(localization_, loc::setting::CinematicAspect, cinematicAspectValue,
                cinematicAspectTooltip);

            static constexpr TooltipOption gameplayFovTooltip[] = {
                {{}, loc::camera_state::GameplayFov, loc::tooltip::runtime::GameplayFov},
                {{}, loc::common::Unavailable, loc::tooltip::runtime::GameplayFovUnavailable},
            };
            char gameplayFovValue[64]{};
            if (integration.gameplayFovValid)
                std::snprintf(gameplayFovValue, sizeof(gameplayFovValue), ": %.2f",
                    integration.gameplayFov);
            else
                std::snprintf(gameplayFovValue, sizeof(gameplayFovValue), ": %s",
                    unavailableValue.c_str());
            DrawRuntimeInfoRow(localization_, loc::camera_state::GameplayFov, gameplayFovValue,
                gameplayFovTooltip);

            static constexpr TooltipOption nativeFovTooltip[] = {
                {{}, loc::camera_state::Native, loc::tooltip::runtime::NativeFov},
                {{}, loc::common::Unavailable, loc::tooltip::runtime::NativeFovUnavailable},
            };
            char nativeFovValue[64]{};
            if (integration.nativeFovValid)
                std::snprintf(nativeFovValue, sizeof(nativeFovValue), ": %.2f",
                    integration.nativeFov);
            else
                std::snprintf(nativeFovValue, sizeof(nativeFovValue), ": %s",
                    unavailableValue.c_str());
            DrawRuntimeInfoRow(localization_, loc::camera_state::Native, nativeFovValue, nativeFovTooltip);

            static constexpr TooltipOption horPlusFovTooltip[] = {
                {{}, loc::camera_state::HorPlus, loc::tooltip::runtime::HorPlusFov},
                {{}, loc::common::Unavailable, loc::tooltip::runtime::HorPlusFovUnavailable},
            };
            char horPlusFovValue[64]{};
            if (integration.horPlusFovValid)
                std::snprintf(horPlusFovValue, sizeof(horPlusFovValue), ": %.2f",
                    integration.horPlusFov);
            else
                std::snprintf(horPlusFovValue, sizeof(horPlusFovValue), ": %s",
                    unavailableValue.c_str());
            DrawRuntimeInfoRow(localization_, loc::camera_state::HorPlus, horPlusFovValue, horPlusFovTooltip);
            const auto cameraStateLabel = Tr(localization_, loc::section::CameraState);
            if (ImGui::TreeNodeEx(cameraStateLabel.c_str(), ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth)) {
                DrawCameraStateView(localization_, semantic);
                ImGui::TreePop();
            }
                ImGui::EndTable();
            }
        }
#else
#endif
            ImGui::End();
        }
        DrawNotifications();
        PublishOverlayPopupState(ImGui::IsPopupOpen(
            static_cast<ImGuiID>(0), ImGuiPopupFlags_AnyPopup));
        ImGui::Render();
        const ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData) {
            submittedDrawLists_.reserve(static_cast<std::size_t>(
                (std::max)(0, drawData->CmdListsCount)));
            for (int index = 0; index < drawData->CmdListsCount; ++index) {
                const ImDrawList* list = drawData->CmdLists[index];
                if (list && list->VtxBuffer.Size > 0)
                    submittedDrawLists_.push_back({
                        reinterpret_cast<std::uintptr_t>(list),
                        static_cast<std::size_t>(list->VtxBuffer.Size)});
            }
            for (const auto& notification : notifications_) {
                if (!notification.frameDrawListToken ||
                    submittedNotificationCount_ >=
                        submittedNotificationIds_.size())
                    continue;
                if (NotificationDrawListWasSubmitted(
                        notification.frameDrawListToken, submittedDrawLists_))
                    submittedNotificationIds_[submittedNotificationCount_++] =
                        notification.notification.id;
            }
        }
        const presentation::Rect surfaceExtent{0, 0,
            static_cast<int>(width), static_cast<int>(height)};
        presentation::Rect currentBounds{};
        if (!ComputeImGuiDrawBounds(drawData, width, height, currentBounds)) {
            currentBounds = surfaceExtent;
            forceFullDamage_ = true;
        }
        if (!imguiRenderer_.SupportsPartialSurfaceUpdates())
            currentBounds = surfaceExtent;
        const presentation::Rect damage = damageTracker_.Plan(currentBounds,
            surfaceExtent, forceFullDamage_);
        if (!damage.empty() && !DrawImGuiSurface(targetSurface, width, height,
                drawData, damage)) {
            const HRESULT removedReason = device_ ? device_->GetDeviceRemovedReason() : E_FAIL;
            Log("OVERLAY_DCOMP_DRAW_FAILED hr=" +
                std::to_string(static_cast<long>(removedReason)));
            if (FAILED(removedReason)) {
                if (state_.BeginDeviceRecovery()) {
                    Log("OVERLAY_DCOMP_DEVICE_LOST recovery=started");
                    if (RecoverGraphicsDevice(width, height)) return false;
                    state_.CompleteDeviceRecovery(false, {});
                    Disable("dcomp_device_lost_recovery_failed");
                } else {
                    Disable("dcomp_device_lost");
                }
            } else {
                Disable("dcomp_surface_draw_failed");
            }
            return false;
        }
        damageTracker_.Publish(currentBounds, surfaceExtent);
        forceFullDamage_ = false;
        surfaceDirty_ = false;
        if (!firstFrameLogged_) {
            firstFrameLogged_ = true;
            LogDiagnostic(logger_, "OVERLAY_FIRST_FRAME");
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::MarkOnce(9, "OVERLAY_FIRST_FRAME",
                "first_directcomposition_surface_commit", surfaceGeneration_);
            diagnostics::startup_journal::Flush();
#endif
        }
        return true;
    }

    void Renderer::Disable(const char* reason) noexcept
    {
        visible_.store(false, std::memory_order_release);
        PublishOverlayPopupState(false);
        try {
            GetInputState().Close();
        } catch (...) {
            // The atomic visibility gate is already closed; finish resource teardown.
        }
        try {
            Log(std::string("OVERLAY_RENDER_DISABLED reason=") + reason);
        } catch (...) {
            // Diagnostics are optional; terminal cleanup is not.
        }
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(10, "OVERLAY_DISABLED", reason);
        diagnostics::startup_journal::Flush();
#endif
        Shutdown();
        state_.DisableOverlay();
    }

    void Renderer::FailAfterException() noexcept
    {
        Disable("overlay_exception");
    }

    void Renderer::Shutdown() noexcept
    {
        PublishOverlayPopupState(false);
        try {
            GetInputState().CancelRebind();
        } catch (...) {
            // Rebinding is best-effort cleanup; renderer resources still tear down.
        }
        plugin::SetHotkeyRebindCaptureActive(false);
        plugin::MarkHotkeyRebindKeyConsumed(0);
        DetachImGui();
        ResetComposition();
        window_ = nullptr;
        surfaceGeneration_ = 0;
        firstFrameLogged_ = false;
        surfaceDirty_ = true;
    }
}
