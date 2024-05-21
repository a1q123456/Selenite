module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
module NurbsViewer.MyRenderable;

namespace NurbsViewer
{
    auto MyRenderable::Render(float time) -> void
    {
        auto commandList = CreateDirectCommandList();

        // Transition the render target into the correct state to allow for drawing into it.
        const D3D12_RESOURCE_BARRIER barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(
            GetCurrentRTV().Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrierToRT);

        auto rtvHandle = GetRTVHandle();
        auto dsvHandle = GetDSVHandle();

        commandList->OMSetRenderTargets(
            1,
            &rtvHandle,
            FALSE,
            &dsvHandle);

        auto rgba = DirectX::Colors::CornflowerBlue;
        commandList->ClearRenderTargetView(
            rtvHandle,
            rgba,
            0,
            nullptr);
        commandList->ClearDepthStencilView(
            dsvHandle,
            D3D12_CLEAR_FLAG_DEPTH,
            1.0f,
            0,
            0,
            nullptr);

        // Set the viewport and scissor rect.
        const D3D12_VIEWPORT viewport = {
            0.0f,
            0.0f,
            static_cast<float>(GetOutputWidth()),
            static_cast<float>(GetOutputHeight()),
            D3D12_MIN_DEPTH,
            D3D12_MAX_DEPTH
        };
        const D3D12_RECT scissorRect = {
            0,
            0,
            GetOutputWidth(),
            GetOutputHeight()
        };
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        // Transition the render target to the state that allows it to be presented to the display.
        const D3D12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
            GetCurrentRTV().Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrierToPresent);
        commandList->Close();
        PushCommandList(std::move(commandList));
    }
}
