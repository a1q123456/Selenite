module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Device.Context;
import Engine.Graphics.Device.D3DX12;
import Engine.Core.IntPtr;
import std;

using namespace Engine::Core;
using namespace Microsoft::WRL;

namespace Engine::Graphics::Device
{
    export class CommandListPool;
    export class GPUScheduler;

    export class Context
    {
    public:
        static constexpr std::size_t SWAP_BUFFER_COUNT = 3;

        Context() = default;
        auto Initialise(IntPtr hwnd, int width, int height) -> void;
        auto Teardown() -> void;
        virtual ~Context() noexcept;

        auto OnDeviceLost() -> void;
        auto PresentFrame() -> void;

        auto GetCurrentRTV() const noexcept -> const ComPtr<ID3D12Resource>& { return m_renderTargets[m_backBufferIndex]; }
        auto GetDevice() const noexcept -> const ComPtr<ID3D12Device>& { return m_d3dDevice; }
        auto GetDXGIFactory() const noexcept -> const ComPtr<IDXGIFactory4>& { return m_dxgiFactory; }
        auto GetWindow() const noexcept { return m_window; }
        auto GetOutputWidth() const noexcept { return m_outputWidth; }
        auto GetOutputHeight() const noexcept { return m_outputHeight; }
        auto GetGPUScheduler() const noexcept -> const std::unique_ptr<GPUScheduler>& { return m_gpuScheduler; }
        auto GetCommandListPool() const noexcept -> const std::unique_ptr<CommandListPool>& { return m_commandListPool; }
        auto GetRTVDescriptorHeap() const noexcept -> const ComPtr<ID3D12DescriptorHeap>& { return m_rtvDescriptorHeap; }
        auto GetDSVDescriptorHeap() const noexcept -> const ComPtr<ID3D12DescriptorHeap>& { return m_dsvDescriptorHeap; }
        auto GetRTVDescriptorSize() const noexcept { return m_rtvDescriptorSize; }
        auto GetRTVHandle() const noexcept -> CD3DX12_CPU_DESCRIPTOR_HANDLE;
        auto GetDSVHandle() const noexcept -> CD3DX12_CPU_DESCRIPTOR_HANDLE;
        auto RollRenderTarget() noexcept
        {
            m_backBufferIndex = (m_backBufferIndex + 1) % SWAP_BUFFER_COUNT;
        }
    private:

        auto CreateDevice() -> void;
        auto CreateResources() -> void;
        auto GetAdapter(IDXGIAdapter1** ppAdapter) -> void;


        // Application state
        IntPtr m_window{};
        int m_outputWidth = 1024;
        int m_outputHeight = 768;
        UINT m_backBufferIndex{};
        
        // Direct3D Objects
        D3D_FEATURE_LEVEL m_featureLevel{};
        UINT m_rtvDescriptorSize{};
        ComPtr<ID3D12Device> m_d3dDevice;
        ComPtr<IDXGIFactory4> m_dxgiFactory;
        ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
        ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;

        // Rendering resources
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12Resource> m_renderTargets[SWAP_BUFFER_COUNT];
        ComPtr<ID3D12Resource> m_depthStencil;

        std::unique_ptr<GPUScheduler> m_gpuScheduler;
        std::unique_ptr<CommandListPool> m_commandListPool;
    };
}

