#pragma once
#include <dxgi1_6.h>
#include <type_traits>

namespace overlay
{
    template <typename Method> struct ComCallback;
    template <typename Result, typename Interface, typename... Arguments>
    struct ComCallback<Result(STDMETHODCALLTYPE Interface::*)(Arguments...)>
    {
        using Type = Result(STDMETHODCALLTYPE*)(Interface*, Arguments...);
    };
    template <typename Method> using ComCallbackType = typename ComCallback<Method>::Type;

    struct DxgiInterfaceExtent { const IID* iid; std::size_t methods; };
    // Descending order preserves the full SDK table when inherited interfaces
    // share a pointer. Distinct interface pointers are installed independently.
    inline const DxgiInterfaceExtent FactoryInterfaces[]{
        {&__uuidof(IDXGIFactory7), 32}, {&__uuidof(IDXGIFactory6), 30},
        {&__uuidof(IDXGIFactory5), 29}, {&__uuidof(IDXGIFactory4), 28},
        {&__uuidof(IDXGIFactory3), 26}, {&__uuidof(IDXGIFactory2), 25},
        {&__uuidof(IDXGIFactory1), 14}, {&__uuidof(IDXGIFactory), 12},
        {&__uuidof(IUnknown), 3}
    };
    inline const DxgiInterfaceExtent SwapchainInterfaces[]{
        {&__uuidof(IDXGISwapChain4), 41}, {&__uuidof(IDXGISwapChain3), 40},
        {&__uuidof(IDXGISwapChain2), 36}, {&__uuidof(IDXGISwapChain1), 29},
        {&__uuidof(IDXGISwapChain), 18}, {&__uuidof(IUnknown), 3}
    };
}
