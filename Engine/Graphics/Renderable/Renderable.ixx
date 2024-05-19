module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Renderable;
import Engine.Graphics.Device.GPUScheduler.RenderNode;
import Engine.Graphics.Device.CommandListPool;
import Engine.Graphics.Device.D3DX12;
import std;

using namespace Microsoft::WRL;

namespace Engine::Graphics
{
    namespace Device
    {
        export class GPUScheduler;
        export class Context;
    }

    export class Renderable
    {
    public:
        virtual auto Render(float time) -> void = 0;
        virtual ~Renderable() = default;

    protected:
        auto PushCommandList(Device::GraphicsCommandList&& commandList) const noexcept -> void;
        auto PresentFrame() const noexcept -> void;
        auto CreateDirectCommandList() const noexcept -> Device::GraphicsCommandList;
        auto CreateCopyCommandList() const noexcept -> Device::GraphicsCommandList;
        auto CreateComputeCommandList() const noexcept -> Device::GraphicsCommandList;
        auto CreateBundleCommandList() const noexcept -> Device::GraphicsCommandList;
        auto GetCurrentRTV() const noexcept -> ComPtr<ID3D12Resource>;
        auto GetRTVHandle() const noexcept -> CD3DX12_CPU_DESCRIPTOR_HANDLE;
        auto GetDSVHandle() const noexcept -> CD3DX12_CPU_DESCRIPTOR_HANDLE;
        auto GetOutputWidth() const noexcept -> int;
        auto GetOutputHeight() const noexcept -> int;

    private:
        auto CreateCommandList(D3D12_COMMAND_LIST_TYPE type) const noexcept -> Device::GraphicsCommandList;
        auto SetContext(Device::Context* scheduler) -> void;

        friend class Device::GPUScheduler;

        Device::Context* m_context = nullptr;
    };
}
