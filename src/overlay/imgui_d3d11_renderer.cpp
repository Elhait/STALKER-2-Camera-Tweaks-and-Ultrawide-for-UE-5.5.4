#include "imgui_d3d11_renderer.hpp"

#include <imgui.h>
#include <d3dcompiler.h>
#include <d3d11_1.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace overlay
{
    bool ComputeImGuiDrawBounds(const ImDrawData* drawData, UINT surfaceWidth,
        UINT surfaceHeight, presentation::Rect& bounds) noexcept
    {
        bounds = {};
        if (!drawData || !surfaceWidth || !surfaceHeight ||
            !std::isfinite(drawData->FramebufferScale.x) ||
            !std::isfinite(drawData->FramebufferScale.y) ||
            !std::isfinite(drawData->DisplayPos.x) ||
            !std::isfinite(drawData->DisplayPos.y) ||
            !std::isfinite(drawData->DisplaySize.x) ||
            !std::isfinite(drawData->DisplaySize.y) ||
            drawData->FramebufferScale.x <= 0.0f ||
            drawData->FramebufferScale.y <= 0.0f ||
            drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f ||
            drawData->DisplaySize.x > static_cast<float>(surfaceWidth) * 4.0f ||
            drawData->DisplaySize.y > static_cast<float>(surfaceHeight) * 4.0f)
            return false;

        const presentation::Rect extent{0, 0, static_cast<int>(surfaceWidth),
            static_cast<int>(surfaceHeight)};
        for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
            const ImDrawList* list = drawData->CmdLists[listIndex];
            if (!list) return false;
            for (const ImDrawCmd& command : list->CmdBuffer) {
                if (command.UserCallback) return false;
                if (!command.ElemCount) continue;
                if (!std::isfinite(command.ClipRect.x) ||
                    !std::isfinite(command.ClipRect.y) ||
                    !std::isfinite(command.ClipRect.z) ||
                    !std::isfinite(command.ClipRect.w))
                    return false;
                const std::uint64_t indexEnd =
                    static_cast<std::uint64_t>(command.IdxOffset) + command.ElemCount;
                if (indexEnd > static_cast<std::uint64_t>(list->IdxBuffer.Size))
                    return false;

                float minX = (std::numeric_limits<float>::max)();
                float minY = (std::numeric_limits<float>::max)();
                float maxX = -(std::numeric_limits<float>::max)();
                float maxY = -(std::numeric_limits<float>::max)();
                for (std::uint64_t index = command.IdxOffset;
                    index < indexEnd; ++index) {
                    const std::uint64_t vertexIndex =
                        static_cast<std::uint64_t>(list->IdxBuffer[static_cast<int>(index)]) +
                        command.VtxOffset;
                    if (vertexIndex >= static_cast<std::uint64_t>(list->VtxBuffer.Size))
                        return false;
                    const ImVec2 position = list->VtxBuffer[
                        static_cast<int>(vertexIndex)].pos;
                    if (!std::isfinite(position.x) || !std::isfinite(position.y))
                        return false;
                    minX = (std::min)(minX, position.x);
                    minY = (std::min)(minY, position.y);
                    maxX = (std::max)(maxX, position.x);
                    maxY = (std::max)(maxY, position.y);
                }

                const ImVec2 clipMin(
                    (std::max)(command.ClipRect.x, drawData->DisplayPos.x),
                    (std::max)(command.ClipRect.y, drawData->DisplayPos.y));
                const ImVec2 clipMax(
                    (std::min)(command.ClipRect.z,
                        drawData->DisplayPos.x + drawData->DisplaySize.x),
                    (std::min)(command.ClipRect.w,
                        drawData->DisplayPos.y + drawData->DisplaySize.y));
                minX = (std::max)(minX, clipMin.x);
                minY = (std::max)(minY, clipMin.y);
                maxX = (std::min)(maxX, clipMax.x);
                maxY = (std::min)(maxY, clipMax.y);
                if (maxX <= minX || maxY <= minY) continue;

                const float scaleX = drawData->FramebufferScale.x;
                const float scaleY = drawData->FramebufferScale.y;
                const float originX = drawData->DisplayPos.x;
                const float originY = drawData->DisplayPos.y;
                const auto lower = [](float value) {
                    return static_cast<int>(std::floor(value)) - 1;
                };
                const auto upper = [](float value) {
                    return static_cast<int>(std::ceil(value)) + 1;
                };
                const presentation::Rect commandBounds{
                    lower((minX - originX) * scaleX),
                    lower((minY - originY) * scaleY),
                    upper((maxX - originX) * scaleX),
                    upper((maxY - originY) * scaleY) };
                bounds = presentation::Union(bounds,
                    presentation::Intersect(commandBounds, extent));
            }
        }
        return true;
    }

    namespace
    {
        using Microsoft::WRL::ComPtr;

        struct VertexConstants
        {
            float scale[2];
            float translate[2];
        };

        constexpr char VertexShaderSource[] =
            "cbuffer Projection : register(b0) { float2 Scale; float2 Translate; };"
            "struct Input { float2 Position : POSITION; float2 UV : TEXCOORD0;"
            " float4 Color : COLOR0; };"
            "struct Output { float4 Position : SV_POSITION; float2 UV : TEXCOORD0;"
            " float4 Color : COLOR0; };"
            "Output main(Input input) { Output output;"
            " output.Position = float4(input.Position * Scale + Translate, 0.0, 1.0);"
            " output.UV = input.UV; output.Color = input.Color; return output; }";

        constexpr char PixelShaderSource[] =
            "Texture2D Texture : register(t0);"
            "SamplerState TextureSampler : register(s0);"
            "struct Input { float4 Position : SV_POSITION; float2 UV : TEXCOORD0;"
            " float4 Color : COLOR0; };"
            "float4 main(Input input) : SV_Target {"
            " return input.Color * Texture.Sample(TextureSampler, input.UV); }";

        ImTextureID TextureId(ID3D11ShaderResourceView* view) noexcept
        {
            return static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(view));
        }

        UINT GrowCapacity(UINT current, UINT required) noexcept
        {
            constexpr UINT MaximumBytes = 64u * 1024u * 1024u;
            if (!required || required > MaximumBytes) return 0;
            UINT capacity = current ? current : 4096u;
            while (capacity < required) {
                if (capacity > MaximumBytes / 2u) {
                    capacity = MaximumBytes;
                    break;
                }
                capacity *= 2u;
            }
            return capacity >= required ? capacity : 0;
        }

        bool CompileShader(const char* source, const char* target,
            ComPtr<ID3DBlob>& bytecode) noexcept
        {
            ComPtr<ID3DBlob> errors;
            const HRESULT result = D3DCompile(source, std::strlen(source), nullptr,
                nullptr, nullptr, "main", target, D3DCOMPILE_ENABLE_STRICTNESS,
                0, bytecode.GetAddressOf(), errors.GetAddressOf());
            return SUCCEEDED(result) && bytecode;
        }
    }

    bool ImGuiD3D11Renderer::Initialize(ID3D11Device* device,
        ImFontAtlas* fonts) noexcept
    {
        if (!device || !fonts) return false;
        Shutdown(fonts);
        device_ = device;
        device_->GetImmediateContext(context_.GetAddressOf());
        context_.As(&context1_);
        return context_ && CreatePipeline() && CreateFontTexture(fonts);
    }

    bool ImGuiD3D11Renderer::CreatePipeline() noexcept
    {
        ComPtr<ID3DBlob> vertexBytecode;
        ComPtr<ID3DBlob> pixelBytecode;
        if (!CompileShader(VertexShaderSource, "vs_4_0", vertexBytecode) ||
            !CompileShader(PixelShaderSource, "ps_4_0", pixelBytecode))
            return false;

        if (FAILED(device_->CreateVertexShader(vertexBytecode->GetBufferPointer(),
                vertexBytecode->GetBufferSize(), nullptr, vertexShader_.GetAddressOf())) ||
            FAILED(device_->CreatePixelShader(pixelBytecode->GetBufferPointer(),
                pixelBytecode->GetBufferSize(), nullptr, pixelShader_.GetAddressOf())))
            return false;

        const D3D11_INPUT_ELEMENT_DESC elements[]{
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                static_cast<UINT>(offsetof(ImDrawVert, pos)), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                static_cast<UINT>(offsetof(ImDrawVert, uv)), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0,
                static_cast<UINT>(offsetof(ImDrawVert, col)), D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        if (FAILED(device_->CreateInputLayout(elements,
                static_cast<UINT>(std::size(elements)),
                vertexBytecode->GetBufferPointer(), vertexBytecode->GetBufferSize(),
                inputLayout_.GetAddressOf())))
            return false;

        D3D11_BUFFER_DESC constantsDesc{};
        constantsDesc.ByteWidth = sizeof(VertexConstants);
        constantsDesc.Usage = D3D11_USAGE_DYNAMIC;
        constantsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantsDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(device_->CreateBuffer(&constantsDesc, nullptr,
                constants_.GetAddressOf())))
            return false;

        D3D11_BLEND_DESC blendDesc{};
        auto& blend = blendDesc.RenderTarget[0];
        blend.BlendEnable = TRUE;
        blend.SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blend.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend.BlendOp = D3D11_BLEND_OP_ADD;
        blend.SrcBlendAlpha = D3D11_BLEND_ONE;
        blend.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blend.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(device_->CreateBlendState(&blendDesc, blendState_.GetAddressOf())))
            return false;

        D3D11_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.ScissorEnable = TRUE;
        rasterizerDesc.DepthClipEnable = TRUE;
        if (FAILED(device_->CreateRasterizerState(&rasterizerDesc,
                rasterizerState_.GetAddressOf())))
            return false;

        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        return SUCCEEDED(device_->CreateSamplerState(&samplerDesc,
            sampler_.GetAddressOf()));
    }

    bool ImGuiD3D11Renderer::CreateFontTexture(ImFontAtlas* fonts) noexcept
    {
        if (!device_ || !fonts) return false;
        unsigned char* pixels{};
        int width{};
        int height{};
        fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        if (!pixels || width <= 0 || height <= 0 ||
            static_cast<std::uint64_t>(width) * 4u > UINT_MAX)
            return false;

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = static_cast<UINT>(width);
        textureDesc.Height = static_cast<UINT>(height);
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = pixels;
        initialData.SysMemPitch = static_cast<UINT>(width) * 4u;
        ComPtr<ID3D11Texture2D> texture;
        if (FAILED(device_->CreateTexture2D(&textureDesc, &initialData,
                texture.GetAddressOf())) ||
            FAILED(device_->CreateShaderResourceView(texture.Get(), nullptr,
                fontTexture_.GetAddressOf())))
            return false;
        fonts->SetTexID(TextureId(fontTexture_.Get()));
        return true;
    }

    void ImGuiD3D11Renderer::InvalidateFontTexture(ImFontAtlas* fonts) noexcept
    {
        if (fonts && fonts->TexID == TextureId(fontTexture_.Get()))
            fonts->SetTexID(0);
        fontTexture_.Reset();
    }

    bool ImGuiD3D11Renderer::EnsureBuffers(UINT vertexCount,
        UINT indexCount) noexcept
    {
        const std::uint64_t vertexBytes64 = static_cast<std::uint64_t>(vertexCount) *
            sizeof(ImDrawVert);
        const std::uint64_t indexBytes64 = static_cast<std::uint64_t>(indexCount) *
            sizeof(ImDrawIdx);
        if (!vertexCount || !indexCount || vertexBytes64 > UINT_MAX ||
            indexBytes64 > UINT_MAX)
            return false;
        const UINT vertexRequired = static_cast<UINT>(vertexBytes64);
        const UINT indexRequired = static_cast<UINT>(indexBytes64);
        if (vertexRequired > vertexCapacity_) {
            const UINT capacity = GrowCapacity(vertexCapacity_, vertexRequired);
            if (!capacity) return false;
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = capacity;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            ComPtr<ID3D11Buffer> replacement;
            if (FAILED(device_->CreateBuffer(&desc, nullptr, replacement.GetAddressOf())))
                return false;
            vertexBuffer_ = std::move(replacement);
            vertexCapacity_ = capacity;
        }
        if (indexRequired > indexCapacity_) {
            const UINT capacity = GrowCapacity(indexCapacity_, indexRequired);
            if (!capacity) return false;
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = capacity;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            ComPtr<ID3D11Buffer> replacement;
            if (FAILED(device_->CreateBuffer(&desc, nullptr, replacement.GetAddressOf())))
                return false;
            indexBuffer_ = std::move(replacement);
            indexCapacity_ = capacity;
        }
        return true;
    }

    bool ImGuiD3D11Renderer::SetupRenderState(const ImDrawData* drawData,
        ID3D11RenderTargetView* target, UINT surfaceWidth,
        UINT surfaceHeight, RECT updateRect, POINT updateOffset,
        UINT vertexOffset) noexcept
    {
        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = static_cast<float>(updateOffset.x);
        viewport.TopLeftY = static_cast<float>(updateOffset.y);
        viewport.Width = static_cast<float>(surfaceWidth);
        viewport.Height = static_cast<float>(surfaceHeight);
        viewport.MaxDepth = 1.0f;
        context_->RSSetViewports(1, &viewport);
        const UINT stride = sizeof(ImDrawVert);
        const UINT offset = vertexOffset;
        ID3D11Buffer* vertexBuffer = vertexBuffer_.Get();
        context_->IASetInputLayout(inputLayout_.Get());
        context_->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context_->IASetIndexBuffer(indexBuffer_.Get(),
            sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
        context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
        ID3D11Buffer* constants = constants_.Get();
        context_->VSSetConstantBuffers(0, 1, &constants);
        ID3D11SamplerState* sampler = sampler_.Get();
        context_->PSSetSamplers(0, 1, &sampler);
        const float factor[]{0, 0, 0, 0};
        context_->OMSetBlendState(blendState_.Get(), factor, 0xFFFFFFFFu);
        context_->OMSetRenderTargets(1, &target, nullptr);
        context_->RSSetState(rasterizerState_.Get());

        const ImVec2 displayPosition = drawData->DisplayPos;
        const ImVec2 displaySize = drawData->DisplaySize;
        const VertexConstants projection{
            {2.0f / displaySize.x, -2.0f / displaySize.y},
            {-1.0f - displayPosition.x * (2.0f / displaySize.x),
                1.0f + displayPosition.y * (2.0f / displaySize.y)}
        };
        auto adjustedProjection = projection;
        adjustedProjection.translate[0] -= 2.0f * updateRect.left /
            static_cast<float>(surfaceWidth);
        adjustedProjection.translate[1] += 2.0f * updateRect.top /
            static_cast<float>(surfaceHeight);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context_->Map(constants_.Get(), 0, D3D11_MAP_WRITE_DISCARD,
                0, &mapped)))
            return false;
        std::memcpy(mapped.pData, &adjustedProjection, sizeof(adjustedProjection));
        context_->Unmap(constants_.Get(), 0);
        return true;
    }

    bool ImGuiD3D11Renderer::Render(const ImDrawData* drawData,
        ID3D11RenderTargetView* target, UINT surfaceWidth, UINT surfaceHeight,
        RECT updateRect, POINT updateOffset) noexcept
    {
        if (!context_ || !drawData || !target || !surfaceWidth || !surfaceHeight ||
            drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
            return false;
        const LONG updateWidth = updateRect.right - updateRect.left;
        const LONG updateHeight = updateRect.bottom - updateRect.top;
        if (updateWidth <= 0 || updateHeight <= 0 || updateOffset.x < 0 ||
            updateOffset.y < 0)
            return false;
        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> targetTexture;
        target->GetResource(resource.GetAddressOf());
        if (FAILED(resource.As(&targetTexture))) return false;
        D3D11_TEXTURE2D_DESC targetDesc{};
        targetTexture->GetDesc(&targetDesc);
        if (static_cast<std::uint64_t>(updateOffset.x) + updateWidth > targetDesc.Width ||
            static_cast<std::uint64_t>(updateOffset.y) + updateHeight > targetDesc.Height)
            return false;
        const D3D11_RECT clearRect{updateOffset.x, updateOffset.y,
            updateOffset.x + updateWidth, updateOffset.y + updateHeight};
        const float transparent[]{0.0f, 0.0f, 0.0f, 0.0f};
        if (context1_) {
            context1_->ClearView(target, transparent, &clearRect, 1);
        } else if (updateRect.left == 0 && updateRect.top == 0 &&
            updateRect.right == static_cast<LONG>(surfaceWidth) &&
            updateRect.bottom == static_cast<LONG>(surfaceHeight)) {
            context_->ClearRenderTargetView(target, transparent);
        } else {
            return false;
        }
        if (!drawData->TotalVtxCount || !drawData->TotalIdxCount) {
            return true;
        }
        if (!EnsureBuffers(static_cast<UINT>(drawData->TotalVtxCount),
                static_cast<UINT>(drawData->TotalIdxCount)))
            return false;

        D3D11_MAPPED_SUBRESOURCE vertexMap{};
        D3D11_MAPPED_SUBRESOURCE indexMap{};
        if (FAILED(context_->Map(vertexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD,
                0, &vertexMap)))
            return false;
        if (FAILED(context_->Map(indexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD,
                0, &indexMap))) {
            context_->Unmap(vertexBuffer_.Get(), 0);
            return false;
        }
        auto* vertexDestination = static_cast<ImDrawVert*>(vertexMap.pData);
        auto* indexDestination = static_cast<ImDrawIdx*>(indexMap.pData);
        for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
            const ImDrawList* list = drawData->CmdLists[listIndex];
            std::memcpy(vertexDestination, list->VtxBuffer.Data,
                static_cast<std::size_t>(list->VtxBuffer.Size) * sizeof(ImDrawVert));
            std::memcpy(indexDestination, list->IdxBuffer.Data,
                static_cast<std::size_t>(list->IdxBuffer.Size) * sizeof(ImDrawIdx));
            vertexDestination += list->VtxBuffer.Size;
            indexDestination += list->IdxBuffer.Size;
        }
        context_->Unmap(vertexBuffer_.Get(), 0);
        context_->Unmap(indexBuffer_.Get(), 0);

        if (!SetupRenderState(drawData, target, surfaceWidth, surfaceHeight,
                updateRect, updateOffset, 0))
            return false;

        UINT globalIndexOffset = 0;
        UINT globalVertexOffset = 0;
        for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
            const ImDrawList* list = drawData->CmdLists[listIndex];
            for (const ImDrawCmd& command : list->CmdBuffer) {
                if (command.UserCallback) {
                    if (command.UserCallback == ImDrawCallback_ResetRenderState) {
                        if (!SetupRenderState(drawData, target, surfaceWidth,
                                surfaceHeight, updateRect, updateOffset, 0))
                            return false;
                    } else {
                        command.UserCallback(list, &command);
                    }
                    continue;
                }

                const ImVec2 clipMinimum(
                    (command.ClipRect.x - drawData->DisplayPos.x) * drawData->FramebufferScale.x,
                    (command.ClipRect.y - drawData->DisplayPos.y) * drawData->FramebufferScale.y);
                const ImVec2 clipMaximum(
                    (command.ClipRect.z - drawData->DisplayPos.x) * drawData->FramebufferScale.x,
                    (command.ClipRect.w - drawData->DisplayPos.y) * drawData->FramebufferScale.y);
                const LONG left = static_cast<LONG>((std::max)(0.0f, clipMinimum.x)) +
                    updateOffset.x - updateRect.left;
                const LONG top = static_cast<LONG>((std::max)(0.0f, clipMinimum.y)) +
                    updateOffset.y - updateRect.top;
                const LONG right = static_cast<LONG>((std::min)(
                    static_cast<float>(surfaceWidth), clipMaximum.x)) +
                    updateOffset.x - updateRect.left;
                const LONG bottom = static_cast<LONG>((std::min)(
                    static_cast<float>(surfaceHeight), clipMaximum.y)) +
                    updateOffset.y - updateRect.top;
                const D3D11_RECT targetUpdateRect{updateOffset.x, updateOffset.y,
                    updateOffset.x + updateWidth, updateOffset.y + updateHeight};
                const LONG clippedLeft = (std::max)(left, targetUpdateRect.left);
                const LONG clippedTop = (std::max)(top, targetUpdateRect.top);
                const LONG clippedRight = (std::min)(right, targetUpdateRect.right);
                const LONG clippedBottom = (std::min)(bottom, targetUpdateRect.bottom);
                if (clippedRight <= clippedLeft || clippedBottom <= clippedTop) continue;
                const D3D11_RECT scissor{clippedLeft, clippedTop,
                    clippedRight, clippedBottom};
                context_->RSSetScissorRects(1, &scissor);
                auto* texture = reinterpret_cast<ID3D11ShaderResourceView*>(
                    static_cast<std::uintptr_t>(command.GetTexID()));
                ID3D11ShaderResourceView* textures[]{texture};
                context_->PSSetShaderResources(0, 1, textures);
                context_->DrawIndexed(command.ElemCount,
                    globalIndexOffset + command.IdxOffset,
                    static_cast<INT>(globalVertexOffset + command.VtxOffset));
            }
            globalIndexOffset += static_cast<UINT>(list->IdxBuffer.Size);
            globalVertexOffset += static_cast<UINT>(list->VtxBuffer.Size);
        }

        ID3D11ShaderResourceView* noTexture{};
        context_->PSSetShaderResources(0, 1, &noTexture);
        context_->OMSetRenderTargets(0, nullptr, nullptr);
        return true;
    }

    void ImGuiD3D11Renderer::Shutdown(ImFontAtlas* fonts) noexcept
    {
        InvalidateFontTexture(fonts);
        if (context_) {
            context_->ClearState();
            context_->Flush();
        }
        indexBuffer_.Reset();
        vertexBuffer_.Reset();
        sampler_.Reset();
        rasterizerState_.Reset();
        blendState_.Reset();
        constants_.Reset();
        inputLayout_.Reset();
        pixelShader_.Reset();
        vertexShader_.Reset();
        context_.Reset();
        context1_.Reset();
        device_.Reset();
        vertexCapacity_ = 0;
        indexCapacity_ = 0;
    }
}
