// Project-local adaptation: checked, bounded font upload lifetime.
#pragma once
#include <d3d12.h>
#include <cstdint>

struct ImGui_ImplDX12_FontUpload
{
    ID3D12Resource* texture{};
    ID3D12Resource* buffer{};
    ID3D12Fence* fence{};
    ID3D12CommandAllocator* allocator{};
    ID3D12GraphicsCommandList* list{};
    ID3D12CommandQueue* queue{};
    HANDLE event{};
    bool submitted{};
    bool completed{};

    bool Submit(ID3D12CommandQueue* submission_queue)
    {
        queue = submission_queue;
        queue->AddRef();
        ID3D12CommandList* lists[] = {list};
        submitted = true;
        queue->ExecuteCommandLists(1, lists);
        if (FAILED(queue->Signal(fence, 1))) return false;
        auto value = fence->GetCompletedValue();
        if (value == UINT64_MAX) return false;
        if (value < 1) {
            if (FAILED(fence->SetEventOnCompletion(1, event)) ||
                WaitForSingleObject(event, 1000) != WAIT_OBJECT_0) return false;
            value = fence->GetCompletedValue();
            if (value == UINT64_MAX || value < 1) return false;
        }
        completed = true;
        return true;
    }

    ~ImGui_ImplDX12_FontUpload()
    {
        // D3D12 does not retain resource references for submitted commands.
        // On unproven completion, deliberately retain this one terminal upload
        // until process exit. No allocation, retry, unsafe Release or event close.
        if (submitted && !completed) return;
        if (list) list->Release();
        if (allocator) allocator->Release();
        if (fence) fence->Release();
        if (buffer) buffer->Release();
        if (texture) texture->Release();
        if (queue) queue->Release();
        if (event) CloseHandle(event);
    }
};
