module Engine.Graphics.Renderable;
import Engine.Graphics.Device.Context;
import Engine.Graphics.Device.GPUScheduler;

namespace Engine::Graphics
{
    auto Renderable::Initialise() -> Task<void>
    {
        return {};
    }

    auto Renderable::OnMouseMove(int x, int y) -> void
    {
    }

    auto Renderable::OnMouseDown(MouseButton button) -> void
    {
    }

    auto Renderable::OnMouseUp(MouseButton button) -> void
    {
    }

    auto Renderable::PushCommandList(Device::GraphicsCommandList&& commandList) const noexcept -> void
    {
        m_context->GetGPUScheduler()
            ->PushCommands(std::move(commandList));
    }

    auto Renderable::PresentFrame() const noexcept -> void
    {
        return PushCommandList({});
    }

    auto Renderable::CreateDirectCommandList() const noexcept -> Device::GraphicsCommandList
    {
        return CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    auto Renderable::CreateCopyCommandList() const noexcept -> Device::GraphicsCommandList
    {
        return CreateCommandList(D3D12_COMMAND_LIST_TYPE_COPY);
    }

    auto Renderable::CreateComputeCommandList() const noexcept -> Device::GraphicsCommandList
    {
        return CreateCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }

    auto Renderable::CreateBundleCommandList() const noexcept -> Device::GraphicsCommandList
    {
        return CreateCommandList(D3D12_COMMAND_LIST_TYPE_BUNDLE);
    }

    auto Renderable::GetCurrentRTV() const noexcept -> ComPtr<ID3D12Resource>
    {
        return m_context->GetCurrentRTV();
    }

    auto Renderable::GetRTVHandle() const noexcept -> CD3DX12_CPU_DESCRIPTOR_HANDLE
    {
        return m_context->GetRTVHandle();
    }

    auto Renderable::GetDSVHandle() const noexcept -> CD3DX12_CPU_DESCRIPTOR_HANDLE
    {
        return m_context->GetDSVHandle();
    }

    auto Renderable::GetOutputWidth() const noexcept -> int
    {
        return m_context->GetOutputWidth();
    }

    auto Renderable::GetOutputHeight() const noexcept -> int
    {
        return m_context->GetOutputHeight();
    }

    auto Renderable::GetDevice() const noexcept -> ComPtr<ID3D12Device>
    {
        return m_context->GetDevice();
    }

    Task<void> Renderable::ExecuteCommandAsync(Device::GraphicsCommandList&& commandList) const noexcept
    {
        auto task = commandList.GetTask();
        m_context->GetGPUScheduler()
            ->PushCommands(std::move(commandList));
        return task;
    }

    auto Renderable::CreateCommandList(D3D12_COMMAND_LIST_TYPE type) const noexcept -> Device::GraphicsCommandList
    {
        return m_context->GetCommandListPool()->Rent(type);
    }

    Task<void> Renderable::SetContext(Device::Context* context)
    {
        m_context = context;
        return Initialise();
    }
}
