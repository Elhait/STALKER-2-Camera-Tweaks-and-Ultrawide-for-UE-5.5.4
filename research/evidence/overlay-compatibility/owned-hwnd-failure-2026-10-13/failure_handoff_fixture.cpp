#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dcomp.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

using Microsoft::WRL::ComPtr;
namespace {
constexpr UINT kHintMessage = WM_APP + 0x6A1;
constexpr UINT64 kTerminal = UINT64{1} << 63;
constexpr UINT64 kCountMask = ~kTerminal;
constexpr DWORD kTimeoutMs = 5000;
enum class Outcome : LONG { Pending, Complete, Retained };

struct Context {
    std::atomic<UINT64> admission{};
    HANDLE terminalEvent{}; // process-resident; never closed/reused in this process
    HANDLE drainEvent{};
    HANDLE queueReady{};
    HANDLE allowStartup{};
    HANDLE ownerReady{};
    HANDLE ownerDone{};
    HANDLE resourcesReady{};
    HANDLE rendererHeld{};
    HANDLE releaseRenderer{};
    HANDLE callbackAdmitted{};
    HANDLE finalizerEntered{};
    HANDLE allowFinalizer{};
    std::mutex rendererMutex;
    std::atomic<LONG> ownerThread{};
    std::atomic<LONG> outcome{static_cast<LONG>(Outcome::Pending)};
    std::atomic<LONG> resourcesCreated{};
    std::atomic<LONG> resourcesReleased{};
    std::atomic<LONG> resourcesRetained{};
    std::atomic<LONG> cleanupCount{};
    std::atomic<LONG> overlayMutations{};
    std::atomic<LONG> continuations{};
    std::atomic<LONG> staleCallbacks{};
    std::atomic<LONG> callbackReturned{};
    std::atomic<LONG> secondaryExceptions{};
    HWND ownerHwnd{};
    HANDLE testResource{};
    ID3D11Device* d3d{};
    ID3D11DeviceContext* context{};
    IDCompositionDevice* dcomp{};
    IDCompositionTarget* target{};
    std::thread joinableOwner;
    int createLimit{3};
    bool failStartup{};
    bool failHint{};
    bool saturateQueue{};
    bool race{};
    bool holdFinalizer{};
    bool failTestResourceRelease{};
    UINT64 currentBinding{7};
} g;

void Check(bool value, const char* what) {
    if (!value) throw std::runtime_error(what);
}
void Wait(HANDLE event, const char* what) {
    Check(WaitForSingleObject(event, kTimeoutMs) == WAIT_OBJECT_0, what);
}
LRESULT CALLBACK ResourceWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return DefWindowProcW(hwnd, msg, wp, lp);
}
struct Admission {
    bool held{};
    Admission() = default;
    Admission(const Admission&) = delete;
    Admission& operator=(const Admission&) = delete;
    ~Admission() {
        if (!held) return;
        const UINT64 before = g.admission.fetch_sub(1, std::memory_order_acq_rel);
        if ((before & kCountMask) == 1) SetEvent(g.drainEvent);
    }
};
bool Enter(Admission& admission) noexcept {
    UINT64 state = g.admission.load(std::memory_order_acquire);
    for (;;) {
        if (state & kTerminal) return false;
        const UINT64 count = state & kCountMask;
        if (count == kCountMask) return false;
        if (count == 0) ResetEvent(g.drainEvent);
        if (g.admission.compare_exchange_weak(state, state + 1,
                std::memory_order_acq_rel, std::memory_order_acquire)) {
            admission.held = true;
            return true;
        }
        if ((state & kTerminal) && (state & kCountMask) == 0)
            SetEvent(g.drainEvent);
    }
}
void LatchTerminal(bool postHint) noexcept {
    const UINT64 before = g.admission.fetch_or(kTerminal, std::memory_order_acq_rel);
    SetEvent(g.terminalEvent);
    if (!(before & kTerminal) && postHint) {
        const DWORD tid = static_cast<DWORD>(g.ownerThread.load(std::memory_order_acquire));
        if (tid) {
            SetLastError(ERROR_SUCCESS);
            const BOOL posted = PostThreadMessageW(tid, kHintMessage, 0, 0);
            std::printf("HINT posted=%d error=%lu\n", posted, GetLastError());
        } else {
            std::printf("HINT skipped=no_owner_thread\n");
        }
    }
}
void NativeContinuation() noexcept { ++g.continuations; }

// Models the game WndProc's optional-work boundary. Native continuation is
// outside the contained work. The failure callback never takes rendererMutex.
void GameCallback(UINT64 binding, bool throwFailure, bool waitForRenderer) noexcept {
    if (binding != g.currentBinding) {
        ++g.staleCallbacks;
        NativeContinuation();
        return;
    }
    Admission ticket;
    if (!Enter(ticket)) { NativeContinuation(); return; }
    SetEvent(g.callbackAdmitted);
    bool failed = false;
    try {
        if (throwFailure) throw std::runtime_error("injected WndProc exception");
        if (waitForRenderer) {
            std::lock_guard lock(g.rendererMutex);
        }
    } catch (...) {
        failed = true;
        LatchTerminal(false);
    }
    if (failed) NativeContinuation();
    else if (!(g.admission.load(std::memory_order_acquire) & kTerminal))
        ++g.overlayMutations;
    ++g.callbackReturned;
}

void ThrowIfInjectedReleaseFailure() {
    throw std::runtime_error("injected secondary cleanup exception");
}
void ReleaseResources() noexcept {
    const LONG owner = g.ownerThread.load(std::memory_order_acquire);
    auto onOwner = [owner]() {
        if (owner == static_cast<LONG>(GetCurrentThreadId())) return true;
        ++g.resourcesRetained;
        std::printf("CLEANUP wrong_thread resource_retained=1\n");
        return false;
    };
    auto noteRelease = []() {
        ++g.resourcesReleased;
    };
    if (g.target && onOwner()) { g.target->Release(); g.target = nullptr; noteRelease(); }
    if (g.dcomp && onOwner()) { g.dcomp->Release(); g.dcomp = nullptr; noteRelease(); }
    if (g.context && onOwner()) { g.context->Release(); g.context = nullptr; noteRelease(); }
    if (g.d3d && onOwner()) { g.d3d->Release(); g.d3d = nullptr; noteRelease(); }
    if (g.ownerHwnd && IsWindow(g.ownerHwnd)) {
        if (onOwner() && DestroyWindow(g.ownerHwnd)) { g.ownerHwnd = nullptr; noteRelease(); }
        else ++g.resourcesRetained;
    } else if (g.ownerHwnd) {
        g.ownerHwnd = nullptr;
    }
    if (g.testResource) {
        try {
            Check(owner == static_cast<LONG>(GetCurrentThreadId()),
                "event cleanup left owner thread");
            if (g.failTestResourceRelease) ThrowIfInjectedReleaseFailure();
            Check(CloseHandle(g.testResource) != FALSE, "CloseHandle test resource");
            g.testResource = nullptr;
            noteRelease();
        } catch (...) {
            ++g.secondaryExceptions;
            ++g.resourcesRetained;
            std::printf("CLEANUP secondary_exception resource=event retained=1\n");
        }
    }
    if (g.resourcesRetained.load() == 0 && g.testResource == nullptr &&
        g.ownerHwnd == nullptr && !g.target && !g.dcomp && !g.context && !g.d3d)
        g.outcome = static_cast<LONG>(Outcome::Complete);
    else
        g.outcome = static_cast<LONG>(Outcome::Retained);
}

void Owner() noexcept {
    g.ownerThread = static_cast<LONG>(GetCurrentThreadId());
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    MSG msg{};
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    SetEvent(g.queueReady);
    SetEvent(g.ownerReady);
    HANDLE startEvents[]{g.allowStartup, g.terminalEvent};
    const DWORD start = FAILED(com) ? WAIT_FAILED :
        WaitForMultipleObjects(2, startEvents, FALSE, kTimeoutMs);
    if (start == WAIT_OBJECT_0 && !(g.admission.load() & kTerminal)) {
        try {
            if (g.failStartup) throw std::runtime_error("injected startup failure");
            if (g.createLimit >= 1) {
                g.testResource = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                Check(g.testResource != nullptr, "CreateEvent resource");
                ++g.resourcesCreated;
            }
            if (g.createLimit >= 2) {
                g.ownerHwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"HandoffResourceWindow",
                    L"hidden fixture resource", WS_POPUP, 0, 0, 8, 8, nullptr, nullptr,
                    GetModuleHandleW(nullptr), nullptr);
                Check(g.ownerHwnd != nullptr, "CreateWindow resource");
                ++g.resourcesCreated;
            }
            if (g.createLimit >= 3) {
                D3D_FEATURE_LEVEL level{};
                Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
                    nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                    D3D11_SDK_VERSION, &g.d3d, &level, &g.context)), "D3D11 resource");
                ComPtr<IDXGIDevice> dxgi;
                Check(SUCCEEDED(g.d3d->QueryInterface(IID_PPV_ARGS(&dxgi))), "DXGI device");
                Check(SUCCEEDED(DCompositionCreateDevice(dxgi.Get(), IID_PPV_ARGS(&g.dcomp))),
                    "DComp device");
                Check(SUCCEEDED(g.dcomp->CreateTargetForHwnd(g.ownerHwnd, TRUE, &g.target)),
                    "DComp target on owner HWND");
                g.resourcesCreated += 4; // D3D device/context, DComp device and target
            }
            SetEvent(g.resourcesReady);
            if (g.race) {
                g.rendererMutex.lock();
                SetEvent(g.rendererHeld);
                WaitForSingleObject(g.releaseRenderer, kTimeoutMs);
                g.rendererMutex.unlock();
            }
            if (!g.failHint) {
                while (!(g.admission.load(std::memory_order_acquire) & kTerminal)) {
                    const DWORD wait = MsgWaitForMultipleObjectsEx(1, &g.terminalEvent,
                        INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                    if (wait == WAIT_OBJECT_0) break;
                    if (wait == WAIT_OBJECT_0 + 1) {
                        MSG queued{};
                        while (PeekMessageW(&queued, nullptr, 0, 0, PM_REMOVE)) {
                            if (queued.message == kHintMessage) LatchTerminal(false);
                        }
                    } else break;
                }
            } else {
                WaitForSingleObject(g.terminalEvent, kTimeoutMs);
            }
        } catch (const std::exception& ex) {
            std::printf("OWNER startup/work exception=%s\n", ex.what());
            SetEvent(g.resourcesReady);
            LatchTerminal(false);
        } catch (...) {
            ++g.secondaryExceptions;
            SetEvent(g.resourcesReady);
            LatchTerminal(false);
        }
    } else if (start == WAIT_TIMEOUT || start == WAIT_FAILED) {
        ++g.secondaryExceptions;
        LatchTerminal(false);
    }
    SetEvent(g.resourcesReady);

    // Terminal state is monotonic. Drain prior admissions without owning the
    // lock those callbacks may need. Queue messages cannot postpone the check.
    LatchTerminal(false);
    while ((g.admission.load(std::memory_order_acquire) & kCountMask) != 0) {
        const DWORD wait = MsgWaitForMultipleObjectsEx(1, &g.drainEvent, INFINITE,
            QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (wait == WAIT_OBJECT_0 + 1) {
            MSG queued{};
            while (PeekMessageW(&queued, nullptr, 0, 0, PM_REMOVE))
                ++g.staleCallbacks; // terminal queue work is pass-through only
        }
    }
    ++g.cleanupCount;
    SetEvent(g.finalizerEntered);
    if (g.holdFinalizer) WaitForSingleObject(g.allowFinalizer, kTimeoutMs);
    ReleaseResources();
    if (SUCCEEDED(com)) CoUninitialize();
    g.ownerThread = 0;
    SetEvent(g.ownerDone);
}

void MakeEvent(HANDLE& handle, bool signaled = false) {
    handle = CreateEventW(nullptr, TRUE, signaled, nullptr);
    Check(handle != nullptr, "CreateEvent control");
}
void Start(bool awaitStart = true) {
    MakeEvent(g.terminalEvent);
    MakeEvent(g.drainEvent, true);
    MakeEvent(g.queueReady); MakeEvent(g.allowStartup, !awaitStart);
    MakeEvent(g.ownerReady); MakeEvent(g.ownerDone); MakeEvent(g.resourcesReady);
    MakeEvent(g.rendererHeld);
    MakeEvent(g.releaseRenderer); MakeEvent(g.callbackAdmitted);
    MakeEvent(g.finalizerEntered); MakeEvent(g.allowFinalizer);
    g.joinableOwner = std::thread(Owner);
    Wait(g.ownerReady, "owner ready/queue created");
}
}

// Driver entry point intentionally kept in this translation unit's anonymous
// test namespace. Scenario-specific resets happen in the runner's subprocess.
int RunScenario(const char* mode) {
    WNDCLASSW wc{};
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpfnWndProc = ResourceWindowProc;
    wc.lpszClassName = L"HandoffResourceWindow";
    Check(RegisterClassW(&wc) != 0, "RegisterClass");
    const std::string name(mode);
    g.failStartup = name == "startup";
    g.failHint = name == "race" || name == "queue-full" || name == "stale-token";
    g.race = name == "race";
    g.holdFinalizer = name == "stopping";
    g.failTestResourceRelease = name == "cleanup-failure";
    if (name == "partial-0") g.createLimit = 0;
    if (name == "partial-1") g.createLimit = 1;
    if (name == "partial-2") g.createLimit = 2;
    Start(true);
    if (name == "startup") {
        g.ownerThread = 0; // Model request before owner message-route publication.
        LatchTerminal(true); SetEvent(g.allowStartup);
    } else {
        SetEvent(g.allowStartup);
        Wait(g.resourcesReady, "owner resource initialization boundary");
        if (name == "race") {
            Wait(g.rendererHeld, "owner holds renderer lock");
            std::thread inflight([] { GameCallback(g.currentBinding, false, true); });
            Wait(g.callbackAdmitted, "in-flight callback admitted");
            const ULONGLONG before = GetTickCount64();
            GameCallback(g.currentBinding, true, false);
            const ULONGLONG elapsed = GetTickCount64() - before;
            const bool finalizedEarly =
                WaitForSingleObject(g.finalizerEntered, 0) != WAIT_TIMEOUT;
            SetEvent(g.releaseRenderer);
            inflight.join();
            Check(elapsed < 500, "failure callback waited for owner/renderer");
            Check(!finalizedEarly, "owner finalized before in-flight callback drained");
        } else if (name == "queue-full") {
            unsigned posted = 0;
            while (PostThreadMessageW(static_cast<DWORD>(g.ownerThread), kHintMessage,
                    1, 0)) ++posted;
            const DWORD queueError = GetLastError();
            Check(posted > 0 && queueError == ERROR_NOT_ENOUGH_QUOTA,
                "could not establish real full thread queue");
            LatchTerminal(true);
            std::printf("QUEUE saturated_posts=%u error=%lu\n", posted, queueError);
        } else if (name == "partial-0" || name == "partial-1" || name == "partial-2") {
            LatchTerminal(false);
        } else if (name == "cleanup-failure") {
            LatchTerminal(true);
        } else if (name == "stopping") {
            LatchTerminal(true);
            Wait(g.finalizerEntered, "finalizer entered stopping state");
            LatchTerminal(true);
            SetEvent(g.allowFinalizer);
        } else if (name == "late") {
            LatchTerminal(true);
        } else if (name == "stale-token") {
            LatchTerminal(false);
            GameCallback(g.currentBinding - 1, false, true);
        } else {
            throw std::runtime_error("unknown scenario");
        }
    }
    Wait(g.ownerDone, "owner finalization deadline");
    g.joinableOwner.join();
    if (name == "late") {
        const LONG mutations = g.overlayMutations.load();
        GameCallback(g.currentBinding, false, false);
        LatchTerminal(true); // late, idempotent request after owner has stopped
        Check(g.overlayMutations == mutations && g.continuations == 1,
            "late callback resurrected owner instead of continuing natively");
        Check(g.cleanupCount == 1 && g.outcome == static_cast<LONG>(Outcome::Complete),
            "late terminal request duplicated cleanup or changed completed outcome");
    }
    if (name == "stale-token")
        Check(g.staleCallbacks == 1 && g.continuations == 1 && g.overlayMutations == 0,
            "stale binding touched Overlay state");
    Check(g.cleanupCount == 1, "cleanup not exactly once");
    Check(g.ownerThread == 0, "owner identity not cleared after finalizer");
    const bool retained = g.resourcesRetained.load() != 0;
    Check((g.outcome.load() == static_cast<LONG>(Outcome::Retained)) == retained,
        "cleanup outcome does not reflect retained resources");
    if (name == "cleanup-failure") {
        DWORD liveFlags{};
        const BOOL handleStillLive = g.testResource &&
            GetHandleInformation(g.testResource, &liveFlags);
        std::printf("CHECK cleanup_failure retained=%ld secondary=%ld released=%ld created=%ld\n",
            g.resourcesRetained.load(), g.secondaryExceptions.load(),
            g.resourcesReleased.load(), g.resourcesCreated.load());
        Check(handleStillLive && retained && g.secondaryExceptions == 1 &&
            g.resourcesReleased > 0,
            "secondary cleanup exception was not contained/recorded");
    } else {
        Check(!retained && g.outcome == static_cast<LONG>(Outcome::Complete),
            "Complete published with retained resources");
        Check(g.resourcesCreated == g.resourcesReleased,
            "successful cleanup count differs from creation count");
    }
    if (name == "race")
        Check(g.overlayMutations == 0 && g.continuations == 1 &&
            (g.admission.load() & kCountMask) == 0, "race gate invariant failed");
    std::printf("RESULT mode=%s outcome=%s created=%ld released=%ld retained=%ld "
        "cleanup_count=%ld mutations=%ld continuations=%ld secondary=%ld PASS\n",
        mode, retained ? "Retained" : "Complete", g.resourcesCreated.load(),
        g.resourcesReleased.load(), g.resourcesRetained.load(), g.cleanupCount.load(),
        g.overlayMutations.load(), g.continuations.load(), g.secondaryExceptions.load());
    HANDLE controls[]{g.terminalEvent, g.drainEvent, g.queueReady, g.allowStartup,
        g.ownerReady, g.ownerDone, g.resourcesReady, g.rendererHeld, g.releaseRenderer,
        g.callbackAdmitted, g.finalizerEntered, g.allowFinalizer};
    for (HANDLE handle : controls) if (handle) CloseHandle(handle);
    return 0;
}

int wmain(int argc, wchar_t** argv) {
    try {
        setvbuf(stdout, nullptr, _IONBF, 0);
        Check(argc == 2, "usage: failure-handoff-fixture <scenario>");
        char mode[64]{};
        const int converted = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1,
            mode, sizeof(mode), nullptr, nullptr);
        Check(converted > 0, "scenario encoding");
        const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        Check(SUCCEEDED(com), "main CoInitializeEx");
        const int code = RunScenario(mode);
        CoUninitialize();
        return code;
    } catch (const std::exception& error) {
        std::printf("FAIL %s error=%lu\n", error.what(), GetLastError());
        SetEvent(g.allowStartup);
        SetEvent(g.releaseRenderer);
        SetEvent(g.allowFinalizer);
        LatchTerminal(false);
        if (g.joinableOwner.joinable()) {
            if (WaitForSingleObject(g.ownerDone, kTimeoutMs) == WAIT_OBJECT_0)
                g.joinableOwner.join();
            else
                TerminateProcess(GetCurrentProcess(), 1);
        }
        return 1;
    } catch (...) {
        std::puts("FAIL unknown exception");
        SetEvent(g.allowStartup);
        SetEvent(g.releaseRenderer);
        SetEvent(g.allowFinalizer);
        LatchTerminal(false);
        if (g.joinableOwner.joinable()) {
            if (WaitForSingleObject(g.ownerDone, kTimeoutMs) == WAIT_OBJECT_0)
                g.joinableOwner.join();
            else
                TerminateProcess(GetCurrentProcess(), 1);
        }
        return 1;
    }
}
