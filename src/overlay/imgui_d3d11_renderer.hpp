#pragma once

#include "presentation_contracts.hpp"

#include <d3d11_1.h>
#include <Windows.h>
#include <wrl/client.h>

struct ImDrawData;
struct ImFontAtlas;

namespace overlay
{
    bool ComputeImGuiDrawBounds(const ImDrawData* drawData, UINT surfaceWidth,
        UINT surfaceHeight, presentation::Rect& bounds) noexcept;

    class ImGuiD3D11Renderer
    {
    public:
        bool Initialize(ID3D11Device* device, ImFontAtlas* fonts) noexcept;
        bool CreateFontTexture(ImFontAtlas* fonts) noexcept;
        void InvalidateFontTexture(ImFontAtlas* fonts) noexcept;
        bool SupportsPartialSurfaceUpdates() const noexcept
        {
            return context1_ != nullptr;
        }
        bool Render(const ImDrawData* drawData, ID3D11RenderTargetView* target,
            UINT surfaceWidth, UINT surfaceHeight, RECT updateRect,
            POINT updateOffset) noexcept;
        void Shutdown(ImFontAtlas* fonts) noexcept;

    private:
        bool CreatePipeline() noexcept;
        bool EnsureBuffers(UINT vertexCount, UINT indexCount) noexcept;
        bool SetupRenderState(const ImDrawData* drawData,
            ID3D11RenderTargetView* target, UINT surfaceWidth,
            UINT surfaceHeight, RECT updateRect, POINT updateOffset,
            UINT vertexOffset) noexcept;

        Microsoft::WRL::ComPtr<ID3D11Device> device_;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context1_;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11BlendState> blendState_;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> fontTexture_;
        UINT vertexCapacity_{};
        UINT indexCapacity_{};
    };
}
