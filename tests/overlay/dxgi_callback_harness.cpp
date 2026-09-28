// Exercise the actual discovery callbacks without renderer/game initialization.
#define OVERLAY_COMBINED
#include "../../src/overlay/discovery_runtime.cpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace
{
    struct FakeCom { void** table; };
    std::atomic<unsigned> nativeCalls{};
    bool argsCorrect{};
    UINT presentFlags{};
    unsigned identityQueries{};
    IUnknown* const expectedDevice = reinterpret_cast<IUnknown*>(0x1000);
    IUnknown* const expectedWindow = reinterpret_cast<IUnknown*>(0x2000);
    IDXGIOutput* const expectedOutput = reinterpret_cast<IDXGIOutput*>(0x3000);
    DXGI_SWAP_CHAIN_DESC1 expectedDesc{};
    DXGI_PRESENT_PARAMETERS expectedPresentParameters{};
    UINT present1Interval{};
    UINT present1Flags{};
    const DXGI_PRESENT_PARAMETERS* actualPresentParameters{};
    IDXGISwapChain1* resultStorage{};
    const UINT nodeMasks[]{1, 2};
    IUnknown* queues[]{expectedDevice, expectedWindow};
    HRESULT nativeResizeResult{E_ACCESSDENIED};
    HRESULT nativeResize1Result{E_ACCESSDENIED};
    HRESULT nativePresentResult{DXGI_ERROR_WAS_STILL_DRAWING};
    bool resize1CallsNestedResize{};
    std::array<void*, 32> failCaptureTable{};
    std::array<void*, 32> passCaptureTable{};
    FakeCom failCaptureApplication{};
    FakeCom failCaptureSeed{};
    FakeCom passCaptureApplication{};
    FakeCom passCaptureSeed{};
    FakeCom* captureFixtureApplication{};
    CreateFactoryFn capturedForeignOriginal{};
    unsigned observedExportCallbacks{};

    HRESULT STDMETHODCALLTYPE Noop(IUnknown*) { return S_OK; }
    __declspec(noinline) HRESULT WINAPI NativeFactory(REFIID, void** result)
    { ++nativeCalls; *result = nullptr; return E_ACCESSDENIED; }
    HRESULT WINAPI CaptureFixtureNativeFactory(REFIID, void** result)
    {
        ++nativeCalls;
        *result = captureFixtureApplication;
        return S_OK;
    }
    HRESULT WINAPI CaptureFixtureOurFactoryHook(REFIID iid, void** result)
    {
        ++observedExportCallbacks;
        const auto status = CaptureFixtureNativeFactory(iid, result);
        ObserveFactoryResult(status, iid, result, "capture_order_fixture", nullptr);
        return status;
    }
    HRESULT WINAPI CaptureFixtureForeignWrapper(REFIID iid, void** result)
    {
        return capturedForeignOriginal
            ? capturedForeignOriginal(iid, result) : E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE NativeHwnd(IDXGIFactory2*, IUnknown* device, HWND window,
        const DXGI_SWAP_CHAIN_DESC1* desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen,
        IDXGIOutput* output, IDXGISwapChain1** result)
    {
        ++nativeCalls;
        argsCorrect = device == expectedDevice && window == reinterpret_cast<HWND>(expectedWindow) &&
            desc == &expectedDesc && fullscreen == nullptr && output == expectedOutput &&
            result == &resultStorage;
        return E_ACCESSDENIED;
    }
    HRESULT STDMETHODCALLTYPE Query(IUnknown* self, REFIID, void** result)
    { ++identityQueries; *result = self; return S_OK; }
    HRESULT STDMETHODCALLTYPE NativePresent(IDXGISwapChain*, UINT interval, UINT flags)
    { ++nativeCalls; argsCorrect = interval == 2 && flags == presentFlags; return nativePresentResult; }
    HRESULT STDMETHODCALLTYPE NativePresent1(IDXGISwapChain1*, UINT interval, UINT flags,
        const DXGI_PRESENT_PARAMETERS* parameters)
    {
        ++nativeCalls;
        present1Interval = interval;
        present1Flags = flags;
        actualPresentParameters = parameters;
        return E_ABORT;
    }
    ULONG STDMETHODCALLTYPE Ref(IUnknown*) { return 1; }
    HRESULT STDMETHODCALLTYPE NativeCoreWindow(IDXGIFactory2*, IUnknown* device,
        IUnknown* window, const DXGI_SWAP_CHAIN_DESC1* desc, IDXGIOutput* output,
        IDXGISwapChain1** result)
    {
        ++nativeCalls;
        argsCorrect = device == expectedDevice && window == expectedWindow &&
            desc == &expectedDesc && output == expectedOutput && result == &resultStorage;
        return E_ACCESSDENIED;
    }
    HRESULT STDMETHODCALLTYPE NativeResize(IDXGISwapChain*, UINT count,
        UINT width, UINT height, DXGI_FORMAT format, UINT flags)
    {
        ++nativeCalls;
        argsCorrect = count == 2 && width == 900 && height == 600 &&
            format == DXGI_FORMAT_R8G8B8A8_UNORM && flags == 7;
        return nativeResizeResult;
    }
    HRESULT STDMETHODCALLTYPE NativeResize1(IDXGISwapChain3* self, UINT count,
        UINT width, UINT height, DXGI_FORMAT format, UINT flags,
        const UINT* nodes, IUnknown* const* presentQueues)
    {
        ++nativeCalls;
        argsCorrect = count == 2 && width == 900 && height == 600 &&
            format == DXGI_FORMAT_R8G8B8A8_UNORM && flags == 7 &&
            nodes == nodeMasks && presentQueues == queues;
        if (resize1CallsNestedResize)
            (void)HookResizeBuffers(reinterpret_cast<IDXGISwapChain*>(self), count,
                width, height, format, flags);
        return nativeResize1Result;
    }
    bool Check(bool value, const char* name)
    { std::cout << name << ": " << (value ? "PASS" : "FAIL") << '\n'; return value; }

    template <std::size_t Size> std::array<void*, Size> Table()
    {
        std::array<void*, Size> table;
        table.fill(reinterpret_cast<void*>(&Noop));
        table[0] = reinterpret_cast<void*>(&Query);
        table[1] = table[2] = reinterpret_cast<void*>(&Ref);
        return table;
    }
}

int main()
{
    bool pass = true;
    pass &= Check(!overlay::HasFactoryObservationCoverage(false),
        "export_hook_success_without_shared_table_is_not_ready");
    pass &= Check(overlay::HasFactoryObservationCoverage(true),
        "validated_shared_factory_table_establishes_observation_readiness");
    pass &= Check(overlay::IsExactSystemDxgiPath(
        L"C:/Windows/System32/dxgi.dll", L"c:\\windows\\system32\\dxgi.dll"),
        "canonical_system_dxgi_path_matches_case_and_separator_variants");
    pass &= Check(!overlay::IsExactSystemDxgiPath(
        L"E:\\Game\\Binaries\\Win64\\dxgi.dll",
        L"C:\\Windows\\System32\\dxgi.dll"),
        "same_basename_wrapper_is_not_system_dxgi");

    failCaptureTable = Table<32>();
    failCaptureTable[15] = reinterpret_cast<void*>(&NativeHwnd);
    failCaptureApplication.table = failCaptureTable.data();
    failCaptureSeed.table = failCaptureTable.data();
    captureFixtureApplication = &failCaptureApplication;
    capturedForeignOriginal = &CaptureFixtureNativeFactory; // foreign manager captured before our hook
    void* failCaptureOutput{};
    observedExportCallbacks = 0;
    pass &= Check(CaptureFixtureForeignWrapper(__uuidof(IDXGIFactory2),
        &failCaptureOutput) == S_OK && failCaptureOutput == &failCaptureApplication &&
        observedExportCallbacks == 0,
        "fail_order_foreign_original_bypasses_export_callback");
    pass &= Check(InstallBootstrapFactoryTable(
        reinterpret_cast<IUnknown*>(&failCaptureSeed)),
        "fail_order_seed_installs_pinned_shared_factory_table");
    nativeCalls = 0;
    auto failOrderCreate = reinterpret_cast<CreateSwapChainForHwndFn>(
        failCaptureApplication.table[15]);
    pass &= Check(failOrderCreate(reinterpret_cast<IDXGIFactory2*>(&failCaptureApplication),
        expectedDevice, reinterpret_cast<HWND>(expectedWindow), &expectedDesc,
        nullptr, expectedOutput, &resultStorage) == E_ACCESSDENIED &&
        nativeCalls == 1 && argsCorrect,
        "export_bypass_existing_factory_observed_through_shared_table");

    auto privateSeedTable = Table<32>();
    FakeCom privateSeed{privateSeedTable.data()};
    pass &= Check(!InstallBootstrapFactoryTable(
        reinterpret_cast<IUnknown*>(&privateSeed)),
        "bootstrap_rejects_non_image_factory_table_without_patch");

    passCaptureTable = Table<32>();
    passCaptureTable[15] = reinterpret_cast<void*>(&NativeHwnd);
    passCaptureApplication.table = passCaptureTable.data();
    passCaptureSeed.table = passCaptureTable.data();
    captureFixtureApplication = &passCaptureApplication;
    capturedForeignOriginal = &CaptureFixtureOurFactoryHook; // foreign manager captured after our hook
    void* passCaptureOutput{};
    observedExportCallbacks = 0;
    pass &= Check(CaptureFixtureForeignWrapper(__uuidof(IDXGIFactory2),
        &passCaptureOutput) == S_OK && passCaptureOutput == &passCaptureApplication &&
        observedExportCallbacks == 1 && FindFactory(&passCaptureApplication),
        "pass_order_foreign_original_preserves_export_callback");
    pass &= Check(InstallBootstrapFactoryTable(
        reinterpret_cast<IUnknown*>(&passCaptureSeed)),
        "pass_order_bootstrap_accepts_already_observed_shared_table");
    auto passOrderCreate = reinterpret_cast<CreateSwapChainForHwndFn>(
        passCaptureApplication.table[15]);
    nativeCalls = 0;
    pass &= Check(passOrderCreate(reinterpret_cast<IDXGIFactory2*>(&passCaptureApplication),
        expectedDevice, reinterpret_cast<HWND>(expectedWindow), &expectedDesc,
        nullptr, expectedOutput, &resultStorage) == E_ACCESSDENIED && nativeCalls == 1,
        "pass_order_shared_table_preserves_native_result");

    {
        overlay::FactoryCreationScope outer;
        pass &= Check(!overlay::FactoryCreationScope::AlreadyObserved(0x1110) &&
            !overlay::FactoryCreationScope::AlreadyObserved(0x2220),
            "nested_creation_tracks_distinct_proxy_and_native_identities");
        {
            overlay::FactoryCreationScope inner;
            pass &= Check(overlay::FactoryCreationScope::AlreadyObserved(0x1110) &&
                !overlay::FactoryCreationScope::AlreadyObserved(0x3330),
                "nested_proxy_native_same_identity_is_deduplicated");
        }
        pass &= Check(overlay::FactoryCreationScope::AlreadyObserved(0x3330),
            "nested_observation_identity_survives_until_outer_return");
    }
    auto factoryTable = Table<32>(); // complete Factory7 extent and private-tail sentinel
    factoryTable[16] = reinterpret_cast<void*>(&NativeCoreWindow);
    FakeCom factory{factoryTable.data()};
    InstallFactoryHooks(reinterpret_cast<IUnknown*>(&factory), 25, "IDXGIFactory2");
    auto core = reinterpret_cast<CreateSwapChainForCoreWindowFn>(factory.table[16]);
    nativeCalls = 0;
    pass &= Check(core(reinterpret_cast<IDXGIFactory2*>(&factory), expectedDevice,
        expectedWindow, &expectedDesc, expectedOutput, &resultStorage) == E_ACCESSDENIED &&
        argsCorrect && nativeCalls == 1, "CoreWindow_all_arguments_and_native_result");
    pass &= Check(factory.table == factoryTable.data() &&
        factory.table[26] == reinterpret_cast<void*>(&Noop), "private_vtable_tail_preserved");
    overlay::DxgiHookInstallStatus duplicateFactoryStatus{};
    pass &= Check(!g_factoryHooks.Install(&factory, 25,
        [](overlay::DxgiHookRecord&) {}, &duplicateFactoryStatus) &&
        duplicateFactoryStatus == overlay::DxgiHookInstallStatus::DuplicateVtable,
        "factory_hook_failure_reports_duplicate_vtable");
    auto baseFirstTable = Table<32>();
    baseFirstTable[15] = reinterpret_cast<void*>(&NativeHwnd);
    FakeCom baseFirstFactory{baseFirstTable.data()};
    void* baseFirstOutput = &baseFirstFactory;
    ObserveFactoryResult(S_OK, __uuidof(IDXGIFactory1), &baseFirstOutput,
        "base_first_contract", nullptr);
    nativeCalls = 0;
    auto hwndFactoryMethod = reinterpret_cast<CreateSwapChainForHwndFn>(
        baseFirstFactory.table[15]);
    pass &= Check(hwndFactoryMethod == &HookCreateSwapChainForHwnd &&
        FindFactory(&baseFirstFactory)->original.size() == 32 &&
        hwndFactoryMethod(reinterpret_cast<IDXGIFactory2*>(&baseFirstFactory),
            expectedDevice, reinterpret_cast<HWND>(expectedWindow), &expectedDesc, nullptr, nullptr,
            &resultStorage) == E_ACCESSDENIED && nativeCalls == 1,
        "base_requested_factory_publishes_modern_shared_table_and_forwards_native");
    pass &= Check(FindFactory(&factory)->original.size() == 25 &&
        factory.table[15] == reinterpret_cast<void*>(&HookCreateSwapChainForHwnd) &&
        factory.table[16] == reinterpret_cast<void*>(&HookCreateSwapChainForCoreWindow) &&
        factory.table[24] == reinterpret_cast<void*>(&HookCreateSwapChainForComposition) &&
        factory.table[26] == reinterpret_cast<void*>(&Noop),
        "modern_first_factory_patches_only_supported_slots_and_preserves_private_tail");
    g_log.clear();
    g_log.exceptions(std::ios::badbit);
    nativeCalls = 0;
    pass &= Check(core(reinterpret_cast<IDXGIFactory2*>(&factory), expectedDevice,
        expectedWindow, &expectedDesc, expectedOutput, &resultStorage) == E_ACCESSDENIED &&
        nativeCalls == 1, "optional_logging_exception_preserves_native_result");
    g_log.exceptions(std::ios::goodbit);
    g_log.clear();

    auto exportHook = safetyhook::InlineHook::create(&NativeFactory,
        &HookCreateFactory, safetyhook::InlineHook::StartDisabled);
    pass &= Check(exportHook.has_value(), "disabled_export_hook_prepared");
    if (exportHook) {
        CreateFactoryFn volatile entry = &NativeFactory;
        void* output{};
        nativeCalls = 0;
        pass &= Check(entry(__uuidof(IDXGIFactory), &output) == E_ACCESSDENIED &&
            nativeCalls == 1 && !g_createFactory.original<CreateFactoryFn>(),
            "disabled_hook_does_not_dispatch_before_owner_publication");
        g_createFactory = std::move(*exportHook);
        pass &= Check(g_createFactory.enable().has_value(), "published_export_hook_enabled");
        nativeCalls = 0;
        pass &= Check(entry(__uuidof(IDXGIFactory), &output) == E_ACCESSDENIED &&
            nativeCalls == 1, "active_export_callback_has_native_original");
        g_createFactory.reset();
    }

    auto swapTable = Table<42>();
    swapTable[13] = reinterpret_cast<void*>(&NativeResize);
    swapTable[22] = reinterpret_cast<void*>(&NativePresent1);
    swapTable[39] = reinterpret_cast<void*>(&NativeResize1);
    FakeCom swapchain{swapTable.data()};
    g_swapchainHooks.Install(&swapchain, 40, [](overlay::DxgiHookRecord& record) {
        record.replacement[13] = reinterpret_cast<void*>(&HookResizeBuffers);
        record.replacement[22] = reinterpret_cast<void*>(&HookPresent1);
        record.replacement[39] = reinterpret_cast<void*>(&HookResizeBuffers1);
    });
    nativeCalls = 0;
    present1Interval = 0;
    present1Flags = 0;
    actualPresentParameters = nullptr;
    auto present1 = reinterpret_cast<Present1Fn>(swapchain.table[22]);
    pass &= Check(present1(reinterpret_cast<IDXGISwapChain1*>(&swapchain), 0, 0x200,
        &expectedPresentParameters) == E_ABORT && nativeCalls == 1 &&
        present1Interval == 0 && present1Flags == 0x200 &&
        actualPresentParameters == &expectedPresentParameters,
        "Present1_forwards_all_arguments_and_original_failure");
    nativeCalls = 0;
    pass &= Check(HookResizeBuffers(reinterpret_cast<IDXGISwapChain*>(&swapchain),
        2, 900, 600, DXGI_FORMAT_R8G8B8A8_UNORM, 7) == E_ACCESSDENIED &&
        argsCorrect && nativeCalls == 1, "ResizeBuffers_native_pass_through");
    nativeCalls = 0;
    pass &= Check(HookResizeBuffers1(reinterpret_cast<IDXGISwapChain3*>(&swapchain),
        2, 900, 600, DXGI_FORMAT_R8G8B8A8_UNORM, 7, nodeMasks, queues) == E_ACCESSDENIED &&
        argsCorrect && nativeCalls == 1, "ResizeBuffers1_all_arguments_and_native_result");

    const auto nestedResizeIdentity = reinterpret_cast<std::uintptr_t>(&swapchain);
    g_evidence.BeginSwapchain(nestedResizeIdentity);
    pass &= Check(g_evidence.ObserveCandidate(nestedResizeIdentity, 0x1010, 0x2020, 0),
        "nested_resize_fixture_starts_with_valid_queue_evidence");
    resize1CallsNestedResize = true;
    nativeResizeResult = E_ACCESSDENIED;
    nativeResize1Result = S_OK;
    nativePresentResult = S_OK;
    auto resize1Callback = reinterpret_cast<ResizeBuffers1Fn>(swapchain.table[39]);
    nativeCalls = 0;
    const HRESULT recoveredResize = resize1Callback(
        reinterpret_cast<IDXGISwapChain3*>(&swapchain), 2, 900, 600,
        DXGI_FORMAT_R8G8B8A8_UNORM, 7, nodeMasks, queues);
    pass &= Check(recoveredResize == S_OK && nativeCalls == 2,
        "nested_resize_preserves_outer_native_success_and_calls_each_native_method_once");
    presentFlags = 0;
    pass &= Check(HookPresent(reinterpret_cast<IDXGISwapChain*>(&swapchain), 2, 0) == S_OK &&
        HookPresent(reinterpret_cast<IDXGISwapChain*>(&swapchain), 2, 0) == S_OK &&
        g_evidence.CandidateCount(nestedResizeIdentity) == 1 &&
        g_evidence.HasStableAssociation(nestedResizeIdentity, 2),
        "recovered_inner_failure_retains_candidate_until_outer_success_and_revalidation");

    g_evidence.BeginSwapchain(nestedResizeIdentity);
    g_evidence.ObserveCandidate(nestedResizeIdentity, 0x1010, 0x2020, 0);
    nativeResizeResult = S_OK;
    nativeResize1Result = E_ACCESSDENIED;
    const HRESULT failedOuterResize = resize1Callback(
        reinterpret_cast<IDXGISwapChain3*>(&swapchain), 2, 900, 600,
        DXGI_FORMAT_R8G8B8A8_UNORM, 7, nodeMasks, queues);
    pass &= Check(failedOuterResize == E_ACCESSDENIED &&
        !g_evidence.HasSufficientAssociation(nestedResizeIdentity),
        "outer_resize_failure_invalidates_evidence_after_nested_success");
    resize1CallsNestedResize = false;
    nativeResizeResult = E_ACCESSDENIED;
    nativeResize1Result = E_ACCESSDENIED;
    nativePresentResult = DXGI_ERROR_WAS_STILL_DRAWING;

    auto probeTable = Table<18>();
    probeTable[8] = reinterpret_cast<void*>(&NativePresent);
    FakeCom probe{probeTable.data()};
    g_swapchainHooks.Install(&probe, 18, [](overlay::DxgiHookRecord& record) {
        record.replacement[8] = reinterpret_cast<void*>(&HookPresent);
    });
    const auto probeIdentity = reinterpret_cast<std::uintptr_t>(&probe);
    g_evidence.ObserveCandidate(probeIdentity, 1, 2, 0);
    for (UINT flags : {DXGI_PRESENT_TEST, DXGI_PRESENT_DO_NOT_WAIT}) {
        nativeCalls = 0; identityQueries = 0; presentFlags = flags;
        pass &= Check(HookPresent(reinterpret_cast<IDXGISwapChain*>(&probe), 2, flags) == DXGI_ERROR_WAS_STILL_DRAWING &&
            nativeCalls == 1 && argsCorrect && !identityQueries && !g_evidence.HasSufficientAssociation(probeIdentity),
            "present_probe_nonblocking_skip_all_overlay_work_and_preserve_native_result");
    }
    auto freshTable = Table<18>();
    FakeCom fresh{freshTable.data()};
    try {
        g_swapchainHooks.Install(&fresh, 18, [](overlay::DxgiHookRecord&) {
            throw std::runtime_error("fixture");
        });
    } catch (const std::runtime_error&) {}
    pass &= Check(!FindSwapchain(&fresh) && fresh.table[8] == reinterpret_cast<void*>(&Noop),
        "failed_preparation_does_not_activate");
    overlay::DxgiHookInstallStatus invalidArgumentStatus{};
    pass &= Check(!g_swapchainHooks.Install(nullptr, 18, [](auto&) {},
        &invalidArgumentStatus) &&
        invalidArgumentStatus == overlay::DxgiHookInstallStatus::InvalidArgument,
        "null_object_rejected_with_failure_reason");
    auto invalidTable = Table<18>(); invalidTable[13] = nullptr;
    FakeCom invalid{invalidTable.data()};
    pass &= Check(!g_swapchainHooks.Install(&invalid, 18, [](auto&) {}), "null_method_rejected");
    auto readOnly = static_cast<void**>(VirtualAlloc(nullptr, 4096,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    pass &= Check(readOnly != nullptr, "read_only_table_fixture_allocated");
    if (readOnly) {
        const auto source = Table<18>();
        std::copy(source.begin(), source.end(), readOnly);
        DWORD previous{};
        VirtualProtect(readOnly, 4096, PAGE_READONLY, &previous);
        FakeCom readOnlyObject{readOnly};
        const bool installed = g_swapchainHooks.Install(&readOnlyObject, 18,
            [](overlay::DxgiHookRecord& record) {
                record.replacement[8] = reinterpret_cast<void*>(&HookPresent);
            });
        MEMORY_BASIC_INFORMATION protection{};
        VirtualQuery(readOnly, &protection, sizeof(protection));
        pass &= Check(installed && FindSwapchain(&readOnlyObject)->active.load(std::memory_order_acquire) &&
            protection.Protect == PAGE_READONLY &&
            readOnly[8] == reinterpret_cast<void*>(&HookPresent),
            "read_only_table_patch_restores_protection");
        // Registry owns no allocation/object; no callback is invoked after this free.
        VirtualFree(readOnly, 0, MEM_RELEASE);
    }

    // Object destruction/address reuse does not invalidate table-owned originals.
    FakeCom reused{factoryTable.data()};
    pass &= Check(FindFactory(&reused) == FindFactory(&factory), "new_object_same_table_has_original");
    reused.table = freshTable.data();
    pass &= Check(!FindFactory(&reused), "reused_address_different_table_not_stale");

    std::atomic<bool> concurrencyPass{true};
    std::vector<std::thread> readers;
    for (int index = 0; index < 4; ++index)
        readers.emplace_back([&]() {
            for (int iteration = 0; iteration < 2000; ++iteration) {
                const auto lease = FindFactory(&factory);
                if (!lease || lease->Original<CreateSwapChainForCoreWindowFn>(16) != &NativeCoreWindow)
                    concurrencyPass = false;
            }
        });
    for (int iteration = 0; iteration < 2000; ++iteration)
        InstallFactoryHooks(reinterpret_cast<IUnknown*>(&factory), 25, "IDXGIFactory2");
    for (auto& reader : readers) reader.join();
    pass &= Check(concurrencyPass, "concurrent_registry_reads_and_duplicate_install");
    std::array<std::array<void*, 18>, 128> concurrentTables;
    std::array<FakeCom, 128> concurrentObjects{};
    for (std::size_t index = 0; index < concurrentObjects.size(); ++index) {
        concurrentTables[index] = Table<18>();
        concurrentObjects[index].table = concurrentTables[index].data();
    }
    std::atomic<bool> finished{};
    std::thread reader([&]() {
        while (!finished) {
            for (auto& object : concurrentObjects) {
                const auto lease = FindSwapchain(&object);
                if (lease && lease->Original<PresentFn>(8) !=
                        reinterpret_cast<PresentFn>(&Noop)) concurrencyPass = false;
            }
        }
    });
    for (auto& object : concurrentObjects)
        g_swapchainHooks.Install(&object, 18, [](overlay::DxgiHookRecord& record) {
            record.replacement[8] = reinterpret_cast<void*>(&HookPresent);
        });
    finished = true;
    reader.join();
    pass &= Check(concurrencyPass, "concurrent_new_table_publication_has_originals");
    return pass ? 0 : 1;
}
