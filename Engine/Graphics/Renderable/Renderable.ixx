module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Renderable;
import Engine.Graphics.Device.GPUScheduler.RenderNode;
import Engine.Graphics.Device.CommandListPool;
import Engine.Graphics.Device.D3DX12;
import Engine.Core.Threading.Tasks;
import std;

using namespace Microsoft::WRL;
using namespace Engine::Core::Threading;

namespace Engine::Graphics
{
    namespace Device
    {
        export class GPUScheduler;
        export class Context;
    }

    // TODO Move this to somewhere else
    export enum class MouseButton
    {
        Left,
        Middle,
        Right
    };

    export class Renderable
    {
    public:
        virtual Task<void> Initialise();
        virtual auto Render(float time) -> void = 0;
        virtual auto OnMouseMove(int x, int y) -> void;
        virtual auto OnMouseDown(MouseButton button) -> void;
        virtual auto OnMouseUp(MouseButton button) -> void;
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
        auto GetDevice() const noexcept -> ComPtr<ID3D12Device>;
        Task<void> ExecuteCommandAsync(Device::GraphicsCommandList&& commandList) const noexcept;

    private:
        auto CreateCommandList(D3D12_COMMAND_LIST_TYPE type) const noexcept -> Device::GraphicsCommandList;
        Task<void> SetContext(Device::Context* scheduler);

        friend class Device::GPUScheduler;

        Device::Context* m_context = nullptr;
    };
}
