#include "src/overlay/presenter/renderer_runtime.hpp"
#include "src/plugin/runtime.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef OVERLAY_TARGET_TIMING_SMOKE
#include "src/overlay/presenter/target_timing.hpp"
#include <thread>
#endif

namespace {
    std::vector<std::string> records;
    void Log(const std::string& message) { records.push_back(message); std::cout << message << '\n'; }
    void Require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    bool Snapshot(plugin::RuntimeSettingsSnapshot& out, void*) {
        out = {};
        out.overlayLocaleCode = "en";
        out.overlayLocaleAuto = false;
        return true;
    }
    bool Semantic(plugin::OverlaySemanticSnapshot& out, void*) { out = {}; return true; }
    void ExpectRoot(overlay::Renderer& renderer, bool attached, const char* reason) {
#ifdef OVERLAY_ATTACHMENT_PRODUCTION_SMOKE
        (void)renderer;
        for (auto record = records.rbegin(); record != records.rend(); ++record) {
            if (record->find("OVERLAY_DCOMP_ROOT_ATTACHMENT attached=") == std::string::npos) continue;
            Require(record->find(attached ? "attached=1" : "attached=0") != std::string::npos, reason);
            return;
        }
        Require(false, "production attachment lifecycle record missing");
#else
        renderer.LogAttachmentExperimentSnapshot(reason);
        Require(!records.empty() && records.back().find(std::string("root_commit_accepted=") +
            (attached ? "1" : "0")) != std::string::npos, reason);
#endif
    }
    void Pump(overlay::Renderer& renderer, DWORD duration) {
        const auto until = GetTickCount64() + duration;
        do {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&message); DispatchMessageW(&message);
            }
            if (renderer.needsRenderWork()) renderer.Render();
#ifdef OVERLAY_TARGET_TIMING_SMOKE
            overlay::target_timing_experiment::Drain(Log);
#endif
            Require(!renderer.disabled(), "renderer unexpectedly disabled");
            Sleep(16);
        } while (GetTickCount64() < until);
    }
    HWND CreateFixtureWindow(HINSTANCE instance) {
        return CreateWindowExW(WS_EX_NOACTIVATE, L"OverlayAttachmentFixture",
            L"Overlay attachment fixture", WS_POPUP, 0, 0, 640, 480,
            nullptr, nullptr, instance, nullptr);
    }
#ifdef OVERLAY_TARGET_RELEASE_SMOKE
    Microsoft::WRL::ComPtr<IDCompositionDevice> probeDevice;
    void CheckLayerAvailability(HWND window, bool available) {
        Microsoft::WRL::ComPtr<IDCompositionTarget> target;
        const HRESULT result = probeDevice->CreateTargetForHwnd(window, TRUE,
            target.GetAddressOf());
        Require(available ? SUCCEEDED(result) : FAILED(result),
            available ? "released target layer available" : "open target layer occupied");
        std::cout << "target_layer_probe available=" << available << " hr=" << result << '\n';
    }
#endif
}

int main() {
    try {
#ifdef OVERLAY_TARGET_TIMING_SMOKE
        namespace timing = overlay::target_timing_experiment;
        timing::Initialize();
        const auto threshold = (timing::frequency.load() + 499) / 500;
        Require(!timing::Record(timing::Operation::Commit, 1000, 1000 + threshold - 1),
            "subthreshold timing skipped");
        Require(timing::Record(timing::Operation::Commit, 1000, 1000 + threshold),
            "inclusive threshold recorded");
        unsigned count = 0;
        timing::Consume([&](const timing::Sample& sample) {
            Require(sample.operation == timing::Operation::Commit && sample.end - sample.begin == threshold,
                "sample fields preserved"); ++count;
        });
        Require(count == 1, "exactly one threshold sample");
        for (unsigned index = 0; index < 70; ++index)
            timing::Record(timing::Operation::SetRoot, 1000, 1000 + threshold);
        count = 0; timing::Consume([&](const timing::Sample&) { ++count; });
        Require(count == 64 && timing::dropped.exchange(0) == 6, "bounded overflow");
        Require(timing::Invoke(timing::Operation::Commit, [] { return E_FAIL; }) == E_FAIL,
            "HRESULT preserved");
        bool caught = false;
        try { timing::Invoke(timing::Operation::SetRoot, [] { throw std::runtime_error("fixture"); }); }
        catch (const std::runtime_error&) { caught = true; }
        Require(caught, "exception behavior preserved");
        std::recursive_mutex mutex;
        HANDLE locked = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        Require(locked != nullptr, "mutex fixture event");
        std::thread holder([&] { std::scoped_lock lock{mutex}; SetEvent(locked); Sleep(40); });
        WaitForSingleObject(locked, INFINITE);
        { timing::WindowLock lock{mutex, timing::Operation::WindowMutexBefore}; }
        holder.join(); CloseHandle(locked);
        bool waitRecorded = false;
        timing::Consume([&](const timing::Sample& sample) {
            if (sample.operation == timing::Operation::WindowMutexBefore) waitRecorded = true;
        });
        Require(waitRecorded, "contended mutex wait recorded");
        // Empty queue produces no new diagnostics; logging exceptions are contained.
        unsigned logs = 0;
        timing::Drain([&](const std::string&) { ++logs; });
        Require(logs == 0, "no logging for empty buffer");
        timing::Record(timing::Operation::Commit, 1000, 1000 + threshold);
        timing::Drain([](const std::string&) { throw std::runtime_error("sink"); });
        std::cout << "target_timing_buffer_and_contracts=PASS\n";
#endif
        // Hidden local HWND: exercises actual API acceptance/resources, not display pacing.
        const auto instance = GetModuleHandleW(nullptr);
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = DefWindowProcW;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = L"OverlayAttachmentFixture";
        Require(RegisterClassW(&windowClass) != 0, "register fixture class");
        HWND window = CreateFixtureWindow(instance);
        Require(window != nullptr, "create fixture HWND");
#ifdef OVERLAY_TARGET_RELEASE_SMOKE
        Microsoft::WRL::ComPtr<ID3D11Device> probeD3d;
        Require(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
            nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
            D3D11_SDK_VERSION, probeD3d.GetAddressOf(), nullptr, nullptr)), "probe D3D device");
        Microsoft::WRL::ComPtr<IDXGIDevice> probeDxgi;
        Require(SUCCEEDED(probeD3d.As(&probeDxgi)), "probe DXGI device");
        Require(SUCCEEDED(DCompositionCreateDevice(probeDxgi.Get(),
            __uuidof(IDCompositionDevice), reinterpret_cast<void**>(probeDevice.GetAddressOf()))),
            "probe composition device");
#endif
        Require(plugin::GetRuntimeSettingsApi().Publish({nullptr, nullptr,
            Snapshot, nullptr, Semantic, nullptr, nullptr, nullptr}), "publish fixture settings");
        overlay::Renderer renderer;
        renderer.SetLogger(Log);
        Require(renderer.Initialize(window, 640, 480), "actual Renderer initialization");
        ExpectRoot(renderer, true, "startup_notification_retains_root");
        auto& input = overlay::GetInputState();
        input.Toggle(); renderer.SetVisible(true); renderer.Render();
        input.Toggle(); renderer.SetVisible(false); renderer.Render();
        ExpectRoot(renderer, true, "closed_during_startup_notification");
        Pump(renderer, overlay::StartupHintDurationMs + 100);
#ifdef OVERLAY_ATTACHMENT_PRODUCTION_SMOKE
        constexpr bool hiddenAttached = false;
#else
        constexpr bool hiddenAttached = OVERLAY_ATTACHMENT_EXPERIMENT_DETACH == 0;
#endif
        ExpectRoot(renderer, hiddenAttached, "closed_after_startup_notification");
#ifdef OVERLAY_TARGET_RELEASE_SMOKE
        CheckLayerAvailability(window, true);
#endif
        Require(!renderer.needsRenderWork(), "closed presenter is idle");
#ifdef OVERLAY_ATTACHMENT_PRODUCTION_SMOKE
        const auto before = records.size();
        Pump(renderer, 500);
#ifdef OVERLAY_TARGET_TIMING_SMOKE
        (void)before;
        Require(!renderer.needsRenderWork(), "timed renderer remains settled");
#else
        Require(!renderer.needsRenderWork() && records.size() == before,
            "closed production renderer remains settled");
#endif
#else
        renderer.LogAttachmentExperimentSnapshot("idle_before_wait");
        const auto before = records.back().substr(records.back().find("render_calls="));
        Pump(renderer, 500);
        renderer.LogAttachmentExperimentSnapshot("toggle_before");
        Require(before == records.back().substr(records.back().find("render_calls=")),
            "closed idle interval must add zero render/draw/Commit work");
#endif
        for (int index = 0; index < 3; ++index) {
            input.Toggle(); renderer.SetVisible(true);
            Require(renderer.Render(), "open frame draw");
            ExpectRoot(renderer, true, "open_draw_attaches");
#ifdef OVERLAY_TARGET_RELEASE_SMOKE
            CheckLayerAvailability(window, false);
#endif
            input.Toggle(); renderer.SetVisible(false);
            Require(renderer.Render(), "close frame clear");
            ExpectRoot(renderer, hiddenAttached, "closed_clear_completed");
#ifdef OVERLAY_TARGET_RELEASE_SMOKE
            CheckLayerAvailability(window, true);
#endif
        }
        Require(renderer.OnWindowGeometry(window, 800, 600, false), "closed resize transaction");
        ExpectRoot(renderer, hiddenAttached, "closed_resize_root");
        Require(renderer.OnWindowGeometry(window, 800, 600, true), "minimize");
        Require(renderer.OnWindowGeometry(window, 800, 600, false), "restore");
        ExpectRoot(renderer, hiddenAttached, "closed_restore_root");
        input.Toggle(); renderer.SetVisible(true); Require(renderer.Render(), "open before minimize");
        Require(renderer.OnWindowGeometry(window, 800, 600, true), "visible minimize");
        Require(renderer.OnWindowGeometry(window, 800, 600, false), "visible restore");
        ExpectRoot(renderer, true, "visible_restore_root");
        input.Toggle(); renderer.SetVisible(false); Require(renderer.Render(), "close after restore");
        ExpectRoot(renderer, hiddenAttached, "closed_after_visible_restore");
        plugin::PublishOverlayNotification(plugin::OverlayNotificationKind::SettingChanged,
            plugin::OverlayNotificationAction::AnotherSetting, "A", "B",
            plugin::OverlayNotificationStatus::Ready, 80);
        renderer.RequestRedraw(); Require(renderer.Render(), "notification draw");
        ExpectRoot(renderer, true, "hidden_notification_attaches");
        Pump(renderer, 160);
        ExpectRoot(renderer, hiddenAttached, "notification_expiry_root");
        HWND replacement = CreateFixtureWindow(instance);
        Require(replacement != nullptr && renderer.RebindWindow(replacement, 640, 480), "HWND rebind");
        ExpectRoot(renderer, hiddenAttached, "closed_rebind_root");
        input.Toggle(); renderer.SetVisible(true); Require(renderer.Render(), "open before rebind");
        HWND visibleReplacement = CreateFixtureWindow(instance);
        Require(visibleReplacement != nullptr && renderer.RebindWindow(visibleReplacement, 640, 480),
            "visible HWND rebind");
        ExpectRoot(renderer, true, "visible_rebind_root");
        input.Toggle(); renderer.SetVisible(false); Require(renderer.Render(), "close after visible rebind");
        ExpectRoot(renderer, hiddenAttached, "closed_after_visible_rebind");
#ifdef OVERLAY_TARGET_RELEASE_SMOKE
        CheckLayerAvailability(visibleReplacement, true);
        // Exercise successful FALSE fallback without altering the Renderer.
        Microsoft::WRL::ComPtr<IDCompositionTarget> foreignTop, foreignBottom;
        Require(SUCCEEDED(probeDevice->CreateTargetForHwnd(visibleReplacement, TRUE,
            foreignTop.GetAddressOf())), "occupy top layer");
        input.Toggle(); renderer.SetVisible(true);
        Require(renderer.Render() && !renderer.disabled(), "FALSE fallback reopen");
        ExpectRoot(renderer, true, "fallback root attached");
        input.Toggle(); renderer.SetVisible(false); Require(renderer.Render(), "fallback close");
        Require(SUCCEEDED(probeDevice->CreateTargetForHwnd(visibleReplacement, FALSE,
            foreignBottom.GetAddressOf())), "fallback close released bottom layer");
        // Both layers held externally: no partial open or retries.
        input.Toggle(); renderer.SetVisible(true);
        Require(!renderer.Render() && renderer.disabled(), "occupied layers fail closed");
        Require(!renderer.Render(), "disabled Renderer cannot retry");
        foreignBottom.Reset(); foreignTop.Reset();
        CheckLayerAvailability(visibleReplacement, true);
#endif
        renderer.Shutdown();
#ifdef OVERLAY_TARGET_TIMING_SMOKE
        timing::Drain(Log);
#endif
        DestroyWindow(visibleReplacement); DestroyWindow(replacement); DestroyWindow(window);
#ifdef OVERLAY_TARGET_RELEASE_SMOKE
        std::cout << "actual_renderer_hidden_hwnd_smoke=PASS variant=target-release\n";
#elif defined(OVERLAY_ATTACHMENT_PRODUCTION_SMOKE)
        std::cout << "actual_renderer_hidden_hwnd_smoke=PASS variant=production\n";
#else
        std::cout << "actual_renderer_hidden_hwnd_smoke=PASS variant="
            << (OVERLAY_ATTACHMENT_EXPERIMENT_DETACH ? "C" : "B") << '\n';
#endif
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "actual_renderer_hidden_hwnd_smoke=FAIL " << e.what() << '\n';
        return 1;
    }
}
