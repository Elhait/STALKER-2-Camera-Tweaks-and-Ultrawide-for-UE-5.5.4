// Actual backend with synthetic COM dispatch: no GPU or game initialization.
#include "../../external/imgui/backends/imgui_impl_dx12.cpp"
#include <array>
#include <iostream>

namespace {
struct Fake { void** table; ULONG refs{1}; unsigned calls{}; };
ULONG STDMETHODCALLTYPE Add(IUnknown* p) { return ++reinterpret_cast<Fake*>(p)->refs; }
ULONG STDMETHODCALLTYPE Release(IUnknown* p) { return --reinterpret_cast<Fake*>(p)->refs; }
unsigned resourceCalls{}, failResourceCall{1};
Fake* fontAllocation{};
Fake* uploadAllocation{};
HRESULT STDMETHODCALLTYPE FailResource(ID3D12Device*, const D3D12_HEAP_PROPERTIES*, D3D12_HEAP_FLAGS,
    const D3D12_RESOURCE_DESC*, D3D12_RESOURCE_STATES, const D3D12_CLEAR_VALUE*, REFIID, void** out)
{
    ++resourceCalls;
    if (resourceCalls == failResourceCall) { *out = nullptr; return E_OUTOFMEMORY; }
    *out = resourceCalls == 1 ? fontAllocation : uploadAllocation;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE FailMap(ID3D12Resource* p, UINT, const D3D12_RANGE*, void**)
{ ++reinterpret_cast<Fake*>(p)->calls; return E_FAIL; }
void STDMETHODCALLTYPE Execute(ID3D12CommandQueue* p, UINT, ID3D12CommandList* const*)
{ ++reinterpret_cast<Fake*>(p)->calls; }
HRESULT signalResult{S_OK}, eventResult{S_OK}; UINT64 completedValue{1};
HRESULT STDMETHODCALLTYPE Signal(ID3D12CommandQueue*, ID3D12Fence*, UINT64) { return signalResult; }
UINT64 STDMETHODCALLTYPE Completed(ID3D12Fence*) { return completedValue; }
HRESULT STDMETHODCALLTYPE Event(ID3D12Fence*, UINT64, HANDLE) { return eventResult; }
template<size_t N> auto Table() { std::array<void*, N> r{};
    r[1] = reinterpret_cast<void*>(&Add); r[2] = reinterpret_cast<void*>(&Release); return r; }
bool Check(bool ok, const char* name) { std::cout << name << ": " << (ok ? "PASS" : "FAIL") << '\n'; return ok; }
void FreeDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {}
}

int main() {
    bool pass = true;
    auto resources = Table<15>(); resources[8] = reinterpret_cast<void*>(&FailMap);
    auto devices = Table<44>(); devices[27] = reinterpret_cast<void*>(&FailResource);
    auto queues = Table<19>(); queues[10] = reinterpret_cast<void*>(&Execute); queues[14] = reinterpret_cast<void*>(&Signal);
    auto fences = Table<11>(); fences[8] = reinterpret_cast<void*>(&Completed); fences[9] = reinterpret_cast<void*>(&Event);
    auto refs = Table<3>(); Fake device{devices.data()};
    ImGui::CreateContext(); ImGui::GetIO().Fonts->AddFontDefault();
    auto* bd = IM_NEW(ImGui_ImplDX12_Data)();
    bd->pd3dDevice = reinterpret_cast<ID3D12Device*>(&device); bd->numFramesInFlight = 3;
    bd->InitInfo.SrvDescriptorFreeFn = &FreeDescriptor;
    ImGui::GetIO().BackendRendererUserData = bd;
    pass &= Check(!ImGui_ImplDX12_CreateFontsTexture(), "failed_font_allocation_returns_false_without_null_use");
    for (unsigned failure : {2u, 3u}) {
        Fake texture{resources.data()}, buffer{resources.data()};
        resourceCalls = 0; failResourceCall = failure;
        fontAllocation = &texture; uploadAllocation = &buffer;
        pass &= Check(!ImGui_ImplDX12_CreateFontsTexture() && !texture.refs &&
            (failure == 2 ? buffer.refs == 1 : !buffer.refs && buffer.calls == 1),
            "upload_allocation_map_failure_unwinds_pre_submit_refs");
    }
    ImGui_ImplDX12_Shutdown();
    pass &= Check(!ImGui::GetIO().BackendRendererUserData, "partial_backend_null_frame_array_shutdown");
    for (int scenario = 0; scenario < 5; ++scenario) {
        Fake texture{refs.data()}, buffer{refs.data()}, fence{fences.data()}, allocator{refs.data()}, list{refs.data()}, queue{queues.data()};
        signalResult = scenario == 1 ? E_FAIL : S_OK; eventResult = scenario == 3 ? E_FAIL : S_OK;
        completedValue = scenario == 2 ? UINT64_MAX : scenario >= 3 ? 0 : 1;
        HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr); const auto start = GetTickCount64();
        {
            ImGui_ImplDX12_FontUpload upload;
            upload.texture = reinterpret_cast<ID3D12Resource*>(&texture); upload.buffer = reinterpret_cast<ID3D12Resource*>(&buffer);
            upload.fence = reinterpret_cast<ID3D12Fence*>(&fence); upload.allocator = reinterpret_cast<ID3D12CommandAllocator*>(&allocator);
            upload.list = reinterpret_cast<ID3D12GraphicsCommandList*>(&list); upload.event = event;
            pass &= Check(upload.Submit(reinterpret_cast<ID3D12CommandQueue*>(&queue)) == (scenario == 0), "upload_checks_signal_device_event_timeout");
        }
        pass &= Check(scenario == 0 ? !texture.refs && !buffer.refs && !list.refs && queue.refs == 1 :
            texture.refs == 1 && buffer.refs == 1 && list.refs == 1 && queue.refs == 2, "unproven_gpu_upload_retains_refs");
        pass &= Check(GetTickCount64() - start < 3000, "upload_wait_bounded");
        if (scenario != 0) CloseHandle(event); // Synthetic GPU never executes.
    }
    Fake unsubmitted{refs.data()};
    { ImGui_ImplDX12_FontUpload upload; upload.texture = reinterpret_cast<ID3D12Resource*>(&unsubmitted); }
    pass &= Check(!unsubmitted.refs, "pre_submit_failure_releases_refs");
    bd = IM_NEW(ImGui_ImplDX12_Data)(); bd->pd3dDevice = reinterpret_cast<ID3D12Device*>(&device);
    bd->numFramesInFlight = 3; bd->InitInfo.SrvDescriptorFreeFn = &FreeDescriptor;
    bd->pFrameResources = new ImGui_ImplDX12_RenderBuffers[3]{};
    Fake root{refs.data()}, pipeline{refs.data()}, font{refs.data()};
    bd->pRootSignature = reinterpret_cast<ID3D12RootSignature*>(&root); bd->pPipelineState = reinterpret_cast<ID3D12PipelineState*>(&pipeline);
    bd->FontTexture.pTextureResource = reinterpret_cast<ID3D12Resource*>(&font);
    std::array<Fake, 3> vertices{{{resources.data()}, {resources.data()}, {resources.data()}}};
    std::array<Fake, 3> indices{{{resources.data()}, {resources.data()}, {resources.data()}}};
    for (unsigned i = 0; i < 3; ++i) bd->pFrameResources[i] = {
        reinterpret_cast<ID3D12Resource*>(&indices[i]), reinterpret_cast<ID3D12Resource*>(&vertices[i]), 100, 100};
    ImGui::GetIO().BackendRendererUserData = bd;
    ImDrawData data; data.DisplaySize = ImVec2(100, 100);
    for (int slot : {2, 2, 0, 2}) pass &= Check(!ImGui_ImplDX12_RenderDrawData(&data, nullptr, slot) && bd->frameIndex == (UINT)slot,
        "actual_backend_waited_slot_and_map_failure");
    pass &= Check(vertices[0].calls == 1 && !vertices[1].calls && vertices[2].calls == 3, "only_waited_buffers_touched");
    ImGui_ImplDX12_Shutdown(true);
    pass &= Check(root.refs == 1 && pipeline.refs == 1 && font.refs == 1 && vertices[2].refs == 1, "terminal_backend_retains_gpu_refs");
    ImGui::DestroyContext(); return pass ? 0 : 1;
}
