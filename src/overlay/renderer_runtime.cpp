#include "renderer_runtime.hpp"

#include "optional_overlay_boundary.hpp"
#include "../diagnostics/diagnostic_runtime.hpp"

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
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
            const std::string keyText(keyLabel);
            ImGui::TextColored(ImVec4(0.18f, 0.78f, 1.0f, 1.0f), "%s",
                keyText.c_str());
            if (!afterKey.empty()) {
                ImGui::SameLine(0.0f, 4.0f);
                const float afterWidth = ImGui::CalcTextSize(afterKey.c_str()).x;
                if (afterWidth > ImGui::GetContentRegionAvail().x) {
                    ImGui::NewLine();
                    const auto firstNonSpace = afterKey.find_first_not_of(" \t\r\n");
                    const std::string_view tail = firstNonSpace == std::string::npos
                        ? std::string_view{} : std::string_view(afterKey).substr(firstNonSpace);
                    if (!tail.empty())
                        ImGui::TextWrapped("%.*s", static_cast<int>(tail.size()), tail.data());
                } else {
                    ImGui::TextUnformatted(afterKey.c_str());
                }
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
            const ImVec2& windowSize, const ImGuiViewport* viewport) noexcept
        {
            if (!viewport) return position;
            const auto clamped = layout_metrics::ClampPosition(
                {position.x, position.y}, {viewport->Pos.x, viewport->Pos.y},
                {viewport->Size.x, viewport->Size.y},
                {windowSize.x, windowSize.y});
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

    void Renderer::AllocateSrv(ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu)
    {
        auto* renderer = static_cast<Renderer*>(info->UserData);
        *cpu = renderer->srvHeap_->GetCPUDescriptorHandleForHeapStart();
        *gpu = renderer->srvHeap_->GetGPUDescriptorHandleForHeapStart();
    }

    void Renderer::FreeSrv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE,
        D3D12_GPU_DESCRIPTOR_HANDLE)
    {
    }

    bool Renderer::Initialize(IDXGISwapChain* swapchain, ID3D12Device* device,
        ID3D12CommandQueue* queue, HWND window, UINT bufferCount,
        DXGI_FORMAT format) noexcept
    {
        bool initialized = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            initialized = InitializeImpl(swapchain, device, queue, window,
                bufferCount, format);
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && initialized;
    }

    bool Renderer::InitializeImpl(IDXGISwapChain* swapchain, ID3D12Device* device,
        ID3D12CommandQueue* queue, HWND window, UINT bufferCount,
        DXGI_FORMAT format)
    {
        if (!swapchain || !device || !queue || !bufferCount || bufferCount > 16)
            return false;
        if (swapchain_ == swapchain && device_ == device && queue_ == queue &&
            lifecycle_.state() == RendererState::Ready)
            return true;
        if (swapchain_ || device_ || queue_) {
            Shutdown();
            lifecycle_ = RendererLifecycle{};
        }
        if (!lifecycle_.BeginInitialization()) return false;

        Log("OVERLAY_RENDER_INIT_BEGIN");
        swapchain_ = swapchain;
        device_ = device;
        queue_ = queue;
        window_ = window;
        bufferCount_ = bufferCount;
        format_ = format;
        swapchain_->AddRef();
        device_->AddRef();
        queue_->AddRef();

        auto fail = [this]() noexcept {
            Disable("initialization_failed");
            return false;
        };
        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
        rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDesc.NumDescriptors = bufferCount_;
        if (FAILED(device_->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap_))))
            return fail();
        rtvStride_ = device_->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
        srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvDesc.NumDescriptors = 1;
        srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(device_->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&srvHeap_))))
            return fail();
        if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&fence_))))
            return fail();
        fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent_) return fail();
        frames_.resize(bufferCount_);
        for (auto& frame : frames_) {
            if (FAILED(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&frame.allocator))))
                return fail();
        }
        if (FAILED(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            frames_.front().allocator, nullptr, IID_PPV_ARGS(&commandList_))))
            return fail();
        commandList_->Close();
        if (!BuildResources(swapchain_) || !BuildImGui()) return fail();
        lifecycle_.MarkReady();
        Log("OVERLAY_RENDER_INIT_OK");
        return true;

    }

    bool Renderer::BuildResources(IDXGISwapChain* swapchain)
    {
        if (!device_ || !rtvHeap_ || !swapchain) return false;
        DXGI_SWAP_CHAIN_DESC desc{};
        if (FAILED(swapchain->GetDesc(&desc)) || desc.BufferCount != bufferCount_)
            return false;
        ReleaseBackbuffers();
        backbuffers_.resize(bufferCount_);
        auto handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < bufferCount_; ++i) {
            if (FAILED(swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffers_[i])))) {
                ReleaseBackbuffers();
                return false;
            }
            D3D12_CPU_DESCRIPTOR_HANDLE target = handle;
            target.ptr += static_cast<SIZE_T>(i) * rtvStride_;
            device_->CreateRenderTargetView(backbuffers_[i], nullptr, target);
        }
        resourcesReady_ = true;
        Log("OVERLAY_RESOURCES_CREATED bufferCount=" + std::to_string(bufferCount_));
        return true;
    }

    bool Renderer::BuildImGui()
    {
        if (!device_ || !queue_ || !srvHeap_ || !window_) return false;
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
            int fontSizePixels = config::OverlayFontSizeDefault;
#ifdef OVERLAY_SETTINGS_FRONTEND
            plugin::RuntimeSettingsSnapshot settings{};
            if (plugin::GetRuntimeSettingsApi().Snapshot(settings))
                fontSizePixels = settings.overlayFontSize;
#endif
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
        bool win32Initialized = false;
        if (!imguiReady_) {
            if (!ImGui_ImplWin32_Init(window_)) return false;
            win32Initialized = true;
        }
        ImGui_ImplDX12_InitInfo info{};
        info.Device = device_;
        info.CommandQueue = queue_;
        info.NumFramesInFlight = static_cast<int>(bufferCount_);
        info.RTVFormat = format_;
        info.DSVFormat = DXGI_FORMAT_UNKNOWN;
        info.UserData = this;
        info.SrvDescriptorHeap = srvHeap_;
        info.SrvDescriptorAllocFn = &AllocateSrv;
        info.SrvDescriptorFreeFn = &FreeSrv;
        if (!imguiReady_ && !ImGui_ImplDX12_Init(&info)) {
            if (win32Initialized) ImGui_ImplWin32_Shutdown();
            return false;
        }
        imguiReady_ = true;
        if (!ImGui_ImplDX12_CreateDeviceObjects()) {
            ImGui_ImplDX12_Shutdown();
            ImGui_ImplWin32_Shutdown();
            imguiReady_ = false;
            return false;
        }
        return true;
    }

    bool Renderer::WaitForGpu() noexcept
    {
        if (!queue_ || !fence_ || !fenceEvent_) return false;
        const auto value = ++nextFenceValue_;
        if (FAILED(queue_->Signal(fence_, value))) return false;
        if (fence_->GetCompletedValue() < value) {
            if (FAILED(fence_->SetEventOnCompletion(value, fenceEvent_))) return false;
            WaitForSingleObject(fenceEvent_, INFINITE);
        }
        return true;
    }

    bool Renderer::RebuildFontAtlas(int fontSizePixels,
        std::string_view fontProfileCode)
    {
        if (!imguiReady_ || !ImGui::GetCurrentContext() ||
            fontSizePixels < config::OverlayFontSizeMin ||
            fontSizePixels > config::OverlayFontSizeMax)
            return false;
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
            !fontChoiceChanged) return true;
        if (!WaitForGpu()) {
            Log("OVERLAY_FONT_REBUILD_GPU_WAIT_FAILED");
            return false;
        }

        ImGui_ImplDX12_InvalidateDeviceObjects();
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
            ImGui_ImplDX12_CreateDeviceObjects();
        if (rebuilt) {
            activeFontSizePixels_ = fontSizePixels;
            activeFontProfileCode_ = fontProfileCode;
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
        ImGui_ImplDX12_InvalidateDeviceObjects();
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
            ImGui_ImplDX12_CreateDeviceObjects();
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
        ApplySetting(plugin::RuntimeSettingMutation::FontSize(activeFontSizePixels_), logger_);
#endif
        Log("OVERLAY_FONT_ATLAS_RESTORED size=" + std::to_string(activeFontSizePixels_));
        return true;
    }

    bool Renderer::WaitForFrame(FrameContext& frame) noexcept
    {
        if (!frame.fenceValue || fence_->GetCompletedValue() >= frame.fenceValue)
            return true;
        if (FAILED(fence_->SetEventOnCompletion(frame.fenceValue, fenceEvent_)))
            return false;
        WaitForSingleObject(fenceEvent_, INFINITE);
        return true;
    }

    void Renderer::ReleaseBackbuffers() noexcept
    {
        for (auto*& resource : backbuffers_) {
            if (resource) resource->Release();
            resource = nullptr;
        }
        backbuffers_.clear();
        resourcesReady_ = false;
    }

    void Renderer::BeforeResize()
    {
        if (lifecycle_.state() != RendererState::Ready) return;
        lifecycle_.BeginResize();
        LogDiagnostic(logger_, "OVERLAY_RESIZE_RELEASE");
        const auto waitStart = std::chrono::steady_clock::now();
        if (!WaitForGpu()) {
            Disable("resize_gpu_wait_failed");
            return;
        }
        resizePreWaitMs_ = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - waitStart).count();
        const auto releaseStart = std::chrono::steady_clock::now();
        if (imguiReady_) ImGui_ImplDX12_InvalidateDeviceObjects();
        ReleaseBackbuffers();
        resizeReleaseMs_ = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - releaseStart).count();
    }

    void Renderer::OnResizeResult(bool success, double originalResizeMs,
        double totalResizeHookMs)
    {
        originalResizeMs_ = originalResizeMs;
        totalResizeHookMs_ = totalResizeHookMs;
        resizeResultTime_ = std::chrono::steady_clock::now();
        if (!success) {
            Disable("resize_failed");
            return;
        }
        LogDiagnostic(logger_, "OVERLAY_RESIZE_REVALIDATION_WAIT");
    }

    void Renderer::UpdateNotifications() noexcept
    {
        try {
            std::vector<plugin::OverlayNotification> incoming;
            plugin::DrainOverlayNotifications(incoming);
            const auto now = static_cast<std::uint64_t>(GetTickCount64());
            for (auto& notification : incoming) {
                if (notification.kind == plugin::OverlayNotificationKind::StartupOverlayHint)
                    notification.createdAtMs = now;
                if (notifications_.size() >= 3)
                    notifications_.erase(notifications_.begin());
                notifications_.push_back(std::move(notification));
            }
            notifications_.erase(std::remove_if(notifications_.begin(), notifications_.end(),
                [now](const auto& notification) {
                    return NotificationExpired(notification.createdAtMs, now,
                        notification.durationMs);
                }), notifications_.end());
        } catch (...) {
            // Toasts are optional and must never affect rendering or settings.
        }
    }

    void Renderer::DrawNotifications()
    {
        const auto now = static_cast<std::uint64_t>(GetTickCount64());
        float y = 30.0f;
        for (const auto& notification : notifications_) {
            if (now < notification.createdAtMs) continue;
            const auto age = now - notification.createdAtMs;
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
            ImGui::SetNextWindowSizeConstraints(ImVec2(250.0f, 0.0f),
                ImVec2(440.0f, FLT_MAX));
            ImGui::SetNextWindowBgAlpha(0.92f * alpha);
            ImGui::Begin(windowId.c_str(), nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_AlwaysAutoResize);
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

    void Renderer::Render(IDXGISwapChain* swapchain,
        AssociationState association) noexcept
    {
        const auto outcome = RunOptionalOverlayWork([&]() {
            RenderImpl(swapchain, association);
        }, [this]() noexcept { FailAfterException(); });
        (void)outcome;
    }

    void Renderer::RenderImpl(IDXGISwapChain* swapchain,
        AssociationState association)
    {
        UpdateNotifications();
        if (!swapchain || association != AssociationState::Supported) return;
        if (swapchain != swapchain_) {
            Disable("swapchain_identity_changed");
            return;
        }
        if (lifecycle_.state() == RendererState::Resizing) {
            const auto resourceRebuildStart = std::chrono::steady_clock::now();
            if (!BuildResources(swapchain) || !BuildImGui() ||
                !lifecycle_.CompleteResize(true)) {
                Disable("resize_rebuild_failed");
                return;
            }
            const auto resourceRebuildMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - resourceRebuildStart).count();
            const auto deferredRebuildDelayMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - resizeResultTime_).count();
            Log("OVERLAY_RESIZE_REBUILD_OK resourceRebuildMs=" +
                std::to_string(resourceRebuildMs) + " deferredAfterResizeMs=" +
                std::to_string(deferredRebuildDelayMs));
            LogDiagnostic(logger_, "OVERLAY_RESIZE_TIMING originalResizeBuffersMs=" +
                std::to_string(originalResizeMs_) + " resizeHookMs=" +
                std::to_string(totalResizeHookMs_) + " preResizeWaitMs=" +
                std::to_string(resizePreWaitMs_) + " overlayReleaseMs=" +
                std::to_string(resizeReleaseMs_));
        }
        if (!resourcesReady_ || !lifecycle_.CanSubmit(association)) return;
#ifdef OVERLAY_SETTINGS_FRONTEND
        plugin::RuntimeSettingsSnapshot frameSettings{};
        bool frameSettingsAvailable = false;
        const bool rendererReady = lifecycle_.state() == RendererState::Ready &&
            imguiReady_ && resourcesReady_;
        if (startupHintGate_.ShouldReadSettings(rendererReady, true)) {
            frameSettingsAvailable =
                plugin::GetRuntimeSettingsApi().Snapshot(frameSettings);
            if (frameSettingsAvailable && frameSettings.overlayLocaleAuto &&
                !frameSettings.overlayLocaleAutoSynchronized &&
                !startupAutoLanguageSyncRequested_) {
                startupAutoLanguageSyncRequested_ =
                    RequestAutoLanguageSynchronization(window_);
                if (!startupAutoLanguageSyncRequested_ &&
                    !startupAutoLanguageSyncRequestFailureLogged_) {
                    Log("OVERLAY_STARTUP_AUTO_LANGUAGE_SYNC_REQUEST_FAILED");
                    startupAutoLanguageSyncRequestFailureLogged_ = true;
                }
            }
            const bool startupLocaleReady = frameSettingsAvailable &&
                StartupLocaleReady(frameSettings.overlayLocaleAuto,
                    frameSettings.overlayLocaleAutoSynchronized);
            if (startupHintGate_.TryPublish(rendererReady, true,
                    startupLocaleReady)) {
                try {
                    const std::string keyName = config::HotkeyName(
                        frameSettings.overlayToggleKey);
                    plugin::PublishOverlayNotification(
                        plugin::OverlayNotificationKind::StartupOverlayHint,
                        plugin::OverlayNotificationAction::OverlayToggle,
                        keyName, {}, plugin::OverlayNotificationStatus::Ready,
                        StartupHintDurationMs);
                    Log("OVERLAY_STARTUP_HINT_QUEUED key=" + keyName);
                } catch (...) {
                    // Startup guidance is optional; it must not interrupt rendering.
                }
            }
        }
#endif
        const bool overlayVisible = visible_.load(std::memory_order_acquire);
        if ((!overlayVisible && notifications_.empty())) return;
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
                frameSettings.overlayFontSize != activeFontSizePixels_ ||
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
                return;
            }
            localization_.SetLocale(frameSettings.overlayLocaleCode);
        }
#endif
        IDXGISwapChain3* swapchain3 = nullptr;
        if (FAILED(swapchain->QueryInterface(IID_PPV_ARGS(&swapchain3))) || !swapchain3)
            return;
        const UINT index = swapchain3->GetCurrentBackBufferIndex();
        swapchain3->Release();
        if (index >= frames_.size() || index >= backbuffers_.size()) {
            Disable("backbuffer_index_invalid");
            return;
        }
        auto& frame = frames_[index];
        if (!WaitForFrame(frame) || FAILED(frame.allocator->Reset()) ||
            FAILED(commandList_->Reset(frame.allocator, nullptr))) {
            Disable("command_reuse_failed");
            return;
        }
        D3D12_RESOURCE_BARRIER toTarget{};
        toTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toTarget.Transition.pResource = backbuffers_[index];
        toTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        toTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList_->ResourceBarrier(1, &toTarget);
        auto rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += static_cast<SIZE_T>(index) * rtvStride_;
        commandList_->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        ID3D12DescriptorHeap* heaps[] = {srvHeap_};
        commandList_->SetDescriptorHeaps(1, heaps);

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (!ImGui::IsPopupOpen(static_cast<ImGuiID>(0), ImGuiPopupFlags_AnyPopup))
            GetInputState().ConsumeEscapePopupDismissRequest();
        ImGui::GetStyle().CellPadding.x = 6.0f;
#ifdef OVERLAY_SETTINGS_FRONTEND
#endif
        if (overlayVisible) {
            const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
            const float maxOverlayWidth = layout_metrics::AvailableWindowWidth(
                mainViewport->Size.x);
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
                ImGui::GetMainViewport());
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
        ImGui::TextUnformatted("Overlay Rendering POC");
        ImGui::TextUnformatted("Presentation path: OK");
        ImGui::Text("D3D12 queue: DIRECT");
        ImGui::Text("Buffers: %u", bufferCount_);
#endif
            ImGui::End();
        }
        DrawNotifications();
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_);

        D3D12_RESOURCE_BARRIER toPresent = toTarget;
        toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList_->ResourceBarrier(1, &toPresent);
        if (FAILED(commandList_->Close())) {
            Disable("command_list_close_failed");
            return;
        }
        ID3D12CommandList* lists[] = {commandList_};
        queue_->ExecuteCommandLists(1, lists);
        const auto fenceValue = ++nextFenceValue_;
        if (FAILED(queue_->Signal(fence_, fenceValue))) {
            Disable("queue_signal_failed");
            return;
        }
        frame.fenceValue = fenceValue;
        if (fenceValue == 1) LogDiagnostic(logger_, "OVERLAY_FIRST_FRAME");
    }

    void Renderer::Disable(const char* reason) noexcept
    {
        visible_.store(false, std::memory_order_release);
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
        Shutdown();
        lifecycle_.Disable();
    }

    void Renderer::FailAfterException() noexcept
    {
        Disable("overlay_exception");
    }

    void Renderer::Shutdown() noexcept
    {
        try {
            GetInputState().CancelRebind();
        } catch (...) {
            // Rebinding is best-effort cleanup; renderer resources still tear down.
        }
        plugin::SetHotkeyRebindCaptureActive(false);
        plugin::MarkHotkeyRebindKeyConsumed(0);
        if (queue_ && fence_) WaitForGpu();
        if (ImGui::GetCurrentContext()) {
            auto& io = ImGui::GetIO();
            // The backend init paths can allocate and fail partway through.
            // Their owned ImGui IO slots, not the combined ready flag, tell
            // teardown which individual backend reached its own boundary.
            if (io.BackendRendererUserData)
                ImGui_ImplDX12_Shutdown();
            if (io.BackendPlatformUserData)
                ImGui_ImplWin32_Shutdown();
        }
        imguiReady_ = false;
        if (imguiContextCreated_ && ImGui::GetCurrentContext())
            ImGui::DestroyContext();
        imguiContextCreated_ = false;
        ReleaseBackbuffers();
        for (auto& frame : frames_)
            if (frame.allocator) frame.allocator->Release();
        frames_.clear();
        if (commandList_) commandList_->Release();
        if (fence_) fence_->Release();
        if (rtvHeap_) rtvHeap_->Release();
        if (srvHeap_) srvHeap_->Release();
        if (fenceEvent_) CloseHandle(fenceEvent_);
        if (swapchain_) swapchain_->Release();
        if (device_) device_->Release();
        if (queue_) queue_->Release();
        commandList_ = nullptr;
        fence_ = nullptr;
        rtvHeap_ = nullptr;
        srvHeap_ = nullptr;
        fenceEvent_ = nullptr;
        swapchain_ = nullptr;
        device_ = nullptr;
        queue_ = nullptr;
        resourcesReady_ = false;
        nextFenceValue_ = 0;
    }
}
