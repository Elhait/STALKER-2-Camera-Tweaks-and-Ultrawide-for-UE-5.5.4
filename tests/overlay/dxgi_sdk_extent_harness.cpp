#define CINTERFACE
#include <dxgi1_6.h>
#include <cstddef>
#include <iostream>
// CINTERFACE omits C++ uuid attributes. Only table extents are tested here;
// actual IIDs and typed signatures are exercised by dxgi_callback_harness.
constexpr IID kIgnoredIID{};
#define __uuidof(type) kIgnoredIID
#include "../../src/overlay/dxgi_abi.hpp"
#undef __uuidof

// Match every inherited extent and selected slot against the installed SDK.
#define EXTENT(type, count) static_assert(sizeof(type##Vtbl) == (count) * sizeof(void*))
EXTENT(IDXGIFactory, 12); EXTENT(IDXGIFactory1, 14);
EXTENT(IDXGIFactory2, 25); EXTENT(IDXGIFactory3, 26);
EXTENT(IDXGIFactory4, 28); EXTENT(IDXGIFactory5, 29);
EXTENT(IDXGIFactory6, 30); EXTENT(IDXGIFactory7, 32);
EXTENT(IDXGISwapChain, 18); EXTENT(IDXGISwapChain1, 29);
EXTENT(IDXGISwapChain2, 36); EXTENT(IDXGISwapChain3, 40);
EXTENT(IDXGISwapChain4, 41);
static_assert(offsetof(IDXGIFactory2Vtbl, CreateSwapChain) == 10 * sizeof(void*));
static_assert(offsetof(IDXGIFactory2Vtbl, CreateSwapChainForHwnd) == 15 * sizeof(void*));
static_assert(offsetof(IDXGIFactory2Vtbl, CreateSwapChainForCoreWindow) == 16 * sizeof(void*));
static_assert(offsetof(IDXGIFactory2Vtbl, CreateSwapChainForComposition) == 24 * sizeof(void*));
static_assert(offsetof(IDXGISwapChain3Vtbl, Present) == 8 * sizeof(void*));
static_assert(offsetof(IDXGISwapChain3Vtbl, ResizeBuffers) == 13 * sizeof(void*));
static_assert(offsetof(IDXGISwapChain3Vtbl, ResizeBuffers1) == 39 * sizeof(void*));

int main()
{
    const std::size_t factorySizes[]{sizeof(IDXGIFactory7Vtbl), sizeof(IDXGIFactory6Vtbl),
        sizeof(IDXGIFactory5Vtbl), sizeof(IDXGIFactory4Vtbl), sizeof(IDXGIFactory3Vtbl),
        sizeof(IDXGIFactory2Vtbl), sizeof(IDXGIFactory1Vtbl), sizeof(IDXGIFactoryVtbl),
        sizeof(IUnknownVtbl)};
    const std::size_t swapSizes[]{sizeof(IDXGISwapChain4Vtbl), sizeof(IDXGISwapChain3Vtbl),
        sizeof(IDXGISwapChain2Vtbl), sizeof(IDXGISwapChain1Vtbl), sizeof(IDXGISwapChainVtbl),
        sizeof(IUnknownVtbl)};
    for (std::size_t index = 0; index < std::size(factorySizes); ++index)
        if (overlay::FactoryInterfaces[index].methods * sizeof(void*) != factorySizes[index]) return 1;
    for (std::size_t index = 0; index < std::size(swapSizes); ++index)
        if (overlay::SwapchainInterfaces[index].methods * sizeof(void*) != swapSizes[index]) return 1;
    std::cout << "DXGI production extents and method slots match SDK: PASS\n";
}
