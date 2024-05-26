module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Device.GPUJobExecutor;
import Engine.Graphics.Device.Context;
import Engine.Graphics.Device.GPUScheduler.RenderNode;
import Engine.Graphics.Device.CommandListPool;
import std;

using namespace Microsoft::WRL;

namespace Engine::Graphics::Device
{
    export class GPUJobExecutor
    {
    public:
        using JobID = UINT64;

        auto Initialise(Context* context) -> void;
        auto Teardown() -> void;

        auto ExecuteGPUJob(Private::RenderNode& job) -> JobID;
        auto GetFence(D3D12_COMMAND_LIST_TYPE type) const noexcept -> const ComPtr<ID3D12Fence>&
        {
            return m_fences[type];
        }
        auto GetCurrentJobID(D3D12_COMMAND_LIST_TYPE type) const noexcept -> JobID
        {
            return m_fenceValues[type];
        }
        auto Signal(D3D12_COMMAND_LIST_TYPE type) noexcept -> JobID;

    private:

        Context* m_context = nullptr;

        std::array<ComPtr<ID3D12Fence>, CommandListPool::MAX_COMMAND_LIST_TYPE> m_fences;
        std::array<UINT64, CommandListPool::MAX_COMMAND_LIST_TYPE> m_fenceValues{};
    };
}
